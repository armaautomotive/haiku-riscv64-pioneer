/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDecoderLayout.h"
#include "../PioneerGraphRope.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>
static PioneerDecoderJob Job()
{
	PioneerDecoderJob j={}; j.magic=0x31444344; j.operation=8; j.version=1; j.tokens=1; j.nonce=1;
	j.kvBase=0x82400000; j.finalNorm=j.vocabulary=0x04000000; j.hostLow=0x80000000; j.hostHigh=1; j.hostBytes=0x100000; j.workers=119;
	for (auto& l:j.layers) { auto* w=reinterpret_cast<uint32_t*>(&l); for (unsigned i=0;i<11;i++) w[i]=0x04000000; }
	for (auto& s:j.sequence) s=1;
	for (unsigned i=0;i<64;i++) j.rope[i*2]=0x3f800000;
	return j;
}
int main()
{
	auto job=Job(); assert(PioneerDecoderJobValid(job) && sizeof(job)<=4096);
	unsigned advances[119]={};
	for (unsigned layer=0;layer<28;layer++) for (unsigned peers:{64U,32U,32U,32U,96U,96U,32U}) for (unsigned w=0;w<peers;w++) advances[w]++;
	for (unsigned w=0;w<119;w++) { advances[w]++; assert(advances[w]==PioneerDecoderSequenceAdvance(w));
		auto j=job; j.sequence[w]=0x7fffffff-advances[w]; assert(PioneerDecoderJobValid(j)); j.sequence[w]++; assert(!PioneerDecoderJobValid(j)); }
	for (unsigned layer=0;layer<28;layer++) for (unsigned field=0;field<11;field++) {
		auto j=job; auto* words=reinterpret_cast<uint32_t*>(&j.layers[layer]); words[field]=0x82400000; assert(!PioneerDecoderJobValid(j));
		words[field]=0x04000001; assert(!PioneerDecoderJobValid(j));
	}
	for (unsigned fault=0;fault<18;fault++) {
		auto j=job;
		if (fault==0) j.magic^=1; if (fault==1) j.operation=7; if (fault==2) j.version=2;
		if (fault==3) j.tokens=0; if (fault==4) j.tokens=257; if (fault==5) j.nonce=0;
		if (fault==6) j.nonce=0x7fffffff; if (fault==7) j.kvBase-=4096; if (fault==8) j.workers=120;
		if (fault==9) j.hostHigh=0; if (fault==10) j.hostHigh=0x10000; if (fault==11) j.hostLow+=4096;
		if (fault==12) j.hostBytes=16384; if (fault==13) j.finalNorm=0x82400000; if (fault==14) j.vocabulary=0x823ff000;
		if (fault==15) j.reserved[3]=1; if (fault==16) j.sequence[118]=0; if (fault==17) j.rope[127]=0x7fc00000;
		assert(!PioneerDecoderJobValid(j));
	}
	for (unsigned tokens:{1U,2U,64U,65U,128U,129U,256U}) for (unsigned polls:{0U,1U,1000U}) {
		unsigned r=0,rf=0,w=0,wf=0;
		const unsigned outputs[]={2048,1024,1024,1024,3072,3072,1024};
		for (unsigned layer=0;layer<28;layer++) {
			for (unsigned n:outputs) { unsigned p=n/32; r+=4*p; rf+=3*p+n/16; w+=2*p; wf+=2*p; }
			r+=4+16*((tokens+63)/64); rf+=64+64+8+8+64*tokens; w+=16; wf+=64;
		}
		r+=1+5*119+2*polls; rf+=64+3*119+151936/8+2*polls; w+=3*119; wf+=2*119+151936/16;
		uint32_t s[24]={r,rf,w,wf,119,151936,17,8},before[8],after[8];
		const unsigned delta[]={r,rf,r+w,r,w,wf,r+w,w};
		for (unsigned i=0;i<8;i++) { s[8+i]=before[i]=UINT32_MAX-7; s[16+i]=after[i]=before[i]+delta[i]; }
		assert(PioneerDecoderSummary(s,tokens,17,before,after));
		for (unsigned i=0;i<24;i++) { s[i]^=1; assert(!PioneerDecoderSummary(s,tokens,17,before,after)); s[i]^=1; }
	}
	unsigned vectors=0;
	for (unsigned n:{1024U,2048U}) for (unsigned position=0;position<256;position++) {
		std::vector<float> input(n),output(n),inplace(n); float table[128]; float theta=float(position),step=powf(1000000.0f,-2.0f/128);
		for (unsigned i=0;i<64;i++) { table[2*i]=cosf(theta); table[2*i+1]=sinf(theta); theta*=step; }
		for (unsigned i=0;i<n;i++) input[i]=float(sin(i*.173+position*.137)*12.7);
		inplace=input; assert(PioneerGraphRopeNeox(n,input.data(),table,output.data()) && PioneerGraphRopeNeox(n,inplace.data(),table,inplace.data()) && output==inplace);
		for (unsigned head=0;head<n;head+=128) for (unsigned i=0;i<64;i++) {
			float a=input[head+i],b=input[head+64+i],c=table[2*i],s=table[2*i+1];
			assert(PioneerAttentionBits(output[head+i])==PioneerAttentionBits(fmaf(a,c,-(b*s))) && PioneerAttentionBits(output[head+64+i])==PioneerAttentionBits(fmaf(a,s,b*c)));
		}
		vectors++;
	}
	float input[1024]={},output[1024],table[128]={}; for (auto& x:output) x=42;
	table[127]=INFINITY; assert(!PioneerGraphRopeNeox(1024,input,table,output)); for (auto x:output) assert(x==42);
	table[127]=0; input[1023]=NAN; assert(!PioneerGraphRopeNeox(1024,input,table,output)); for (auto x:output) assert(x==42);
	input[1023]=PioneerAttentionFloat(1); assert(!PioneerGraphRopeNeox(1024,input,table,output));
	printf("PASS OFFLINE decoder descriptor bounds/all28 layer extents/all119 exact whole-position sequence budgets,%u exact/out-of-place/in-place RoPE vectors;NO driver/native inference exposure\n",vectors);
}
