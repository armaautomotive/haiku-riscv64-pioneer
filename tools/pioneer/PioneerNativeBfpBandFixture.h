/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_BFP_BAND_FIXTURE_H
#define PIONEER_NATIVE_BFP_BAND_FIXTURE_H
#include "PioneerNativeBfpBandLayout.h"
// Fixed synthetic DATA for the NEW full-width owner diagnostic. Identical
// n/128 weights and BF16 inputs to the independent full-band image fixtures;
// no user data,model,CPU learned product,address or allocation capability.
struct PioneerNativeBfpBandFixture {
	static int Numerator(unsigned tile,unsigned row,unsigned column) {
		return int((row*37+column*19+tile*11)%255)-127;
	}
	static uint8_t WeightByte(unsigned tile,unsigned index) {
		if(index<64)return 126; // each16-value group contains magnitude>=64
		if(index>=1088)return 0;
		const unsigned element=index-64,face=element/256;
		const int n=Numerator(tile,(face/2)*16+(element%256)/16,(face%2)*16+element%16);
		return uint8_t(n<0?unsigned(-n)|128:unsigned(n));
	}
	static uint16_t InputBits(unsigned tile,unsigned column) {
		return uint16_t(0x3f00+(column*13+tile*7)%256+((column&3)==2?0x8000:0));
	}
	static uint32_t Word(unsigned width,unsigned address) {
		using B=PioneerNativeBfpBandLayout;
		if(!B::Width(width) || (address&3))return 0;
		if(address>=B::Weight-16 && address<B::WeightEnd(width)) {
			const unsigned relative=address-(B::Weight-16),tile=relative/B::WeightStride,within=relative%B::WeightStride;
			if(within<16)return 0;
			uint32_t word=0;for(unsigned i=0;i<4;i++)word|=uint32_t(WeightByte(tile,within-16+i))<<(i*8);
			return word;
		}
		if(address>=B::Input-16 && address<B::InputEnd(width)) {
			const unsigned relative=address-(B::Input-16),tile=relative/B::InputStride,within=relative%B::InputStride;
			if(within<16)return 0;
			const unsigned first=(within-16)/2;
			return uint32_t(InputBits(tile,first))|(uint32_t(InputBits(tile,first+1))<<16);
		}
		return 0;
	}
};
#endif
