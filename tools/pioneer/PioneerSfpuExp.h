/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_SFPU_EXP_H
#define PIONEER_SFPU_EXP_H
#include "PioneerSfpuSilu.h"

// Offline QGW7 attention experiment. The caller supplies 32 independently
// range-reduced arguments in LReg0 and their powers-of-two in LReg1. This
// evaluates the existing degree-8, separate-rounding exp polynomial across
// all 32 SFPU lanes. It is intentionally not the more accurate degree-10
// PioneerAttentionExp implementation and is not Work/native selectable.
template<class E> static inline void PioneerSfpuExp32(E& e)
{
	static const float coefficients[]={1.0f/40320.0f,1.0f/5040.0f,1.0f/720.0f,
		1.0f/120.0f,1.0f/24.0f,1.0f/6.0f,0.5f,1,1};
	PioneerSfpuLoad32(e,2,PioneerAttentionBits(coefficients[0]));
	for(unsigned i=1;i<9;i++){
		PioneerSfpuLoad32(e,3,PioneerAttentionBits(coefficients[i]));
		e.Op(0x84020940); // temporary4 = RN(p2 * reduced0)
		e.Op(0x850a4320); // p2 = RN(temporary4 + coefficient3)
	}
	e.Op(0x84021940); // temporary4 = RN(p2 * factor1)
	PioneerSfpuMove(e,0,4); // result0 = exp(reduced0) * factor1
	e.Op(0x72030008); // SFPSTORE result0 to Dst rows 8..11
}

// Same range reduction as the already-tested fast SiLU polynomial. Inputs
// outside its safe central interval must use the caller's exact fallback.
static inline bool PioneerSfpuExpRange(float value,float& reduced,float& factor)
{
	return value<=0.0f && PioneerSfpuSiluRange(-value,reduced,factor);
}
#endif
