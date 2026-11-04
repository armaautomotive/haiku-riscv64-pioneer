/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_Q4_FPU_DECODE_MODEL_ONLY
#include "pioneer_q38_q4_fpu_decode_test.cpp"
#include "../PioneerQ38VectorQ4Metadata.h"
#include "../PioneerQ38BinaryScale.h"
#include <cassert>
using V=PioneerQ38VectorQ4Metadata;

// Reuse the original strict qualified FMA/issue-latency observer. Extend
// ONLY integer shifts/immediates and the existing SM32->FP32 operand domain;
// these encodings are already exercised by the independent SFPU divide model.
// Synthetic lane endpoints ONLY,not silicon or a native loading/launch proof.
struct MetadataMachine:Q4FpuMachine {
	void Op(uint32_t word)
	{
		const unsigned code=word>>24,reg=(word>>4)&15,source=(word>>8)&15,mod=word&15;
		if(code!=0x7a && !(code==0x79 && mod==5) && code!=0x90){Q4FpuMachine::Op(word);return;}
		assert(okay && reg<8 && source<8);uint32_t output[32]={};
		for(unsigned lane=0;lane<32;lane++) {
			const uint32_t value=Read(source,lane);
			if(code==0x90) {
				assert(!(word&0x00fff00f) && (value&0x7fffffff)<=2047);
				const float magnitude=float(value&0x7fffffff);
				output[lane]=PioneerFmaBits(value>>31?-magnitude:magnitude);
			} else if(code==0x7a) {
				assert(source==reg && (mod==1 || mod==3));const int amount=int32_t(word<<8)>>20;
				assert(amount>-32 && amount<32);
				output[lane]=amount>=0?value<<unsigned(amount):mod==3
					?uint32_t(int32_t(value)>>unsigned(-amount)):value>>unsigned(-amount);
			} else {
				assert(source==reg);output[lane]=value+uint32_t(int32_t(word<<8)>>20);
			}
		}
		for(unsigned lane=0;lane<32;lane++)lanes[reg][lane]=output[lane];
		ready[reg]=cycle+1;cycle++;
	}
	void Execute(const Q4FpuWords& words){for(uint32_t word:words.words)Op(word);assert(okay);}
};

#ifndef PIONEER_Q38_VECTOR_Q4_METADATA_MODEL_ONLY
int main()
{
	Q4FpuWords half,bases,rawBases,products,factors[8];
	V::Half0(half);V::Bases(bases);V::RawBases(rawBases);V::Products(products);
	V::Factors<0>(factors[0]);V::Factors<1>(factors[1]);V::Factors<2>(factors[2]);V::Factors<3>(factors[3]);
	V::Factors<4>(factors[4]);V::Factors<5>(factors[5]);V::Factors<6>(factors[6]);V::Factors<7>(factors[7]);
	for(unsigned g=0;g<8;g++)assert(factors[g].words==factors[g<4?0:4].words);
	unsigned finite=0,refused=0;
	for(unsigned first=0;first<65536;first+=32) {
		if(!V::FiniteHalf(first)){for(unsigned i=0;i<32;i++){assert(!V::FiniteHalf(first+i));refused++;}continue;}
		MetadataMachine machine;uint32_t preserved[32];
		for(unsigned lane=0;lane<32;lane++) {
			assert(V::FiniteHalf(first+lane));machine.lanes[0][lane]=first+lane;
			machine.lanes[1][lane]=preserved[lane]=0x44332200+lane;
		}
		machine.Execute(half);
		for(unsigned lane=0;lane<32;lane++) {
			assert(machine.lanes[0][lane]==PioneerFmaBits(PioneerHalfFloat(uint16_t(first+lane))));
			assert(machine.lanes[1][lane]==preserved[lane]);finite++;
		}
	}
	assert(finite==63488 && refused==2048 && !V::FiniteHalf(65536) && !V::FiniteHalf(UINT32_MAX));
	uint64_t factorCases=0,productCases=0;
	// Every possible byte triple,not just representative high-bit fields.
	for(unsigned a=0;a<256;a++)for(unsigned b=0;b<256;b++)for(unsigned first=0;first<256;first+=32) {
		MetadataMachine machine;
		for(unsigned lane=0;lane<32;lane++) {
			machine.lanes[0][lane]=a;machine.lanes[1][lane]=b;machine.lanes[2][lane]=first+lane;
			machine.lanes[4][lane]=0x3f800000+lane;machine.lanes[5][lane]=0xbf800000+lane;
		}
		machine.Execute(factors[4]);
		for(unsigned lane=0;lane<32;lane++) {
			assert(machine.lanes[0][lane]==((a&15)|((b>>6)<<4)));
			assert(machine.lanes[1][lane]==((a>>4)|(((first+lane)>>6)<<4)));
			assert(machine.lanes[4][lane]==0x3f800000+lane && machine.lanes[5][lane]==0xbf800000+lane);factorCases++;
		}
	}
	for(unsigned a=0;a<256;a++)for(unsigned first=0;first<256;first+=32) {
		MetadataMachine machine;
		for(unsigned lane=0;lane<32;lane++){machine.lanes[0][lane]=a;machine.lanes[1][lane]=first+lane;}
		machine.Execute(factors[0]);
		for(unsigned lane=0;lane<32;lane++) {assert(machine.lanes[0][lane]==(a&63) && machine.lanes[1][lane]==((first+lane)&63));factorCases++;}
	}
	// ALL finite HALF encodings times ALL64 scale/minimum choices,including
	// signed zeros and subnormal HALF. Reference is the unchanged scalar
	// BinaryScale integer normalizer,not this new conversion/formula.
	for(unsigned first=0;first<65536;first+=32) {
		if(!V::FiniteHalf(first))continue;
		MetadataMachine machine;
		uint16_t d[32],dmin[32];
		for(unsigned lane=0;lane<32;lane++) {
			d[lane]=uint16_t(first+lane);dmin[lane]=uint16_t((first+31-lane)^0x8000);
			machine.lanes[0][lane]=d[lane]&255;machine.lanes[1][lane]=d[lane]>>8;
			machine.lanes[2][lane]=dmin[lane]&255;machine.lanes[3][lane]=dmin[lane]>>8;
		}
		machine.Execute(rawBases);
		uint32_t base[2][32];
		for(unsigned lane=0;lane<32;lane++) {
			assert(machine.lanes[4][lane]==PioneerFmaBits(PioneerHalfFloat(d[lane])));
			assert(machine.lanes[5][lane]==PioneerFmaBits(PioneerHalfFloat(dmin[lane])));
			base[0][lane]=machine.lanes[4][lane];base[1][lane]=machine.lanes[5][lane];
		}
		for(unsigned factor=0;factor<64;factor++) {
			for(unsigned lane=0;lane<32;lane++){machine.lanes[0][lane]=factor;machine.lanes[1][lane]=factor;}
			machine.Execute(products);
			for(unsigned lane=0;lane<32;lane++) {
				uint32_t coefficient=0,minimum=0;
				assert(PioneerQ38BinaryScale(d[lane],int(factor),coefficient));
				assert(PioneerQ38BinaryScale(dmin[lane],-int(factor),minimum));
				assert(machine.lanes[6][lane]==coefficient && machine.lanes[7][lane]==minimum);
				assert(machine.lanes[4][lane]==base[0][lane] && machine.lanes[5][lane]==base[1][lane]);productCases++;
			}
		}
	}
	printf("PASS_VECTOR_Q4_METADATA_ARITHMETIC finiteHalves%u nonfiniteRefusals%u factorTriples%llu exactCoefficientMinimumPairs%llu halfWords%zu baseWords%zu groupProductWords%zu;strict issue-latency+unchanged scalar reference,SYNTHETIC_LANES_ONLY;NO worker/native/hardware/speed result\n",
	 finite,refused,(unsigned long long)factorCases,(unsigned long long)productCases,half.words.size(),rawBases.words.size(),products.words.size());
	(void)&Round;
}
#endif
