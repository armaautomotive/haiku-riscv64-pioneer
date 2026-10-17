/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_SFPU_SILU_H
#define PIONEER_SFPU_SILU_H
#include "PioneerGraphMath.h"
#include "PioneerSfpuDivide.h"
// OFFLINE vector activation prerequisite. EXACT same range reduction and
// separate RN multiply/add polynomial as PioneerGraphExpFast,not SFPMAD
// reassociation. Tiny/extreme/zero lanes keep the existing card-side path.
static inline bool PioneerSfpuSiluRange(float gate,float& reduced,float& factor)
{
	uint32_t absolute=PioneerAttentionBits(gate)&0x7fffffff;
	if (absolute<0x0d800000 || absolute>=0x42a00000) { reduced=0; factor=1; return false; }
	float value=-gate,a=PioneerAttentionFloat(absolute);
	uint32_t bits=PioneerAttentionBits(a*1.4426950408889634f);
	int exponent=int((bits>>23)&255)-127; unsigned rounded=0;
	if (exponent>=-1) { unsigned shift=unsigned(23-exponent); rounded=(((bits&0x7fffff)|0x800000)+(1U<<(shift-1)))>>shift; }
	int n=(PioneerAttentionBits(value)&0x80000000)?-int(rounded):int(rounded); float nf=float(n);
	reduced=(value-nf*0.693359375f)-nf*(-0.00021219444005469058f);
	factor=PioneerAttentionFloat(uint32_t(n+127)<<23); return true;
}
// LReg0=reduced,1=power-of-two factor;result0=1+exp(-gate).
// SFPMAD(p,r,+0) rounds the NORMAL product,then SFPADD rounds the sum,
// exactly matching -ffp-contract=off scalar arithmetic. A zero product's
// sign cannot affect the following strictly positive coefficient addition.
template<class E> static void PioneerSfpuSiluDenominator(E& e)
{
	static const float coefficients[]={1.0f/40320.0f,1.0f/5040.0f,1.0f/720.0f,1.0f/120.0f,1.0f/24.0f,1.0f/6.0f,0.5f,1,1};
	PioneerSfpuLoad32(e,2,PioneerAttentionBits(coefficients[0]));
	for (unsigned i=1;i<9;i++) {
		PioneerSfpuLoad32(e,3,PioneerAttentionBits(coefficients[i]));
		e.Op(0x84020940); // temporary4 = RN(p2 * reduced0)
		e.Op(0x850a4320); // p2 = RN(temporary4 + coefficient3)
	}
	e.Op(0x84021940); // temporary4 = RN(p2 * factor1)
	PioneerSfpuLoad32(e,3,0x3f800000);
	e.Op(0x850a4300); // result0 = RN(temporary4 + 1)
}
#endif
