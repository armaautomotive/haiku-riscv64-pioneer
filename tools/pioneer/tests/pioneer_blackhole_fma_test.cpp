/* SPDX-License-Identifier: MIT */
// OFFLINE arithmetic model, not a hardware/LLM performance test.
#include "../PioneerBlackholeFma.h"
#include "../PioneerFloat16.h"
#include "../PioneerAttentionMath.h"
#include <cmath>
#include <cstdio>
#include <initializer_list>
static uint32_t seed=0x12345678;
static uint32_t Random() { seed^=seed<<13; seed^=seed>>17; seed^=seed<<5; return seed; }
int main() {
	// Exercise FULL IEEE half subnormals too,not the older flushing matrix
	// converter. Every finite half pair has an exact normal-FP32 product.
	for (unsigned i=0;i<65536;i++) for (uint16_t b:{uint16_t(0),uint16_t(1),uint16_t(0x3ff),uint16_t(0x400),uint16_t(0x3c00),uint16_t(0x7bff),uint16_t(0x8000),uint16_t(0x8001),uint16_t(0xfbff)}) {
		if ((i&0x7c00)==0x7c00) continue;
		float a=PioneerAttentionHalf(uint16_t(i)),v=PioneerAttentionHalf(b);
		assert(PioneerFmaBits(PioneerBlackholeFma(a,v,0))==PioneerFmaBits(std::fma(a,v,0.0f)));
	}
	unsigned partialDiff=0,accurateDiff=0;
	for (unsigned i=0;i<1000000;i++) {
		uint16_t ah=uint16_t(Random()),bh=uint16_t(Random());
		if ((ah&0x7c00)==0x7c00 || (bh&0x7c00)==0x7c00) continue;
		float a=PioneerFloat16Value(ah),b=PioneerFloat16Value(bh);
		// A standalone finite half-scale product is exactly representable.
		float scale=PioneerBlackholeFma(a,b,0);
		assert(PioneerFmaBits(scale)==PioneerFmaBits(std::fma(a,b,0.0f)));
		int dot=int(Random()%1040385)-520192;
		float c=std::ldexp(float(int(Random()%1048576)-524288),int(Random()%80)-50);
		float actual=PioneerBlackholeFma(float(dot),scale,c),ieee=std::fma(float(dot),scale,c);
		assert(std::isfinite(actual)); partialDiff+=PioneerFmaBits(actual)!=PioneerFmaBits(ieee);
		float product=float(dot)*scale;
		for (float addend:{c,-product,-std::nextafter(product,INFINITY),-std::nextafter(product,-INFINITY)}) {
			if (std::fpclassify(addend)==FP_SUBNORMAL) continue; // Q8 domain stays normal
			float accurate=PioneerBlackholeAccurateFma(float(dot),scale,addend),expected=std::fma(float(dot),scale,addend);
			if (PioneerFmaBits(accurate)!=PioneerFmaBits(expected)) {
				if (accurateDiff<4) printf("CANDIDATE mismatch dot %d scale %08x c %08x expected %08x actual %08x\n",dot,PioneerFmaBits(scale),PioneerFmaBits(addend),PioneerFmaBits(expected),PioneerFmaBits(accurate));
				accurateDiff++;
			}
		}
		assert(PioneerFmaBits(PioneerBlackholeFma(float(dot),scale,0))==PioneerFmaBits(std::fma(float(dot),scale,0.0f)));
		assert(PioneerFmaBits(PioneerBlackholeFma(c,1,scale))==PioneerFmaBits(c+scale));
	}
	assert(partialDiff);
	printf("Accurate candidate mismatches: %u\n",accurateDiff); assert(!accurateDiff);
	printf("PASS: OFFLINE finite-Q8 Blackhole arithmetic model, standalone multiply/add IEEE agreement and %u partially-fused disagreements; NOT hardware/model validation\n",partialDiff);
}
