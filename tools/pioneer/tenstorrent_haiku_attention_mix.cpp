/* SPDX-License-Identifier: MIT */
// Public synthetic handoff proof; no model/chat reads or Assistant changes.
#include "TenstorrentHaikuResident.h"
#include "PioneerAttentionMath.h"
#include <math.h>
#include <vector>
static int W(unsigned row,unsigned k) { return int((row*13+k*7)%31)-15; }
static int X(unsigned k,unsigned trial) { return int((k*3+trial*5)%7)-3; }
int main(int argc,char** argv)
{
	if (argc!=2 || (strcmp(argv[1],"--check-support") && strcmp(argv[1],"--run-shared-attention-test"))) {
		fprintf(stderr,"Usage: %s --check-support | --run-shared-attention-test\nPublic synthetic matrix/attention handoff only; not full inference.\n",argv[0]); return 2;
	}
	setvbuf(stdout,NULL,_IONBF,0);
	int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR); if (fd<0) return 1;
	tt_resident_session_result s={}; int status=ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_SUPPORT,&s,sizeof(s)); close(fd);
	bool supported=!status && s.version==1 && (s.capabilities&TT_PIONEER_CAP_ATTENTION_TILE_SHARED);
	printf("Shared attention capability=%u v=%u mask=%x status=%d\n",supported,s.version,s.capabilities,status);
	if (!supported || !strcmp(argv[1],"--check-support")) return supported?0:1;
	const uint64_t capacity=UINT64_C(2)*1024*1024*1024;
	unsigned identity=1;
	PioneerResidentDevice device;
	PioneerResidentTensorCache<PioneerResidentDevice> cache(device,capacity,false,true,true,true);
	bool passed=device.Open(capacity,false,true,true,true,true,true,true,true);
	uint64_t offset=0;
	if (passed) passed=cache.Q8Weights(&identity,8,1024,64,[](unsigned row,unsigned block,uint8_t* out) {
		out[0]=0; out[1]=0x3c;
		for (unsigned i=0;i<32;i++) out[2+i]=uint8_t(W(row,block*32+i));
		return true;
	},offset);
	for (unsigned trial=0;trial<4 && passed;trial++) {
		uint8_t x[32*34]; float result[64];
		for (unsigned block=0;block<32;block++) {
			x[block*34]=0; x[block*34+1]=0x3c;
			for (unsigned i=0;i<32;i++) x[block*34+2+i]=uint8_t(X(block*32+i,trial%2));
		}
		passed=cache.Q8Multiply(offset,1024,64,x,result);
		for (unsigned row=0;row<64 && passed;row++) {
			int expected=0; for (unsigned k=0;k<1024;k++) expected+=W(row,k)*X(k,trial%2);
			passed=result[row]==float(expected);
		}
		printf("SHARED matrix trial=%u exact-integer-dot %s\n",trial,passed?"PASS":"FAIL");
		if (!passed) break;
		unsigned n=trial==3?256:trial+1;
		float q[256],reference[256]; uint32_t out[256],queryBits[256];
		std::vector<uint16_t> k(n*128),v(n*128);
		std::vector<uint32_t> keyWords(n*64),valueWords(n*64);
		for (unsigned i=0;i<256;i++) { q[i]=float(sin(i*.713)*1.7); queryBits[i]=PioneerAttentionBits(q[i]); }
		for (unsigned i=0;i<k.size();i++) { k[i]=PioneerFloat16(float(sin(i*.429)*1.3)); v[i]=PioneerFloat16(float(cos(i*.139)*2)); }
		for (unsigned w=0;w<n*64;w++) { keyWords[w]=k[w*2]|(uint32_t(k[w*2+1])<<16); valueWords[w]=v[w*2]|(uint32_t(v[w*2+1])<<16); }
		passed=PioneerAttentionHead(q,k.data(),v.data(),n,128,reference)
			&& PioneerAttentionHead(q+128,k.data(),v.data(),n,128,reference+128)
			&& device.AttentionTile(n,queryBits,keyWords.data(),valueWords.data(),out);
		double norm=0,delta=0;
		if (passed) for (unsigned i=0;i<256;i++) {
			float a=PioneerAttentionFloat(out[i]); if (!isfinite(a)) { passed=false; break; }
			double d=double(a)-reference[i]; delta+=d*d; norm+=double(reference[i])*reference[i];
		}
		double error=sqrt(delta/(norm?norm:1)); passed &= isfinite(error) && error<.00002;
		printf("SHARED attention trial=%u n=%u relative-L2=%.9g %s\n",trial,n,error,passed?"PASS":"FAIL");
	}
	// One final matrix proves return from the last attention entry, including
	// restoration ordering on END while the matrix entry is selected.
	if (passed) {
		uint8_t x[32*34]; float y[64];
		for (unsigned b=0;b<32;b++) { x[b*34]=0; x[b*34+1]=0x3c; for (unsigned i=0;i<32;i++) x[b*34+2+i]=0; }
		passed=cache.Q8Multiply(offset,1024,64,x,y);
		if (passed) for (float value:y) passed &= value==0;
	}
	bool clean=device.Finish();
	printf("%s: shared matrix=%llu attention=%llu pages=%llu clean=%u; no fallback, NOT tokens/s\n",
		passed && clean?"PASS":"STOP",(unsigned long long)device.calls,(unsigned long long)device.attentionCalls,(unsigned long long)device.uploads,clean);
	return passed && clean?0:1;
}
