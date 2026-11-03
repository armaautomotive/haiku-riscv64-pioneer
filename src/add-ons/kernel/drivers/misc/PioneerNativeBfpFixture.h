/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_BFP_FIXTURE_H
#define PIONEER_NATIVE_BFP_FIXTURE_H
#include <stdint.h>
// Fixed, asymmetric, finite synthetic DATA. Integer layout construction only;
// no original model, CPU learned products, new conversion or numerical oracle.
struct PioneerNativeBfpFixture {
	static constexpr unsigned WeightBytes(unsigned bits) { return bits==4?576:bits==8?1088:0; }
	static unsigned Exponent(unsigned row) { return 126+row%3; }
	static unsigned Magnitude(unsigned bits,unsigned row,unsigned k) { return (row*37+k*19+11)&(bits==4?7:127); }
	static bool Negative(unsigned row,unsigned k) { return ((row*3+k*5)&3)==2; }
	static unsigned Encoded(unsigned bits,unsigned element) {
		const unsigned face=element/256,row=(face/2)*16+(element%256)/16,k=(face%2)*16+element%16;
		const unsigned magnitude=Magnitude(bits,row,k);
		return magnitude?(magnitude|(Negative(row,k)?1U<<(bits-1):0)):0;
	}
	static uint8_t WeightByte(unsigned bits,unsigned index) {
		if(!WeightBytes(bits) || index>=WeightBytes(bits))return 0;
		if(index<64)return uint8_t(Exponent((index/32)*16+index%16));
		index-=64;
		return uint8_t(bits==4?Encoded(bits,index*2)|(Encoded(bits,index*2+1)<<4):Encoded(bits,index));
	}
	static uint32_t WeightWord(unsigned bits,unsigned word) {
		uint32_t value=0;for(unsigned i=0;i<4;i++)value|=uint32_t(WeightByte(bits,word*4+i))<<(i*8);
		return value;
	}
	static uint16_t InputBits(unsigned k) { return uint16_t(0x3f00+(k*13)%256+((k&3)==2?0x8000:0)); }
	static uint32_t InputWord(unsigned word) {
		const unsigned datum=word*2,k=(datum/128)*16+datum%16;
		return uint32_t(InputBits(k))|(uint32_t(InputBits(k+1))<<16);
	}
};
#endif
