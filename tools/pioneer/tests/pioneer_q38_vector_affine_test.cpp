/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38VectorAffine.h"
#include "../PioneerQ38BinaryScale.h"
#include "../PioneerBlackholeFma.h"
#include <vector>
#include <cmath>
#include <cstdio>
struct AffineWords { std::vector<uint32_t> words; void Op(uint32_t w) { words.push_back(w); } };
struct AffineVector {
	uint32_t lr[8][32]={}; unsigned ready[8]={},cycle=0; bool okay=true;
	uint32_t Read(unsigned reg,unsigned lane) {
		if (reg==9) return 0; if (reg==10) return 0x3f800000;
		assert(reg<8); if (ready[reg]>cycle) okay=false; return lr[reg][lane];
	}
	void Op(uint32_t word) {
		if (!okay) return;
		unsigned code=word>>24,vd=(word>>4)&15,vc=(word>>8)&15;
		if (code==0x8f) { assert(word==0x8f000000); cycle++; return; }
		assert(vd<8);
		uint32_t output[32]={};
		if (code==0x90) {
			assert(!(word&0x00fff00f) && vc<8);
			for (unsigned lane=0;lane<32;lane++) {
				uint32_t bits=Read(vc,lane),magnitude=bits&0x7fffffff;
				assert(magnitude<=(vc==0?262144U:2048U) && (magnitude || !bits));
				int value=int(magnitude)*((bits>>31)?-1:1); output[lane]=PioneerFmaBits(float(value));
			}
		} else {
			unsigned va=(word>>16)&15,vb=(word>>12)&15,mod=word&15;
			assert((code==0x86 && vc==9 && mod==2) || (code==0x85 && va==10 && !mod));
			for (unsigned lane=0;lane<32;lane++) {
				uint32_t a=Read(va,lane),b=Read(vb,lane),c=Read(vc,lane); if (mod&2) c^=0x80000000;
				output[lane]=PioneerBlackholeFmaBits(a,b,c);
			}
		}
		for (unsigned lane=0;lane<32;lane++) lr[vd][lane]=output[lane];
		ready[vd]=cycle+(code==0x90?1:2); cycle++;
	}
};
static uint32_t SM(int value) { return (value<0?0x80000000U:0)|unsigned(value<0?-value:value); }
#ifndef PIONEER_Q38_VECTOR_AFFINE_TEST_MODEL_ONLY
int main()
{
	AffineWords program;
#ifdef PIONEER_Q38_AFFINE_FLOAT_DOT_TEST
	PioneerQ38VectorAffineFloatDot(program); assert(program.words.size()==13);
#else
	PioneerQ38VectorAffine(program); assert(program.words.size()==14);
#endif
	uint32_t seed=0x8f15b3d7; unsigned cases=0;
	auto random=[&]() { seed^=seed<<13; seed^=seed>>17; seed^=seed<<5; return seed; };
	for (unsigned trial=0;trial<32768;trial++) {
		AffineVector m; uint32_t expected[32]={};
		for (unsigned lane=0;lane<32;lane++) {
			unsigned half=(random()%0x7c00)|(random()&0x8000),minHalf=(random()%0x7c00)|(random()&0x8000);
			uint32_t coefficient=0,minimum=0;
			assert(PioneerQ38BinaryScale(uint16_t(half),int(random()%256)-128,coefficient));
			assert(PioneerQ38BinaryScale(uint16_t(minHalf),int(random()%256)-128,minimum));
			unsigned exp=95+random()%49; uint32_t scale=(exp<<23)|(exp==143?0:random()&0x7fffff)|(random()&0x80000000);
			int dot=int(random()%524289)-262144,sum=int(random()%4097)-2048;
			uint32_t accumulated=((47+random()%147)<<23)|(random()&0x7fffff)|(random()&0x80000000);
			if (!(trial%64)) { coefficient=minimum=0; scale=0xbf800000; dot=sum=1; accumulated=0x80000000; }
			if (!(trial%31)) dot=0;
			if (!(trial%29)) sum=0;
			if (!(trial%23)) scale=random()&0x80000000;
			m.lr[0][lane]=SM(dot); m.lr[1][lane]=coefficient; m.lr[2][lane]=minimum;
#ifdef PIONEER_Q38_AFFINE_FLOAT_DOT_TEST
			m.lr[0][lane]=PioneerFmaBits(float(dot));
#endif
			m.lr[3][lane]=scale; m.lr[4][lane]=SM(sum); m.lr[5][lane]=accumulated;
			float product=(PioneerFmaFloat(coefficient)*PioneerFmaFloat(scale))*float(dot);
			float correction=(PioneerFmaFloat(minimum)*PioneerFmaFloat(scale))*float(sum);
			float value=PioneerFmaFloat(accumulated)+(product+correction);
			assert(std::isfinite(value) && (!(PioneerFmaBits(value)&0x7fffffff) || std::isnormal(value)));
			expected[lane]=PioneerFmaBits(value);
		}
		for (uint32_t word:program.words) m.Op(word);
		assert(m.okay);
		for (unsigned lane=0;lane<32;lane++) { assert(m.lr[0][lane]==expected[lane]); cases++; }
	}
	unsigned timingFaults=0;
	for (unsigned at=0;at<program.words.size();at++) if (program.words[at]==0x8f000000) {
		AffineVector m; for (unsigned lane=0;lane<32;lane++) { m.lr[0][lane]=SM(1); m.lr[4][lane]=SM(1); m.lr[1][lane]=m.lr[2][lane]=m.lr[3][lane]=0x3f800000; }
		for (unsigned i=0;i<program.words.size();i++) if (i!=at) m.Op(program.words[i]);
		(void)m.Read(0,0); // caller's first publication read is also a consumer
		// Some independent chains naturally cover a missing gap;only
		// actual read-after-write violations must be rejected.
		if (!m.okay) timingFaults++;
	}
	assert(timingFaults>=3);
	printf("PASS OFFLINE vector affine:%u exact scalar-RISC FP32 comparisons,ALL32 lanes,canonical integer bounds,half/factor metadata,activation scale limits,signed zeros,non-fused rounding;%zu instructions and%u dependent-latency faults rejected;NO Dst loading/image/driver/native/cycle-speed qualification\n",cases,program.words.size(),timingFaults);
}
#endif
