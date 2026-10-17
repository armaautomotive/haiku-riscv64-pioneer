/* SPDX-License-Identifier: MIT */
#define PIONEER_GRAPH_FAST_DIVIDE 1
#include "../PioneerGraphMath.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <initializer_list>
static uint32_t state=0x8375ab69;
static uint32_t Next() { state^=state<<13; state^=state>>17; state^=state<<5; return state; }
static unsigned accepted=0,rejected=0;
static void Check(float a,float b)
{
	uint32_t expected=PioneerAttentionBits(a/b); float output=0;
	bool fast=PioneerGraphDivideNormalFast(a,b,output); fast?accepted++:rejected++;
	if (fast && PioneerAttentionBits(output)!=expected) {
		printf("FAST DIV %08x %08x actual%08x expected%08x\n",PioneerAttentionBits(a),PioneerAttentionBits(b),PioneerAttentionBits(output),expected); fflush(stdout);
	}
	assert(!fast || PioneerAttentionBits(output)==expected);
	float actual=PioneerGraphDividePositive(a,b);
	assert(isnan(a/b)?isnan(actual):PioneerAttentionBits(actual)==expected);
}
int main()
{
	for (unsigned i=0;i<4000000;i++) {
		uint32_t a=Next()&0x7fffffff,b=Next()&0x7fffffff;
		if (a<0x7f800000 && b<0x7f800000) Check(PioneerAttentionFloat(a),PioneerAttentionFloat(b));
	}
	for (unsigned exponent=0;exponent<255;exponent++) for (unsigned low:{0U,1U,2U,0x3fffffU,0x7ffffeU,0x7fffffU})
		for (unsigned denominator:{0U,1U,0x00800000U,0x00800001U,0x3f000000U,0x3f7fffffU,0x3f800000U,0x3f800001U,0x40000000U,0x42fe0000U,0x7f7fffffU})
			Check(PioneerAttentionFloat((exponent<<23)|low),PioneerAttentionFloat(denominator));
	for (unsigned i=0;i<2000000;i++) { float a=float(Next()%1000001)*.000137f,b=1.0f+float(Next()%1000001)*.00013f; Check(a,b); Check(1,b); Check(a,127); }
	assert(accepted>5000000 && rejected>1000000);
	printf("PASS OFFLINE exact division proposal+integer RNE:accepted%u old exact fallback%u,all exponent boundaries/zeros/subnormals/overflow/model ranges;NOT instruction/native qualification\n",accepted,rejected);
}
