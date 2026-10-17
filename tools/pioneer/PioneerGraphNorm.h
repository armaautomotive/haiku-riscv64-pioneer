/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_GRAPH_NORM_H
#define PIONEER_GRAPH_NORM_H
#include "PioneerGraphMath.h"
// OFFLINE decoder prerequisite ONLY. No driver/backend operation uses it.
// Integer restoring square root,24 digit pairs and one exact RNE decision.
// No fsqrt/fdiv,double instructions,integer64 division or runtime helpers.
static inline float PioneerGraphSqrtPositive(float value)
{
	uint32_t bits=PioneerAttentionBits(value),absolute=bits&0x7fffffff;
	if (!absolute) return value;
	if ((bits&0x80000000) || absolute>0x7f800000) return PioneerAttentionFloat(0x7fc00000);
	if (absolute==0x7f800000) return value;
	unsigned m=bits&0x7fffff; int e=int(bits>>23)-127;
	if (bits>>23) m|=0x800000; else { e=-126; while (!(m&0x800000)) { m<<=1; e--; } }
	if (unsigned(e)&1) { m<<=1; e--; }
	uint32_t low=m<<23,high=m>>9,remainder=0,root=0;
	for (int pair=23;pair>=0;pair--) {
		unsigned digit=pair>=16?(high>>((pair-16)*2))&3:(low>>(pair*2))&3;
		remainder=(remainder<<2)|digit; root<<=1;
		uint32_t trial=(root<<1)|1;
		if (remainder>=trial) { remainder-=trial; root++; }
	}
	// N-root^2 is integral. The midpoint square is root^2+root+1/4,
	// so no half-way tie can occur for an IEEE binary32 input here.
	if (remainder>root) root++;
	// e is now even;the qualified RV32 arithmetic shift is exact. At -Os
	// a signed /2 otherwise becomes an unqualified hardware DIV instruction.
	int exponent=(e>>1)+127;
	if (root==0x1000000) { root>>=1; exponent++; }
	return PioneerAttentionFloat((uint32_t(exponent)<<23)|(root&0x7fffff));
}
// Bounded Qwen head/hidden RMS normalization. Product rounding matches the
// pinned CPU's float square before its double reduction. The two-float
// expansion and power-of-two mean avoid unsupported double instructions.
// This remains OFFLINE until actual scalar instructions/full models pass.
static inline bool PioneerGraphRmsNorm(unsigned n,const float* input,float epsilon,float* output)
{
	if ((n!=128 && n!=1024) || !input || !output || (PioneerAttentionBits(epsilon)&0x80000000)
		|| (PioneerAttentionBits(epsilon)&0x7f800000)==0x7f800000) return false;
	PioneerAttentionWide sum={0,0};
	for (unsigned i=0;i<n;i++) {
		uint32_t absolute=PioneerAttentionBits(input[i])&0x7fffffff;
		// Explicit finite/normal bounded domain,not silent BRISC denormal
		// flushing. Larger decoder coverage needs separate qualification.
		if (absolute>0x477fe000 || (absolute && absolute<0x26800000)) return false;
		float product=input[i]*input[i];
		sum=PioneerAttentionWideAdd(sum,{product,0});
	}
	float mean=(sum.high+sum.low)*(n==128?0.0078125f:0.0009765625f);
	float denominator=PioneerGraphSqrtPositive(mean+epsilon);
	if (!(denominator>0) || (PioneerAttentionBits(denominator)&0x7f800000)==0x7f800000) return false;
	float scale=PioneerGraphDividePositive(1,denominator);
	for (unsigned i=0;i<n;i++) output[i]=input[i]*scale;
	return true;
}
#endif
