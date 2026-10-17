/* SPDX-License-Identifier: MIT */
#include "../PioneerGraphFmaFast.h"
#include <cassert>
#include <cmath>
#include <cstdio>
static uint32_t state=0x635ca79b;
static uint32_t Next() { state^=state<<13; state^=state>>17; state^=state<<5; return state; }
static unsigned accepted=0,rejected=0;
static void Check(float a,float b,float c)
{
	uint32_t expected=PioneerAttentionBits(fmaf(a,b,c)); float output=0;
	bool fast=PioneerGraphFmaCompensated(a,b,c,output); fast?accepted++:rejected++;
	if (fast && PioneerAttentionBits(output)!=expected) {
		printf("FAST FMA %08x %08x %08x actual%08x expected%08x\n",PioneerAttentionBits(a),PioneerAttentionBits(b),PioneerAttentionBits(c),PioneerAttentionBits(output),expected); fflush(stdout);
	}
	assert(!fast || PioneerAttentionBits(output)==expected);
	assert(PioneerAttentionBits(PioneerGraphFmaHybrid(a,b,c))==expected);
}
int main()
{
	for (unsigned bit=0;bit<64;bit++) {
		PioneerGraphU64 power=PioneerGraphU64(1)<<bit;
		assert(PioneerGraphHighest(power)==bit && PioneerGraphHighest(power|(power-1))==bit);
		if (bit) assert(PioneerGraphHighest(power-1)==bit-1);
	}
	for (unsigned i=0;i<2000000;i++) {
		PioneerGraphU64 value=(PioneerGraphU64(Next())<<32)|Next();
		if (value) { unsigned bit=0; for (auto v=value;v>>1;v>>=1) bit++; assert(PioneerGraphHighest(value)==bit); }
		float a=PioneerAttentionFloat(Next()),b=PioneerAttentionFloat(Next()),c=PioneerAttentionFloat(Next());
		if (isfinite(a) && isfinite(b) && isfinite(c)) Check(a,b,c);
	}
	for (unsigned i=0;i<2000000;i++) {
		float a=PioneerAttentionFloat((Next()&0x807fffff)|((Next()%36+107)<<23));
		float b=PioneerAttentionFloat((Next()&0x807fffff)|((Next()%30+97)<<23));
		float c=PioneerAttentionFloat((Next()&0x807fffff)|((Next()%66+77)<<23));
		Check(a,b,c); float cancel=-(a*b); Check(a,b,cancel); Check(a,b,nextafterf(cancel,INFINITY)); Check(a,b,nextafterf(cancel,-INFINITY));
	}
	assert(accepted>1000000 && rejected>1000000);
	printf("PASS OFFLINE hybrid exact FMA:arbitrary finite/bounded/cancellation cases,accepted%u integer fallback%u;NOT scalar instruction/native qualification\n",accepted,rejected);
}
