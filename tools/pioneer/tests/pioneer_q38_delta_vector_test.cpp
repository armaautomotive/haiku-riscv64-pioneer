/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38DeltaVector.h"
#include "../PioneerBlackholeFma.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#ifndef PIONEER_Q38_DELTA_VECTOR_EXISTING_BITS
static uint32_t Bits(float f) { uint32_t b; memcpy(&b,&f,4); return b; }
static float Float(uint32_t b) { float f; memcpy(&f,&b,4); return f; }
#endif
struct DeltaVectorMachine {
	struct OwnershipFault {};
	struct OrderingFault {};
	std::array<uint32_t,8192> state={}; //128 keys x64 value columns
	std::array<uint32_t,128> q={},k={};
	uint32_t lr[8][32]={}; bool enabled=false,drained=true,defined[512]={};
	unsigned ops=0,loads=0,stores=0,keys=0,queries=0,drains=0,latency=0;
	bool dropLatencyWait=false,dropFinalDrain=false;
	bool localImage=false;
	uint32_t KeyBits(unsigned i) { assert(i<128); keys++; return k[i]; }
	uint32_t QueryBits(unsigned i) { assert(i<128); queries++; return q[i]; }
	void Drain() { assert(!latency); if (!dropFinalDrain) { drained=true; drains++; } }
	void Op(uint32_t op) {
		ops++; drained=false;
		if (op==0x8f000000) { assert(latency==1); if (!dropLatencyWait) latency=0; return; }
		if (latency) throw OrderingFault{};
		if (op==0x8a000002) { enabled=true; return; }
		assert(enabled); unsigned code=op>>24;
		if (code==0x70 || code==0x72) {
			unsigned reg=(op>>20)&15,mode=(op>>16)&15,addr=op&1023;
			assert(mode==3 && (localImage?(reg<8):!reg) && !(addr&1) && addr<512 && !(op&0xfc00));
			for (unsigned lane=0;lane<32;lane++) {
				// Independent pinned Blackhole SFPLOAD/SFPSTORE lane mapping.
				unsigned row=(addr&~3U)+lane/8,col=(lane&7)*2+((addr&2)!=0);
				assert(row<512 && col<16);
				if (!defined[row]) throw OwnershipFault{};
				unsigned at=row*16+col;
				if (code==0x70) { lr[reg][lane]=state[at]; loads++; }
				else { uint32_t bits=lr[reg][lane]; state[at]=bits&0x7f800000?bits:bits&0x80000000; stores++; }
			}
			return;
		}
		for (unsigned lane=0;lane<32;lane++) {
			if (code==0x71) {
				unsigned r=(op>>20)&15,mode=(op>>16)&15; assert(r<8);
				if (!mode) lr[r][lane]=(op&65535)<<16;
				else if (mode==10) lr[r][lane]=(lr[r][lane]&0xffff0000)|(op&65535);
				else { assert(mode==2); lr[r][lane]=op&65535; }
			} else if (code==0x8d) {
				unsigned s=(op>>8)&15,d=(op>>4)&15; assert(s<8 && d<8 && !(op&15)); lr[d][lane]^=lr[s][lane];
			} else {
				assert(code==0x85 || code==0x86);
				unsigned a=code==0x86?(op>>16)&15:10,b=(op>>12)&15,c=code==0x86?9:(op>>8)&15,d=(op>>4)&15;
				assert(d<8 && b<8 && ((code==0x86 && a<8 && (op&15)==2) || (code==0x85 && c<8 && !(op&15))));
				uint32_t av=a==10?0x3f800000:lr[a][lane],cv=c==9?0x80000000:lr[c][lane];
				lr[d][lane]=PioneerBlackholeFmaBits(av,lr[b][lane],cv);
			}
		}
		if (code==0x85 || code==0x86) latency=1;
	}
};
#ifndef PIONEER_Q38_DELTA_VECTOR_MODEL_ONLY
int main()
{
	unsigned trials=0;
	for (unsigned sample=0;sample<6;sample++) for (float decay:{0.f,.83f,1.f}) for (float beta:{0.f,.37f,1.f}) {
		DeltaVectorMachine m; for (bool& row:m.defined) row=true;
		std::array<float,8192> expected; float v[64],delta[64],out[64]={};
		for (unsigned i=0;i<128;i++) {
			m.q[i]=Bits(sample?float(sin(i*.123+sample*.317)*.01):0);
			m.k[i]=Bits(sample?float(cos(i*.153+sample*.217)*.03):0);
		}
		for (unsigned j=0;j<64;j++) {
			v[j]=sample?float(sin(j*.013+sample*.117)*2.1):0;
			unsigned row=j/16,col=j%16,parity=col&1,lane=row*8+col/2;
			m.lr[6+parity][lane]=Bits(v[j]);
		}
		for (unsigned at=0;at<8192;at++) expected[at]=Float(m.state[at]=Bits(sample?float(sin(at*.031+sample*.17)*.003):0));
		for (unsigned j=0;j<64;j++) {
			float predicted=0;
			for (unsigned i=0;i<128;i++) predicted+=(expected[i*64+j]*decay)*Float(m.k[i]);
			delta[j]=(v[j]-predicted)*beta;
		}
		for (unsigned i=0;i<128;i++) for (unsigned j=0;j<64;j++) expected[i*64+j]=expected[i*64+j]*decay+Float(m.k[i])*delta[j];
		for (unsigned j=0;j<64;j++) for (unsigned i=0;i<128;i++) out[j]+=expected[i*64+j]*Float(m.q[i]);
		assert(PioneerQ38DeltaVectorHalf(m,Bits(decay),Bits(beta)) && m.drained && m.drains==1 && !m.latency);
		assert(m.keys==256 && m.queries==128 && m.loads==16384 && m.stores==8192);
		for (unsigned at=0;at<8192;at++) assert(m.state[at]==Bits(expected[at]));
		for (unsigned j=0;j<64;j++) {
			unsigned row=j/16,col=j%16,parity=col&1,lane=row*8+col/2;
			assert(m.lr[4+parity][lane]==Bits(out[j]));
		}
		trials++;
	}
	for (uint32_t bad:{1U,0x007fffffU,0x80000000U,0x3f800001U,0x7f800000U,0x7fc00001U,0xffffffffU}) for (bool beta:{false,true}) {
		DeltaVectorMachine m; auto before=m.state;
		assert(!PioneerQ38DeltaVectorHalf(m,beta?0x3f800000:bad,beta?bad:0x3f800000) && !m.ops && m.state==before);
	}
	for (unsigned row:{0U,255U,511U}) {
		DeltaVectorMachine m; for (bool& defined:m.defined) defined=true; m.defined[row]=false; bool caught=false;
		try { (void)PioneerQ38DeltaVectorHalf(m,0x3f800000,0x3f800000); } catch (const DeltaVectorMachine::OwnershipFault&) { caught=true; }
		assert(caught && !m.stores && !m.drains); // first pass cannot consume undefined Dst
	}
	{
		DeltaVectorMachine m; for (bool& defined:m.defined) defined=true; m.dropLatencyWait=true; bool caught=false;
		try { (void)PioneerQ38DeltaVectorHalf(m,0x3f800000,0x3f800000); } catch (const DeltaVectorMachine::OrderingFault&) { caught=true; }
		assert(caught && !m.stores && !m.drains);
	}
	{
		DeltaVectorMachine m; for (bool& defined:m.defined) defined=true; m.dropFinalDrain=true;
		assert(PioneerQ38DeltaVectorHalf(m,0x3f800000,0x3f800000) && !m.drained && !m.drains); // owner MUST refuse publication
	}
	printf("PASS OFFLINE vector GDN half-head:%u trials,ALL8192 next-state+64 output bits match independent non-fused scalar arithmetic,32 lanes/even+odd mapping,full-row ownership,explicit MAD latency/final drain,bad scalar gates ZERO issue;NO native image/state registry/NoC/GGML integration or speed claim\n",trials);
}
#endif
