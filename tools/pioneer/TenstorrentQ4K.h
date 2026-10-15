/* SPDX-License-Identifier: MIT */
#ifndef TENSTORRENT_Q4K_H
#define TENSTORRENT_Q4K_H
#include <stdint.h>
#include <string.h>
#include <math.h>
// Independently implemented binary-layout facts, GGML Q4_K: 256 values in
// 144 bytes, half d/dmin, eight 6-bit scale/min pairs, four nibble planes.
// Reference: ggml-org/llama.cpp ggml-common.h and ggml-quants.c.
static inline float PioneerHalfFloat(uint16_t half)
{
	uint32_t sign=uint32_t(half&0x8000)<<16, exponent=(half>>10)&31, mantissa=half&1023, bits;
	if(exponent==0) {
		if(mantissa==0) bits=sign;
		else {
			unsigned shifts=0;
			while((mantissa&1024)==0) { mantissa<<=1; shifts++; }
			bits=sign|((113-shifts)<<23)|((mantissa&1023)<<13);
		}
	} else if(exponent==31) bits=sign|0x7f800000|(mantissa<<13);
	else bits=sign|((exponent+112)<<23)|(mantissa<<13);
	float value; memcpy(&value,&bits,4); return value;
}
static inline bool PioneerDecodeQ4K(const uint8_t* block,float output[256])
{
	float d=PioneerHalfFloat(uint16_t(block[0])|(uint16_t(block[1])<<8));
	float minimum=PioneerHalfFloat(uint16_t(block[2])|(uint16_t(block[3])<<8));
	if(!isfinite(d) || !isfinite(minimum)) return false;
	for(unsigned group=0;group<8;group++) {
		unsigned scale,min;
		if(group<4) { scale=block[4+group]&63; min=block[8+group]&63; }
		else {
			scale=(block[8+group]&15)|((block[group]>>6)<<4);
			min=(block[8+group]>>4)|((block[4+group]>>6)<<4);
		}
		float multiplier=d*scale,offset=minimum*min;
		for(unsigned column=0;column<32;column++) {
			unsigned packed=block[16+(group/2)*32+column];
			unsigned quant=(group&1) ? packed>>4 : packed&15;
			output[group*32+column]=multiplier*quant-offset;
		}
	}
	return true;
}
#endif
