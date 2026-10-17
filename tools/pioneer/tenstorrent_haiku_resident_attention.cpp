/* SPDX-License-Identifier: MIT */
#include "TenstorrentHaikuResident.h"
#include "PioneerFloat16.h"
#include "PioneerAttentionMath.h"
#include <math.h>
#include <time.h>
#include <vector>
static double Seconds() { struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+t.tv_nsec*1e-9; }
int main(int argc,char** argv)
{
	bool blocks=argc==2 && !strcmp(argv[1],"--run-blocks-and-restore");
	bool wide=argc==2 && !strcmp(argv[1],"--coordinated-wide-and-restore");
	bool vocabulary=argc==2 && !strcmp(argv[1],"--coordinated-vocabulary-and-restore");
	bool coordinatedAttention=argc==2 && !strcmp(argv[1],"--coordinated-attention-and-restore");
	bool support=argc==2 && !strcmp(argv[1],"--check-coordinated-support");
	if (support) {
		int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR); if (fd<0) return 1;
		tt_coordinated_support s={}; int status=ioctl(fd,TT_PCI_PROBE_PIONEER_COORDINATED_SUPPORT,&s,sizeof(s)); close(fd);
		printf("Coordinated support status%d v%u workers%u KV-v%u immutable%u gather%u\n",status,s.version,s.max_workers,s.attention_version,s.immutable_weights,s.gather_max);
		return status || s.version!=2 || s.max_workers!=119 || s.attention_version!=3 || s.immutable_weights!=1 || s.gather_max!=8192;
	}
	bool coordinated=wide || vocabulary || coordinatedAttention || (argc==2 && !strcmp(argv[1],"--coordinated-matrices-and-restore"));
	if (!blocks && !coordinated && (argc!=2 || strcmp(argv[1],"--run-and-restore"))) { fprintf(stderr,"Usage: %s --run-and-restore|--run-blocks-and-restore|--coordinated-matrices-and-restore|--coordinated-wide-and-restore|--coordinated-vocabulary-and-restore|--coordinated-attention-and-restore|--check-coordinated-support\n",argv[0]); return 2; }
	setvbuf(stdout,NULL,_IONBF,0); setvbuf(stderr,NULL,_IONBF,0);
	PioneerResidentDevice device;
	if (!device.Open(UINT64_C(0x80000000),false,true,true,true,true,true,true,true,1,coordinated?119:120,true,blocks||coordinated,coordinated)) return 1;
	unsigned rows=vocabulary?151936:wide?6144:64;
	uint32_t x[512],scale[32]; std::vector<uint32_t> y(rows,0);
	uint64_t weightBytes=PioneerQ8CompactWeightBytes(1024,rows);
	std::vector<uint32_t> packed(size_t(weightBytes/4),0);
	for (unsigned row=0;row<rows;row++) for (unsigned b=0;b<32;b++) {
		packed[PioneerQ8NativeF32ScaleByte(1024,b,row)/4]=0x3f800000;
		for (unsigned i=0;i<32;i++) {
			unsigned k=b*32+i; int q=int((row*13+k*7)%31)-15;
			uint8_t encoded=0; if (!PioneerInt8UnpackEncode(q,encoded)) return 1;
			uint64_t p=PioneerQ8NativeF32ValueByte(1024,k,row); packed[p/4]|=uint32_t(encoded)<<(8*(p&3));
		}
	}
	for (auto& w:x) w=0x40004000; // valid integer-overlay zeros
	for (auto& w:scale) w=0x3f800000;
	bool okay=true;
	for (uint64_t at=0;at<weightBytes && okay;at+=16384) {
		unsigned pages=unsigned((weightBytes-at)/4096); if (pages>4) pages=4;
		okay=device.UploadPages(at,packed.data()+at/4,pages);
	}
	okay=okay && device.Q8Matvec(0,1024,rows,x,scale,y.data());
	if (coordinated && !coordinatedAttention) {
		for (unsigned trial=0;trial<3 && okay;trial++) {
			for (unsigned k=0;k<1024;k++) {
				uint32_t v=PioneerInt8Operand(int((k*17+trial*7)%31)-15);
				if (!(k&1)) x[k/2]=v; else x[k/2]|=v<<16;
			}
			unsigned currentRows=trial==1?32:rows;
			double start=Seconds(); okay=device.Q8Matvec(0,1024,currentRows,x,scale,y.data()); double elapsed=Seconds()-start;
			for (unsigned row=0;row<currentRows && okay;row++) {
				int sum=0; for (unsigned k=0;k<1024;k++) sum+=(int((row*13+k*7)%31)-15)*(int((k*17+trial*7)%31)-15);
				okay=y[row]==PioneerAttentionBits(float(sum));
			}
			printf("NATIVE on-card coordinator trial%u full%u rows peak%u math workers %s wall%.6fs; CPU reference OUTSIDE clock\n",trial,currentRows,device.peakWorkers,okay?"PASS":"FAIL",elapsed);
		}
		bool clean=device.Finish();
		printf("%s coordinated matrix primitive; cleanup%u; NOT model/attention qualification or tokens/s\n",okay && clean?"PASS":"FAIL",clean);
		return okay && clean?0:1;
	}
	std::vector<uint32_t> q(2048),k(512),v(512),output(2048);
	std::vector<uint16_t> historyK(4*1024),historyV(4*1024);
	for (unsigned n=1;n<=4 && okay;n++) {
		for (unsigned j=0;j<q.size();j++) q[j]=PioneerAttentionBits(float(sin(j*.713+n*.371)*1.7));
		for (unsigned j=0;j<1024;j++) {
			historyK[(n-1)*1024+j]=PioneerFloat16(float(sin(j*.429+n)*1.3));
			historyV[(n-1)*1024+j]=PioneerFloat16(float(cos(j*.139+n)*2));
		}
		memcpy(k.data(),historyK.data()+(n-1)*1024,2048); memcpy(v.data(),historyV.data()+(n-1)*1024,2048);
		for (unsigned layer:{0U,27U}) {
			double start=Seconds(); okay=device.ResidentAttentionStep(n,layer,q.data(),k.data(),v.data(),output.data()); double elapsed=Seconds()-start;
			if (!okay) break;
			double delta=0,norm=0;
			for (unsigned g=0;g<8;g++) {
				std::vector<float> query(256),reference(256); std::vector<uint16_t> key(n*128),value(n*128);
				memcpy(query.data(),q.data()+g*256,1024);
				for (unsigned t=0;t<n;t++) for (unsigned d=0;d<128;d++) { key[t*128+d]=historyK[t*1024+g*128+d]; value[t*128+d]=historyV[t*1024+g*128+d]; }
				okay=PioneerAttentionHead(query.data(),key.data(),value.data(),n,128,reference.data())
					&& PioneerAttentionHead(query.data()+128,key.data(),value.data(),n,128,reference.data()+128);
				for (unsigned j=0;j<256;j++) { float a=PioneerAttentionFloat(output[g*256+j]); double d=double(a)-reference[j]; delta+=d*d; norm+=double(reference[j])*reference[j]; }
			}
			double error=sqrt(delta/(norm+1e-30)); okay &= isfinite(error) && error<.00002;
			printf("NATIVE resident-KV all8 groups layer%u n%u wall%.6fs reference-L2%.9g %s\n",layer,n,elapsed,error,okay?"PASS":"FAIL");
			if (!okay) break;
			for (unsigned k=0;k<1024;k++) {
				uint32_t v=PioneerInt8Operand(int((k*17+n*7+layer*11)%31)-15);
				if (!(k&1)) x[k/2]=v; else x[k/2]|=v<<16;
			}
			okay=device.Q8Matvec(0,1024,64,x,scale,y.data());
			for (unsigned row=0;row<64 && okay;row++) {
				int sum=0;
				for (unsigned k=0;k<1024;k++) sum+=(int((row*13+k*7)%31)-15)*(int((k*17+n*7+layer*11)%31)-15);
				okay=y[row]==PioneerAttentionBits(float(sum));
			}
			printf("NATIVE adjacent2-worker projection changed-input full64 rows %s\n",okay?"PASS":"FAIL");
		}
	}
	bool clean=device.Finish();
	printf("%s: resident-KV bounded native qualification; cleanup%u; NOT full-model performance\n",okay && clean?"PASS":"FAIL",clean);
	return okay && clean?0:1;
}
