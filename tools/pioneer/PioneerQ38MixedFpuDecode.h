/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_FPU_DECODE_H
#define PIONEER_Q38_MIXED_FPU_DECODE_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(__FAST_MATH__)
#error Mixed Q6/Q8 FPU operand producer is offboard only
#endif
#include "PioneerQ38VectorAffine.h"
#include "PioneerSfpuDivide.h"
// NEW OFFBOARD instruction producer for the remaining original Q6_K/Q8_0
// formats. No UNPACK/Dst/source/route/owner/driver or Work authority.
// ALL32 lanes enabled,all input registers and metadata must be defined.
// Q6 coefficient1 is EXACT finite FP16*d signed int8 scale;Q8 coefficient1
// is the EXACT original finite FP16 scale expressed as FP32. Preparation
// does NOT receive weights or perform learned products. No clipping,
// sign-magnitude Q8 recoding,host dequantization or precision fallback.
// All nonzero operands/products are normal FP32;the smallest is2^-24.
// Q6 half(11bits)*scale(8bits)*quant(6bits) has <=24 significant bits;
// both original multiplication boundaries are EXACT. BF16-RNE is a NEW
// numerical recipe,not a claim of equivalence to old Work inference.
template<class E> static inline void PioneerQ38MixedFpuRound(E& e)
{
	// SAME explicit ties-even bias as the qualified Q4 producer. SFPSTORE
	// truncates;STOCHRND ties-away cannot replace this. Preserve signed0.
	PioneerSfpuMove(e,3,0);PioneerSfpuShift(e,3,-16);
	PioneerSfpuAnd(e,3,3,5);PioneerSfpuAdd(e,0,6);PioneerSfpuAdd(e,0,3);
	PioneerSfpuAnd(e,0,0,7);
}
template<class E> static inline void PioneerQ38Q6FpuPrepare(E& e)
{
	PioneerSfpuLoad32(e,2,15);PioneerSfpuLoad32(e,4,3);
	PioneerSfpuLoad32(e,5,1);PioneerSfpuLoad32(e,6,0x7fff);
	PioneerSfpuLoad32(e,7,0xffff0000);
}
// Fresh0=original ql byte,3=original qh byte (unsigned0..255).
// Group is the ORIGINAL32-value group within a128-value Q6 half. The
// signed coefficient changes EVERY16 values,not every32. Caller binds it.
// Output0=BF16-RNE FP32 bits;clobbers3 only,preserves1/2/4/5/6/7.
template<unsigned Group,class E> static inline void PioneerQ38Q6FpuDecode(E& e)
{
	static_assert(Group<4,"Original Q6 group must be0..3");
	if(Group>=2)PioneerSfpuShift(e,0,-4);
	PioneerSfpuAnd(e,0,0,2);
	if(Group)PioneerSfpuShift(e,3,-int(Group*2));
	PioneerSfpuAnd(e,3,3,4);PioneerSfpuShift(e,3,4);PioneerSfpuAdd(e,0,3);
	e.Op(0x90000000); // exact positive0..63 SM32 -> FP32
	PioneerSfpuLoad32(e,3,0xc2000000); // EXACT -32.0
	PioneerQ38VectorAffineAdd(e,0,3,0); // q in[-32,31],exact including+0
	PioneerQ38VectorAffineMul(e,1,0,0); // C9 negated=-0 preserves product sign
	PioneerQ38MixedFpuRound(e);
}
template<class E> static inline void PioneerQ38Q8FpuPrepare(E& e)
{
	PioneerSfpuLoad32(e,2,127);PioneerSfpuLoad32(e,4,0xc3000000); // -128.0
	PioneerSfpuLoad32(e,5,1);PioneerSfpuLoad32(e,6,0x7fff);
	PioneerSfpuLoad32(e,7,0xffff0000);
}
// Fresh0=the ORIGINAL unsigned TC8 byte,including128 (-128). Native
// INT8/SM7 interpretation is NOT used. low7 -128*bit7 is exact FP32.
// Output0=BF16-RNE FP32 bits;clobbers3 only,preserves1/2/4/5/6/7.
template<class E> static inline void PioneerQ38Q8FpuDecode(E& e)
{
	PioneerSfpuMove(e,3,0);PioneerSfpuShift(e,3,-7);PioneerSfpuAnd(e,0,0,2);
	e.Op(0x90000000);e.Op(0x90000330);
	e.Op(0x84043000);e.Op(0x8f000000); // exact low7+(-128.0)*bit7
	PioneerQ38VectorAffineMul(e,1,0,0);
	PioneerQ38MixedFpuRound(e);
}
#endif
