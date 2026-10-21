/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BINARY_SCALE_H
#define PIONEER_Q38_BINARY_SCALE_H
#include "PioneerQwen38Panel.h"
// Exact half*small-integer FP32 bits,like the qualified scalar helper,but
// normalize with at most FIVE bounded steps instead of up to23 loop trips.
// No float multiplication/rounding,subnormal flush,CLZ extension or libc.
#if defined(PIONEER_Q38_PREDECODED_Q4_STICKY) && defined(__GNUC__) && !defined(__clang__)
__attribute__((optimize("Os")))
#endif
static inline bool PioneerQ38BinaryScale(uint16_t half,int factor,uint32_t& output)
{
	unsigned exponent=(half>>10)&31;
	if (exponent==31 || factor<-128 || factor>127) return false;
	uint32_t sign=(uint32_t(half&0x8000)<<16)^(factor<0?0x80000000U:0);
	unsigned magnitude=exponent?1024+(half&1023):half&1023;
	uint32_t product=magnitude*unsigned(factor<0?-factor:factor);
	if (!product) { output=sign; return true; }
	int encoded=exponent?int(exponent)-25+150:-24+150;
	if (product<0x100) { product<<=16; encoded-=16; }
	if (product<0x10000) { product<<=8; encoded-=8; }
	if (product<0x100000) { product<<=4; encoded-=4; }
	if (product<0x400000) { product<<=2; encoded-=2; }
	if (product<0x800000) { product<<=1; encoded--; }
	output=sign|(uint32_t(encoded)<<23)|(product&0x7fffff); return true;
}
#endif
