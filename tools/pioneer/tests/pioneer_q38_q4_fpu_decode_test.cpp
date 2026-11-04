/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38Q4FpuDecode.h"
#include "../PioneerBlackholeFma.h"
#include "../TenstorrentQ4K.h"
#include <array>
#include <vector>
#include <cstdio>
#include <cmath>
#include <cstring>
struct Q4FpuWords {std::vector<uint32_t> words;void Op(uint32_t word){words.push_back(word);}};
// Independent strict issue/latency observer. No device, Dst, source data
// loads, register assumptions, or shared decode/rounding oracle hidden here.
struct Q4FpuMachine {
	uint32_t lanes[8][32]={};unsigned ready[8]={},cycle=0;bool okay=true;
	uint32_t Read(unsigned r,unsigned lane) {
		if(r==9)return 0;
		if(r==10)return 0x3f800000;
		assert(r<8);if(ready[r]>cycle)okay=false;return lanes[r][lane];
	}
	void Op(uint32_t w) {
		if(!okay)return;
		if(w==0x8f000000){cycle++;return;}
		unsigned code=w>>24,d=(w>>4)&15,c=(w>>8)&15,b=(w>>12)&15,a=(w>>16)&15,mod=w&15;
		if(code==0x71)d=(w>>20)&15;
		assert(d<8);uint32_t out[32]={};unsigned latency=1;
		for(unsigned lane=0;lane<32;lane++) {
			if(code==0x71) {
				assert(!(w&0x000f0000) || ((w>>16)&15)==2 || ((w>>16)&15)==10);
				unsigned mode=(w>>16)&15;
				out[lane]=mode==0?(w&65535)<<16:mode==2?w&65535:(Read(d,lane)&0xffff0000)|(w&65535);
			} else if(code==0x90) {
				assert(!(w&0x00fff00f) && c<8);
				uint32_t sm=Read(c,lane);assert(!(sm>>31) && sm<=63);
				out[lane]=PioneerFmaBits(float(sm));
			} else if(code==0x86 || code==0x85 || code==0x84) {
				assert((code==0x86 && c==9 && mod==2) || (code==0x85 && a==10 && !mod)
					|| (code==0x84 && w==0x84010200));
				uint32_t x=Read(a,lane),y=Read(b,lane),z=Read(c,lane);if(mod&2)z^=0x80000000;
				out[lane]=PioneerBlackholeFmaBits(x,y,z);latency=2;
			} else if(code==0x7c) {assert(!mod && c<8);out[lane]=Read(c,lane);}
			else if(code==0x7a) {
				assert(mod==1 && c==d);int amount=int(w<<8)>>20;
				assert(amount==-4 || amount==-16);out[lane]=Read(d,lane)>>unsigned(-amount);
			} else if(code==0x7e) {assert(mod==1 && b<8 && c<8);out[lane]=Read(b,lane)&Read(c,lane);}
			else if(code==0x79) {assert(mod==4 && c<8);out[lane]=Read(d,lane)+Read(c,lane);}
			else if(code==0x8d) {assert(!mod && c<8);out[lane]=Read(d,lane)^Read(c,lane);}
			else {fprintf(stderr,"UNKNOWN_Q4_FPU_OPCODE %08x\n",w);assert(false);}
		}
		for(unsigned lane=0;lane<32;lane++)lanes[d][lane]=out[lane];
		ready[d]=cycle+latency;cycle++;
	}
	void Execute(const Q4FpuWords& words){for(uint32_t word:words.words)Op(word);}
};
// Independent magnitude/remainder nearest-even, not the producer's bias add.
static uint32_t Round(uint32_t f) {
	assert((f&0x7f800000)!=0x7f800000 && (!(f&0x7fffffff) || (f&0x7f800000)));
	uint32_t high=f>>16,low=f&65535;
	if(low>32768 || (low==32768 && (high&1)))high++;
	return high<<16;
}
#ifndef PIONEER_Q38_Q4_FPU_DECODE_MODEL_ONLY
static void FractionFixture(const Q4FpuWords& prep,const Q4FpuWords decode[2])
{
	uint32_t seed=0x31415926;auto random=[&](){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;};
	for(unsigned trial=0;trial<256;trial++) {
		Q4FpuMachine m;unsigned d[32],minimumHalf[32],scale[32],minimum[32],packed[32];
		for(unsigned lane=0;lane<32;lane++) {
			d[lane]=(random()%0x7c00)|(random()&0x8000);minimumHalf[lane]=(random()%0x7c00)|(random()&0x8000);
			scale[lane]=random()&63;minimum[lane]=random()&63;packed[lane]=random()&255;
			if(!trial){d[lane]=(lane&1)?0x8000:0;minimumHalf[lane]=(lane&2)?0x8000:0;}
			m.lanes[1][lane]=PioneerFmaBits(PioneerHalfFloat(uint16_t(d[lane])));
			m.lanes[2][lane]=PioneerFmaBits(PioneerHalfFloat(uint16_t(minimumHalf[lane])));
			m.lanes[3][lane]=scale[lane];m.lanes[4][lane]=minimum[lane];
		}
		m.Execute(prep);
		for(unsigned upper=0;upper<2;upper++) {
			for(unsigned lane=0;lane<32;lane++)m.lanes[0][lane]=packed[lane];
			m.Execute(decode[upper]);assert(m.okay);
			for(unsigned lane=0;lane<32;lane++)printf("Q4_FPU_FRACTION %04x %04x %u %u %u %08x\n",d[lane],minimumHalf[lane],scale[lane],minimum[lane],(packed[lane]>>(upper*4))&15,m.lanes[0][lane]);
		}
	}
}
int main(int argc,char** argv)
{
	Q4FpuWords prep,decode[2],round;PioneerQ38Q4FpuPrepare(prep);
	PioneerQ38Q4FpuDecode<Q4FpuWords,false>(decode[0]);PioneerQ38Q4FpuDecode<Q4FpuWords,true>(decode[1]);
	PioneerQ38Q4FpuRound(round);
	assert(prep.words.size()==12 && decode[0].words.size()==10 && decode[1].words.size()==10 && round.words.size()==6);
	if(argc==2){assert(!strcmp(argv[1],"--fraction-fixture"));FractionFixture(prep,decode);return 0;}assert(argc==1);
	uint32_t seed=0x48634791;auto random=[&](){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;};
	uint64_t cases=0;unsigned mismatches=0;
	for(unsigned trial=0;trial<32768;trial++) {
		Q4FpuMachine m;uint16_t d[32],min[32];unsigned scale[32],minimum[32];
		for(unsigned lane=0;lane<32;lane++) {
			d[lane]=uint16_t((random()%0x7c00)|(random()&0x8000));
			min[lane]=uint16_t((random()%0x7c00)|(random()&0x8000));
			scale[lane]=random()&63;minimum[lane]=random()&63;
			m.lanes[1][lane]=PioneerFmaBits(PioneerHalfFloat(d[lane]));
			m.lanes[2][lane]=PioneerFmaBits(PioneerHalfFloat(min[lane]));
			m.lanes[3][lane]=scale[lane];m.lanes[4][lane]=minimum[lane];
		}
		m.Execute(prep);assert(m.okay);
		for(unsigned upper=0;upper<2;upper++)for(unsigned pair=0;pair<8;pair++) {
			uint32_t expected[32],preserved[7][32];
			for(unsigned lane=0;lane<32;lane++) {
				const unsigned packed=(pair*32+lane)&255;float multiplier=PioneerHalfFloat(d[lane])*float(scale[lane]);
				float subtract=PioneerHalfFloat(min[lane])*float(minimum[lane]);
				const float product=multiplier*float((packed>>(upper*4))&15);const float value=product-subtract;
				expected[lane]=Round(PioneerFmaBits(value));m.lanes[0][lane]=packed;
				for(unsigned reg:{1U,2U,4U,5U,6U,7U})preserved[reg-1][lane]=m.lanes[reg][lane];
			}
			m.Execute(decode[upper]);assert(m.okay);
			for(unsigned lane=0;lane<32;lane++) {
				for(unsigned reg:{1U,2U,4U,5U,6U,7U})assert(m.lanes[reg][lane]==preserved[reg-1][lane]);
				if(m.lanes[0][lane]!=expected[lane]) {
					if(mismatches<8)fprintf(stderr,"Q4_FPU_MISMATCH trial%u lane%u d%04x min%04x scale%u minimum%u packed%u upper%u got%08x expected%08x\n",trial,lane,d[lane],min[lane],scale[lane],minimum[lane],(pair*32+lane)&255,upper,m.lanes[0][lane],expected[lane]);
					mismatches++;
				}cases++;
			}
		}
	}
	assert(!mismatches);
	unsigned rounded=0,tiesEven=0,wrongTruncation=0,wrongAway=0;
	Q4FpuMachine rounding;rounding.lanes[3][0]=1;rounding.Execute(prep);
	for(unsigned sign:{0U,0x80000000U})for(unsigned exponent=103;exponent<=152;exponent++)
	for(unsigned mantissa=0;mantissa<128;mantissa++)for(unsigned low:{0U,1U,32767U,32768U,32769U,65535U}) {
		uint32_t input=sign|(exponent<<23)|(mantissa<<16)|low,expected=Round(input);
		for(unsigned lane=0;lane<32;lane++)rounding.lanes[0][lane]=input;
		rounding.Execute(round);assert(rounding.okay);
		for(unsigned lane=0;lane<32;lane++)assert(rounding.lanes[0][lane]==expected);
		rounded++;if(low==32768 && !(mantissa&1))tiesEven++;
		wrongTruncation+=expected!=(input&0xffff0000);wrongAway+=expected!=((input+32768)&0xffff0000);
	}
	for(unsigned input:{0U,0x80000000U}) {
		for(unsigned lane=0;lane<32;lane++)rounding.lanes[0][lane]=input;
		rounding.Execute(round);for(unsigned lane=0;lane<32;lane++)assert(rounding.lanes[0][lane]==input);
	}
	assert(tiesEven==6400 && wrongTruncation && wrongAway);
	unsigned timingFaults=0;
	for(unsigned function=0;function<3;function++) {
		const Q4FpuWords& original=function?decode[function-1]:prep;
		for(unsigned drop=0;drop<original.words.size();drop++)if(original.words[drop]==0x8f000000) {
			Q4FpuMachine m;m.lanes[1][0]=m.lanes[2][0]=0x3f800000;m.lanes[3][0]=m.lanes[4][0]=1;
			if(function)m.Execute(prep);
			for(unsigned i=0;i<original.words.size();i++)if(i!=drop)m.Op(original.words[i]);
			(void)m.Read(0,0);if(!m.okay)timingFaults++;
		}
	}
	// The dependent MAD->round MOV needs its explicit gap in EACH decoder.
	// Prepare has no padding:independent operations supply all its latency.
	assert(timingFaults==2);
	printf("PASS OFFBOARD compressed Q4 FPU decode:%llu exact BF16-RNE original-half/scales/nibble comparisons;%u rounding-boundary cases,%u even ties,signed zeros preserved,%u latency faults;prepare%zu decode%zu/%zu instructions per32 lanes;NO Dst/unpack/MOVD2A/image/driver/hardware/model-quality/speed qualification\n",(unsigned long long)cases,rounded,tiesEven,timingFaults,prep.words.size(),decode[0].words.size(),decode[1].words.size());
}
#endif
