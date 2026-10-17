/* SPDX-License-Identifier: MIT */
#include "TenstorrentHaikuResident.h"
#include "PioneerGraphMath.h"
#include <math.h>
#include <time.h>
#include <vector>
static double Seconds() { timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+t.tv_nsec*1e-9; }
static int Weight(unsigned phase,unsigned row,unsigned k) { return int((phase*7+row*3+k*13+k/32)%15)-7; }
static float Scale(unsigned phase,unsigned row,unsigned b) { return (1+(phase*3+row+b)%7)*.0009765625f; }
static bool Quantize(const uint32_t* values,uint32_t* operands,uint32_t* scales)
{
	// Independent pinned-Q8 reference,OUTSIDE every timed card call.
	for (unsigned b=0;b<96;b++) {
		float maximum=0;
		for (unsigned i=0;i<32;i++) maximum=fmaxf(maximum,fabsf(PioneerAttentionFloat(values[b*32+i])));
		float d=maximum/127.0f,id=d?1.0f/d:0;
		scales[b]=PioneerAttentionBits(PioneerAttentionHalf(PioneerAttentionToHalf(d)));
		for (unsigned i=0;i<32;i++) {
			int q=int(roundf(PioneerAttentionFloat(values[b*32+i])*id)); if (q<-127 || q>127) return false;
			uint32_t value=PioneerInt8Operand(q);
			if (!(i&1)) operands[b*16+i/2]=value; else operands[b*16+i/2]|=value<<16;
		}
	}
	return true;
}
static std::vector<float> Reference(unsigned phase,unsigned k,unsigned n,const uint32_t* x,const uint32_t* scales)
{
	std::vector<float> out(n);
	for (unsigned row=0;row<n;row++) {
		float sum=0;
		for (unsigned b=0;b<k/32;b++) {
			int dot=0;
			for (unsigned i=0;i<32;i++) {
				unsigned h=uint16_t(x[b*16+i/2]>>(16*(i&1)));
				dot+=Weight(phase,row,b*32+i)*int(h&1023)*((h&0x8000)?-1:1);
			}
			sum=fmaf(float(dot),Scale(phase,row,b)*PioneerAttentionFloat(scales[b]),sum);
		}
		out[row]=sum;
	}
	return out;
}
int main(int argc,char** argv)
{
	if (argc!=2 || strcmp(argv[1],"--run-and-restore")) { fprintf(stderr,"Usage: %s --run-and-restore\n",argv[0]); return 2; }
	setvbuf(stdout,NULL,_IONBF,0); setvbuf(stderr,NULL,_IONBF,0);
	PioneerResidentDevice device;
	if (!device.Open(UINT64_C(0x80000000),false,true,true,true,true,true,true,true,1,119,true,true,true,false,2)) return 1;
	bool okay=device.HasLayerFfn(); uint64_t offsets[3]={0,0x800000,0x1000000};
	for (unsigned phase=0;phase<3 && okay;phase++) {
		unsigned k=phase==2?3072:1024,n=phase==2?1024:3072;
		uint64_t bytes=PioneerQ8CompactWeightBytes(k,n);
		std::vector<uint32_t> weights(bytes/4,0);
		for (unsigned row=0;row<n;row++) for (unsigned b=0;b<k/32;b++) {
			weights[PioneerQ8NativeF32ScaleByte(k,b,row)/4]=PioneerAttentionBits(Scale(phase,row,b));
			for (unsigned i=0;i<32;i++) {
				uint8_t encoded=0; okay &= PioneerInt8UnpackEncode(Weight(phase,row,b*32+i),encoded);
				uint64_t at=PioneerQ8NativeF32ValueByte(k,b*32+i,row); weights[at/4]|=uint32_t(encoded)<<(8*(at&3));
			}
		}
		for (uint64_t at=0;at<bytes && okay;at+=16384) {
			unsigned pages=(bytes-at)/4096; if (pages>4) pages=4;
			okay=device.UploadPages(offsets[phase]+at,weights.data()+at/4,pages);
		}
		printf("FFN synthetic weight phase%u uploaded%u bytes%llu\n",phase,okay,(unsigned long long)bytes);
	}
	for (unsigned trial=0;trial<4 && okay;trial++) {
		uint32_t input[512],scales[32],gate[3072],up[3072],hidden[3072],downInput[1536],downScales[96],split[1024],fused[1024];
		for (unsigned i=0;i<1024;i++) {
			uint32_t value=PioneerInt8Operand(int((i*13+trial*17)%255)-127);
			if (!(i&1)) input[i/2]=value; else input[i/2]|=value<<16;
		}
		for (unsigned b=0;b<32;b++) scales[b]=PioneerAttentionBits(.0009765625f*(1+b%5));
		// First obtain independently checked split-device results. ALL CPU
		// nonlinear/reference work is outside the fused-job timer below.
		okay=device.Q8Matvec(offsets[0],1024,3072,input,scales,gate)
			&& device.Q8Matvec(offsets[1],1024,3072,input,scales,up);
		if (!okay) break;
		auto rg=Reference(0,1024,3072,input,scales),ru=Reference(1,1024,3072,input,scales);
		for (unsigned i=0;i<3072;i++) {
			okay &= gate[i]==PioneerAttentionBits(rg[i]) && up[i]==PioneerAttentionBits(ru[i]);
			hidden[i]=PioneerAttentionBits(PioneerGraphSiluFast(PioneerAttentionFloat(gate[i]))*PioneerAttentionFloat(up[i]));
		}
		okay=okay && Quantize(hidden,downInput,downScales) && device.Q8Matvec(offsets[2],3072,1024,downInput,downScales,split);
		if (!okay) break;
		uint32_t cardHidden[3072]; double gluStart=Seconds(); okay=device.LayerGlu(3072,gate,up,cardHidden); double gluWall=Seconds()-gluStart;
		unsigned gluExact=0; for (unsigned i=0;i<3072 && okay;i++) gluExact+=hidden[i]==cardHidden[i];
		okay &= gluExact==3072; printf("NATIVE separately scheduled GPU GLU trial%u exact%u/3072 wall%.6fs PASS%u\n",trial,gluExact,gluWall,okay);
		if (!okay) break;
		double start=Seconds(); okay=device.LayerFfn(offsets[0],offsets[1],offsets[2],input,scales,fused); double wall=Seconds()-start;
		unsigned exact=0; double diff=0,norm=0;
		for (unsigned i=0;i<1024 && okay;i++) {
			exact+=fused[i]==split[i]; double d=double(PioneerAttentionFloat(fused[i]))-PioneerAttentionFloat(split[i]); diff+=d*d; norm+=double(PioneerAttentionFloat(split[i]))*PioneerAttentionFloat(split[i]);
		}
		double error=sqrt(diff/(norm+1e-30)); okay &= exact==1024 && isfinite(error) && error<1e-7;
		printf("NATIVE fixed FFN trial%u exact%u/1024 L2%.9g wall%.6fs peak%u PASS%u; generated-fixture CPU references OUTSIDE timer,NOT model tok/s\n",trial,exact,error,wall,device.peakWorkers,okay);
	}
	bool clean=device.Finish(); printf("%s fixed FFN native primitive cleanup%u; NOT full-model quality or speed qualification\n",okay && clean?"PASS":"FAIL",clean);
	return okay && clean?0:1;
}
