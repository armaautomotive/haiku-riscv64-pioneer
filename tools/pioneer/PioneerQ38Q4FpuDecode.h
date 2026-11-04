/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_Q4_FPU_DECODE_H
#define PIONEER_Q38_Q4_FPU_DECODE_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION)
#error Compressed Q4 FPU operand producer is not qualified for a driver or Work
#endif
#include "PioneerQ38VectorAffine.h"
#include "PioneerSfpuDivide.h"
// OFFBOARD vector instruction producer, NOT an execution/format selector.
// All32 lanes enabled. Prepare receives1=d,2=dmin as EXACT finite FP16
// values represented in FP32,3=original unsigned6-bit scale,4=minimum.
// It leaves1=d*scale and2=-(dmin*minimum), with separate exact products,
// plus immutable4=15,5=1,6=32767,7=0xffff0000 for this group.
// Reuse ONLY within that original32-value Q4_K group of the same rows.
// Caller must prove UNPACK/load provenance, domain, zero RWCs, ownership,
// register readiness, all metadata/lane definitions and final real drain.
// Nothing here reads scalar weights/activations, uses NoC/config/Dst, or
// stages a globally expanded model. No host learned products or fallback.
template<class E> static inline void PioneerQ38Q4FpuPrepare(E& e)
{
	e.Op(0x90000330);e.Op(0x90000440); // exact positive SM32 -> FP32
	// Independent products cover one another's first latency slot;the
	// following sign-constant load covers the second product's last slot.
	e.Op(0x86013912);e.Op(0x86024922);
	PioneerSfpuLoad32(e,7,0x80000000);PioneerSfpuXor(e,2,7);
	PioneerSfpuLoad32(e,4,15);PioneerSfpuLoad32(e,5,1);
	PioneerSfpuLoad32(e,6,0x7fff);PioneerSfpuLoad32(e,7,0xffff0000);
}
// Output0=FP32 bits with a BF16-RNE value in the high16 bits. All low16
// bits are zero, INCLUDING signed zeros. Actual SFPSTORE/BF16 truncates;
// SFPSTOCHRND nearest is ties-AWAY, not ties-EVEN. Use integer rounding
// instead. Domain: finite normal/zero Q4 values (magnitude below2^26),
// so no FP32/BF16 denormal, infinity, NaN or overflow handling is hidden.
// Preserves coefficient1/offset2 across independently loaded raw bytes.
// Constants4..7 MUST come from Prepare and remain unchanged. Clobbers0,3;
// does not change lane flags or predication.
// ISA pin4a73a712: SFPSTORE.md and SFPSTOCHRND_FloatFloat.md. This helper
// alone proves neither exact FP32 subtraction nor full-model quality.
template<class E> static inline void PioneerQ38Q4FpuRound(E& e)
{
	PioneerSfpuMove(e,3,0);PioneerSfpuShift(e,3,-16);
	PioneerSfpuAnd(e,3,3,5);PioneerSfpuAdd(e,0,6);PioneerSfpuAdd(e,0,3);
	PioneerSfpuAnd(e,0,0,7);
}
// Fresh0 is an original unsigned packed byte0..255, NOT native BFP4
// sign-magnitude. Input1/2 must come from Prepare for THIS nibble's group.
// A finite FP16 significand has <=11 bits. scale<=63 and q<=15 give
// <=21 significant bits: d*scale*q is EXACT in FP32. Therefore the fused
// multiply-subtract below removes NO original product rounding point.
// BF16 is a NEW numerical recipe, not lossless Q4 or existing Work math.
template<class E,bool Upper> static inline void PioneerQ38Q4FpuDecode(E& e)
{
	if(Upper)PioneerSfpuShift(e,0,-4);else PioneerSfpuAnd(e,0,0,4);
	e.Op(0x90000000); // exact nonnegative0..15 integer -> FP32
	e.Op(0x84010200);e.Op(0x8f000000); // MAD1*0+2;full2-cycle latency
	PioneerQ38Q4FpuRound(e);
}
#endif
