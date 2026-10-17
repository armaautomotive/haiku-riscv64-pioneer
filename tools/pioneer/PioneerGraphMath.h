/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_GRAPH_MATH_H
#define PIONEER_GRAPH_MATH_H
#include "PioneerAttentionMath.h"
// Card-side layer math plus independent offline/native fixture references.
// Never called for learned CPU arithmetic by a native inference fallback.
// BRISC has no qualified fdiv.s: positive IEEE binary32 division is done
// with bounded integer long division and one RNE (including subnormals).
static inline float PioneerGraphDividePositive(float a,float b)
{
	uint32_t aa=PioneerAttentionBits(a),bb=PioneerAttentionBits(b);
	if ((aa|bb)&0x80000000U) return PioneerAttentionFloat(0x7fc00000);
	if (aa>0x7f800000 || bb>0x7f800000 || (!aa && !bb) || (aa==0x7f800000 && bb==aa)) return PioneerAttentionFloat(0x7fc00000);
	if (!bb || aa==0x7f800000) return PioneerAttentionFloat(0x7f800000);
	if (!aa || bb==0x7f800000) return 0;
	uint32_t ma=aa&0x7fffff,mb=bb&0x7fffff;
	int ea=int(aa>>23),eb=int(bb>>23);
	if (ea) ma|=0x800000; else { ea=1; while (!(ma&0x800000)) { ma<<=1; ea--; } }
	if (eb) mb|=0x800000; else { eb=1; while (!(mb&0x800000)) { mb<<=1; eb--; } }
	int exponent=ea-eb+127;
	if (ma<mb) { ma<<=1; exponent--; }
	uint32_t remainder=ma-mb,mantissa=0x800000;
	for (int bit=22;bit>=0;bit--) {
		remainder<<=1;
		if (remainder>=mb) { remainder-=mb; mantissa|=1U<<bit; }
	}
	if (exponent<=0) {
		unsigned shift=unsigned(1-exponent); if (shift>24) return 0;
		uint32_t lower=mantissa&((1U<<shift)-1),halfway=1U<<(shift-1),result=mantissa>>shift;
		if (lower>halfway || (lower==halfway && (remainder || (result&1)))) result++;
		return PioneerAttentionFloat(result);
	}
	if (remainder*2>mb || (remainder*2==mb && (mantissa&1))) mantissa++;
	if (mantissa==0x1000000) { mantissa>>=1; exponent++; }
	if (exponent>=255) return PioneerAttentionFloat(0x7f800000);
	return PioneerAttentionFloat((uint32_t(exponent)<<23)|(mantissa&0x7fffff));
}
static inline float PioneerGraphExp(float value)
{
	if (value<=-80) return 0;
	if (value>=80) return PioneerAttentionFloat(0x7f800000);
	float absolute=PioneerAttentionFloat(PioneerAttentionBits(value)&0x7fffffff);
	uint32_t bits=PioneerAttentionBits(absolute*1.4426950408889634f);
	int exponent=int((bits>>23)&255)-127; unsigned rounded=0;
	if (exponent>=-1) {
		unsigned shift=unsigned(23-exponent);
		rounded=(((bits&0x7fffff)|0x800000)+(1U<<(shift-1)))>>shift;
	}
	int n=(PioneerAttentionBits(value)&0x80000000)?-int(rounded):int(rounded);
	PioneerAttentionWide offset=PioneerAttentionWideMultiply({float(n),0},{0.6931471824645996f,-1.904654323148236e-9f});
	PioneerAttentionWide r=PioneerAttentionWideAdd({value,0},{-offset.high,-offset.low});
	static const uint32_t c[11][2]={
		{0x3f800000,0},{0x3f800000,0},{0x3f000000,0},{0x3e2aaaab,0xb1aaaaab},
		{0x3d2aaaab,0xb0aaaaab},{0x3c088889,0xafeeeeef},{0x3ab60b61,0xae13e93f},
		{0x39500d01,0xac3fcbfd},{0x37d00d01,0xaabfcbfd},{0x3638ef1d,0x292ad8e6},{0x3493f27e,0xa808760a}
	};
	PioneerAttentionWide result={PioneerAttentionFloat(c[10][0]),PioneerAttentionFloat(c[10][1])};
	for (int i=9;i>=0;i--) result=PioneerAttentionWideAdd(PioneerAttentionWideMultiply(result,r),{PioneerAttentionFloat(c[i][0]),PioneerAttentionFloat(c[i][1])});
	return (result.high+result.low)*PioneerAttentionFloat(uint32_t(n+127)<<23);
}
static inline float PioneerGraphSilu(float value)
{
	uint32_t bits=PioneerAttentionBits(value),absolute=bits&0x7fffffff;
	if (absolute>=0x7f800000) return PioneerAttentionFloat(0x7fc00000);
	if (value<=-80) return PioneerAttentionFloat(0x80000000);
	float denominator=1.0f+PioneerGraphExp(-value);
	float result=PioneerGraphDividePositive(PioneerAttentionFloat(absolute),denominator);
	return PioneerAttentionFloat(PioneerAttentionBits(result)|(bits&0x80000000));
}
// Separate fast activation candidate: single-float degree8 range-reduced
// exp instead of the debug two-float polynomial. Numerical qualification
// stays external; this is NOT used for the qualified attention softmax.
static inline float PioneerGraphExpFast(float value)
{
	if (value<=-80) return 0;
	if (value>=80) return PioneerAttentionFloat(0x7f800000);
	float absolute=PioneerAttentionFloat(PioneerAttentionBits(value)&0x7fffffff);
	uint32_t bits=PioneerAttentionBits(absolute*1.4426950408889634f);
	int exponent=int((bits>>23)&255)-127; unsigned rounded=0;
	if (exponent>=-1) {
		unsigned shift=unsigned(23-exponent); rounded=(((bits&0x7fffff)|0x800000)+(1U<<(shift-1)))>>shift;
	}
	int n=(PioneerAttentionBits(value)&0x80000000)?-int(rounded):int(rounded);
	float nf=float(n);
	// The high constant's short mantissa makes nf*high exact here.
	float r=(value-nf*0.693359375f)-nf*(-0.00021219444005469058f);
	float p=1.0f/40320.0f;
	p=p*r+1.0f/5040.0f; p=p*r+1.0f/720.0f; p=p*r+1.0f/120.0f;
	p=p*r+1.0f/24.0f; p=p*r+1.0f/6.0f; p=p*r+0.5f; p=p*r+1.0f; p=p*r+1.0f;
	return p*PioneerAttentionFloat(uint32_t(n+127)<<23);
}
static inline float PioneerGraphSiluFast(float value)
{
	uint32_t bits=PioneerAttentionBits(value),absolute=bits&0x7fffffff;
	if (absolute>=0x7f800000) return PioneerAttentionFloat(0x7fc00000);
	if (value<=-80) return PioneerAttentionFloat(0x80000000);
	float result=PioneerGraphDividePositive(PioneerAttentionFloat(absolute),1.0f+PioneerGraphExpFast(-value));
	return PioneerAttentionFloat(PioneerAttentionBits(result)|(bits&0x80000000));
}
static inline int PioneerGraphRoundQ8(float value)
{
	uint32_t bits=PioneerAttentionBits(value),absolute=bits&0x7fffffff;
	if (absolute<0x3f000000) return 0;
	if (absolute>=0x43000000) return 1000; // explicit range refusal, not saturation
	int exponent=int(absolute>>23)-127;
	if (exponent<0) return bits&0x80000000?-1:1;
	unsigned shift=unsigned(23-exponent),mantissa=(absolute&0x7fffff)|0x800000;
	unsigned q=(mantissa+(1U<<(shift-1)))>>shift; // ties AWAY, matching pinned quantizer
	return bits&0x80000000?-int(q):int(q);
}
static inline bool PioneerGraphQuantize32(const float* input,uint32_t* operand,uint32_t& scale)
{
	if (!input || !operand) return false;
	uint32_t maximum=0;
	for (unsigned i=0;i<32;i++) {
		uint32_t absolute=PioneerAttentionBits(input[i])&0x7fffffff;
		if (absolute>=0x7f800000) return false;
		if (absolute>maximum) maximum=absolute;
	}
	float d=PioneerGraphDividePositive(PioneerAttentionFloat(maximum),127.0f);
	float inverse=maximum?PioneerGraphDividePositive(1.0f,d):0;
	uint16_t half=PioneerAttentionToHalf(d);
	if ((half&0x7c00)==0x7c00 || (PioneerAttentionBits(inverse)&0x7f800000)==0x7f800000) return false;
	scale=PioneerAttentionBits(PioneerAttentionHalf(half));
	for (unsigned i=0;i<16;i++) {
		int a=PioneerGraphRoundQ8(input[i*2]*inverse),b=PioneerGraphRoundQ8(input[i*2+1]*inverse);
		if (a<-128 || a>127 || b<-128 || b>127) return false;
		uint32_t low=0x4000U|(a<0?0x8000U|unsigned(-a):unsigned(a));
		uint32_t high=0x4000U|(b<0?0x8000U|unsigned(-b):unsigned(b));
		operand[i]=low|(high<<16);
	}
	return true;
}
#endif
