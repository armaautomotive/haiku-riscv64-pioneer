/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDecoderLayout.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDecoderGreedy.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPairedFfnPlan.h"
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
	{ unsigned cover[2][96]={},workers[2]={},advance[119]={}; PioneerPairedFfnSpan s={};
		assert(!PioneerPairedFfnWorker(0,s) && !PioneerPairedFfnWorker(120,s));
		for (unsigned w=1;w<=119;w++) { assert(PioneerPairedFfnWorker(w,s) && s.tensor<2 && (s.rows==32 || s.rows==64) && !(s.first&31) && s.first+s.rows<=3072);
			workers[s.tensor]++; for (unsigned b=s.first/32;b<(s.first+s.rows)/32;b++) cover[s.tensor][b]++; }
		assert(workers[0]==60 && workers[1]==59); for (auto& tensor:cover) for (unsigned hits:tensor) assert(hits==1);
		for (unsigned layer=0;layer<28;layer++) { for (unsigned count:{64U,32U,32U,32U,119U,32U}) for (unsigned w=0;w<count;w++) advance[w]++;
			for (unsigned w=0;w<8;w++) advance[w]++; for (unsigned w=0;w<6;w++) advance[w]++; }
		for (unsigned w=0;w<119;w++) assert(++advance[w]==PioneerDecoderSequenceAdvance(w,true,true,true));
		assert(advance[0]==225 && advance[6]==197 && advance[8]==169 && advance[32]==57 && advance[64]==29 && advance[118]==29);
	}
	{ uint32_t packet[32]={TT_DECODER_GREEDY_MAGIC,17,151936,151935,0x80000000,256,119,9};
		assert(PioneerDecoderGreedyValid(packet,17,256));
		for (unsigned i=0;i<32;i++) { auto saved=packet[i]; packet[i]=i==3?151936:i==4?0x7fc00000:saved^1;
			assert(!PioneerDecoderGreedyValid(packet,17,256)); packet[i]=saved; }
		assert(!PioneerDecoderGreedyValid(packet,0,256) && !PioneerDecoderGreedyValid(packet,17,0));
		assert(PioneerDecoderLogitKey(0)==PioneerDecoderLogitKey(0x80000000));
		uint32_t seed=0x9182a7fb; auto random=[&]() { seed^=seed<<13; seed^=seed>>17; seed^=seed<<5; return seed; };
		for (unsigned i=0;i<2000000;i++) { uint32_t a=random(),b=random(); if ((a&0x7f800000)==0x7f800000 || (b&0x7f800000)==0x7f800000) continue;
			assert((PioneerDecoderLogitKey(a)>PioneerDecoderLogitKey(b))==(PioneerAttentionFloat(a)>PioneerAttentionFloat(b))); }
	}
	auto job=Job(); assert(PioneerDecoderJobValid(job) && sizeof(job)<=4096);
	for (unsigned version:{5U,6U,7U,8U}) { auto j=job; j.version=version; for (unsigned op:{8U,9U}) { j.operation=op; assert(PioneerDecoderJobValid(j,version) && !PioneerDecoderJobValid(j,version-1)); }
		for (unsigned i=0;i<119;i++) { unsigned advance=PioneerDecoderSequenceAdvance(i,true,true,true,version>=8); auto bounded=j;
			bounded.sequence[i]=0x7fffffff-advance; assert(PioneerDecoderJobValid(bounded,version)); bounded.sequence[i]++; assert(!PioneerDecoderJobValid(bounded,version)); }
	}
	for (bool narrow:{false,true}) for (bool table:{false,true}) if (!narrow || table) for (bool small:{false,true}) for (unsigned polls:{0U,2U,131070U}) {
		unsigned r=(small?38334:38452)+polls-(table?9219:0)+(narrow?15372:0),rf=(small?70763:80257)+polls-(table?9219:0)+(narrow?10248:0),w=(small?18831:18949)-(table?9219:0)+(narrow?5124:0),wf=(small?18832:28326)-(table?9219:0)+(narrow?5124:0);
		uint32_t before[8],after[8],s[24]={r,rf,w,wf,119,151936,17,small?9U:8U};
		const unsigned delta[]={r,rf,r+w,r,w,wf,r+w,w};
		for (unsigned i=0;i<8;i++) { before[i]=i==6?0xfffffff2:0xfffffff0+i; after[i]=before[i]+delta[i]; s[8+i]=before[i]; s[16+i]=after[i]; }
		assert(PioneerDecoderSummary(s,256,17,before,after,true,true,small,true,true,false,table,narrow));
		assert(!PioneerDecoderSummary(s,256,17,before,after,true,true,small,true,false));
		for (unsigned i=0;i<24;i++) { s[i]^=1; assert(!PioneerDecoderSummary(s,256,17,before,after,true,true,small,true,true,false,table,narrow)); s[i]^=1; }
	}
	{ auto greedy=job; greedy.version=4; for (unsigned op:{8U,9U}) { greedy.operation=op;
		assert(PioneerDecoderJobValid(greedy,4) && !PioneerDecoderJobValid(greedy,3)); }
		greedy.operation=10; assert(!PioneerDecoderJobValid(greedy,4)); }
	{ auto glu=job; glu.version=3; assert(!PioneerDecoderJobValid(glu) && !PioneerDecoderJobValid(glu,2) && PioneerDecoderJobValid(glu,3));
		for (unsigned w=0;w<119;w++) { unsigned advance=PioneerDecoderSequenceAdvance(w,true)+(w<6?28:0);
			assert(PioneerDecoderSequenceAdvance(w,true,true)==advance); auto j=glu; j.sequence[w]=0x7fffffff-advance;
			assert(PioneerDecoderJobValid(j,3)); j.sequence[w]++; assert(!PioneerDecoderJobValid(j,3)); }
	}
	{ auto parallel=job; parallel.version=2; assert(!PioneerDecoderJobValid(parallel) && PioneerDecoderJobValid(parallel,2));
		for (unsigned w=0;w<119;w++) {
			unsigned advance=PioneerDecoderSequenceAdvance(w)+(w<8?28:0);
			assert(PioneerDecoderSequenceAdvance(w,true)==advance);
			auto j=parallel; j.sequence[w]=0x7fffffff-advance; assert(PioneerDecoderJobValid(j,2));
			j.sequence[w]++; assert(!PioneerDecoderJobValid(j,2));
		}
		assert(!PioneerDecoderJobValid(parallel,0) && !PioneerDecoderJobValid(parallel,3));
	}
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
