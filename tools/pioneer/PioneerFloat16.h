/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_FLOAT16_H
#define PIONEER_FLOAT16_H
#include <stdint.h>
#include <string.h>
// IEEE normal FP16 round-to-nearest-even, with explicit subnormal flush.
// No caller is allowed to use a NaN/Inf result as a device operand.
static inline uint16_t PioneerFloat16(float value)
{
	uint32_t bits; memcpy(&bits,&value,4);
	uint16_t sign=uint16_t((bits>>16)&0x8000);
	unsigned exponent=(bits>>23)&255;
	if (exponent==255) return sign|0x7c00|((bits&0x7fffff)?0x200:0);
	if (exponent<112) return sign;
	if (exponent==112) {
		// The top subnormal interval can round into the minimum normal.
		return sign|((bits&0x7fffff)>=0x7fe000?0x400:0);
	}
	if (exponent>142) return sign|0x7c00;
	uint32_t rounded=(bits&0x7fffffff)+0xfff+((bits>>13)&1);
	return sign|uint16_t((rounded-0x38000000)>>13);
}
static inline float PioneerFloat16Value(uint16_t half)
{
	uint32_t bits=uint32_t(half&0x8000)<<16;
	unsigned exponent=(half>>10)&31;
	if (exponent==31) bits|=0x7f800000|uint32_t(half&1023)<<13;
	else if (exponent) bits|=uint32_t(exponent+112)<<23|uint32_t(half&1023)<<13;
	float value; memcpy(&value,&bits,4); return value;
}
#endif
