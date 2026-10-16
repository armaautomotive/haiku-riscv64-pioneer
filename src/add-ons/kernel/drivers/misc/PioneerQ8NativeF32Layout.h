/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q8_NATIVE_F32_LAYOUT_H
#define PIONEER_Q8_NATIVE_F32_LAYOUT_H
#include "PioneerQ8CompactLayout.h"
// Distinct packing: 16 exact FP32 scales then 512 sign-magnitude bytes.
// The footprint and shape are unchanged, but this is NOT the FP16-scale
// native format. Never change packing within a prepared-code session.
static inline uint64_t PioneerQ8NativeF32ScaleByte(unsigned inputs,unsigned block,unsigned row)
{
	return (uint64_t(row/16)*(inputs/32)+block)*576+(row%16)*4;
}
static inline uint64_t PioneerQ8NativeF32ValueByte(unsigned inputs,unsigned k,unsigned row)
{
	return (uint64_t(row/16)*(inputs/32)+k/32)*576+64+(k%32)*16+row%16;
}
static inline uint32_t PioneerQ8HalfToFloatBits(uint16_t half)
{
	uint32_t sign=uint32_t(half&0x8000)<<16,exp=(half>>10)&31,m=half&1023;
	if (exp==31) return sign|0x7f800000|(m<<13); // callers refuse nonfinite scales
	if (exp) return sign|((exp+112)<<23)|(m<<13);
	if (!m) return sign;
	unsigned shifts=0; while (!(m&1024)) { m<<=1; shifts++; }
	return sign|((113-shifts)<<23)|((m&1023)<<13);
}
#endif
