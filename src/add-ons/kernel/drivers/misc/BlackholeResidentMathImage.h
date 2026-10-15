/* SPDX-License-Identifier: MIT */
#ifndef BLACKHOLE_RESIDENT_MATH_IMAGE_H
#define BLACKHOLE_RESIDENT_MATH_IMAGE_H
#include "BlackholeTensixTileData.h"

// Original fixed TRISC0 program: two dense BF16 fixtures, inputs fetched by
// the device from a single fixed GDDR block. L1 input tables start poisoned.
// No general code/address/descriptor upload interface.
enum {
	TT_RESIDENT_MATH_CODE_WORDS=1024,
	TT_RESIDENT_MATH_IMAGE_WORDS=1+TT_RESIDENT_MATH_CODE_WORDS+256+3,
	TT_RESIDENT_MATH_STATUS_INDEX=1+TT_RESIDENT_MATH_CODE_WORDS+256,
	TT_RESIDENT_MATH_GDDR_WORDS=272,
	TT_RESIDENT_MATH_FAULT_PC=0x70aa4
};
static inline uint32_t BlackholeResidentMathAddress(unsigned index)
{
	if (index==0) return 0x6000;
	if (index<=TT_RESIDENT_MATH_CODE_WORDS) return 0x70000+(index-1)*4;
	index-=1+TT_RESIDENT_MATH_CODE_WORDS;
	if (index<256) return 0x17f800+index*4;
	index-=256;
	return index<3?0x17ffc0+index*4:UINT32_MAX;
}
static inline uint32_t BlackholeResidentMathImageWord(unsigned index)
{
	uint32_t address=BlackholeResidentMathAddress(index);
	if (address==0x70270) return 0x7d00006f;
	if (address>=0x70a40 && address<0x70ab0) {
		const uint32_t fetch[]={0xffb20ab7,0x040aa283,0x04029263,0x208aab03,
			0x001b0b13,0x00100293,0x045aa023,0x040aa283,0x0002f293,0x00010bb7,
			0x040aa283,0x00029c63,0x208aa283,0x01629863,0x240aa283,0x0ff2f293,
			0x02028463,0xfffb8b93,0xfe0b90e3,0x4e4f42b7,0x34628293,
			0x0059a423,0x0089a283,0x0002f293,0x0ff0000f,0x0000006f,
			0x0ff0000f,0x995ff06f};
		return fetch[(address-0x70a40)/4];
	}
	if (index<=TT_UNPACK_CODE_WORDS) {
		tt_tensix_tile_data poison={}; poison.version=1;
		return BlackholeTileDataImageWord(index,poison);
	}
	return 0;
}
static inline uint32_t BlackholeResidentMathGddrWord(unsigned word)
{
	if (word>=TT_RESIDENT_MATH_GDDR_WORDS) return 0;
	uint32_t address=0x70500+word*4;
	if (address<0x70700) {
		unsigned element=(address-0x70500)/2;
		uint32_t low=BlackholeFixedIntegerFp32(BlackholeFixedVector(element/128,(element%128)/16,element%16))>>16;
		uint32_t high=BlackholeFixedIntegerFp32(BlackholeFixedVector((element+1)/128,((element+1)%128)/16,(element+1)%16))>>16;
		return low|(high<<16);
	}
	if (address>=0x70710 && address<0x70910) {
		unsigned element=(address-0x70710)/2;
		uint32_t low=BlackholeFixedIntegerFp32(BlackholeFixedWeight(element/16,element%16,true))>>16;
		uint32_t high=BlackholeFixedIntegerFp32(BlackholeFixedWeight((element+1)/16,(element+1)%16,true))>>16;
		return low|(high<<16);
	}
	return 0;
}
static inline uint32_t BlackholeResidentMathCommandWord(unsigned word)
{
	const uint32_t command[]={0x04000000,0,18|(12<<6),0x70500,0,1|(2<<6),0,0,0x440,0};
	return word<10?command[word]:UINT32_MAX;
}
#endif
