/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NATIVE_RAW_OPERAND_H
#define PIONEER_Q38_NATIVE_RAW_OPERAND_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION)
#error Native raw-code recovery is an unqualified offboard prerequisite, not Work or a driver
#endif
#include "PioneerQ38VectorOperand.h"
// Input LReg0: EXACT FP32 bits obtained from constant-exponent127 native
// BFP2/BFP4/BFP8 unpack -> MOVA2D -> SFPLOAD, ALL32 lanes enabled/ready.
// The caller must prove that chain, canonical source data, bank ownership,
// Dst layout and scheduling. This helper proves NONE of those invariants.
// Output0: original UNSIGNED2/4/8-bit code, not requantized floating weights.
// All bit patterns are preserved, INCLUDING sign1/magnitude0, whose unpacked
// negative-infinity bits identify the otherwise missing raw code2/8/128.
// Never perform floating arithmetic on those intermediate opaque bits.
// Clobbers0,1,2,3,7;preserves4,5,6;NO flags/FP/config/Dst/NoC/host products.
// Primary facts (no vendor code copied): ISA4a73a712 SFPSHFT and
// FloatBitPatterns.md#bfp. Register shifts use VC as AMOUNT, VD as DATA,
// Mod1=0;this is distinct from the already used immediate-shift modifier.
template<class E> static inline bool PioneerQ38NativeRawRecoverBaseline(E& e,unsigned bits)
{
	if(bits!=2 && bits!=4 && bits!=8)return false;
	PioneerSfpuMove(e,2,0); // preserve original sign and special exponent bit
	PioneerSfpuMove(e,1,0);PioneerSfpuShift(e,1,-23);
	PioneerSfpuLoad32(e,7,7);PioneerSfpuAnd(e,1,1,7);
	PioneerSfpuAddI(e,1,int(bits)-32); // shifts stay in[-30,-17], even zero/Inf
	PioneerSfpuLoad32(e,7,0x007fffff);PioneerSfpuAnd(e,0,0,7);
	PioneerSfpuLoad32(e,7,0x00800000);PioneerSfpuAdd(e,0,7);
	e.Op(0x7a000100); // SFPSHFT VC1(amount),VD0(data/result),Mod1=0,Imm12=0
	PioneerSfpuMove(e,3,2);PioneerSfpuShift(e,3,-30);
	PioneerSfpuLoad32(e,7,1);PioneerSfpuAnd(e,3,3,7);PioneerSfpuAddI(e,3,-1);
	PioneerSfpuAnd(e,0,0,3); // negative Inf has zero magnitude, retains raw sign
	PioneerSfpuShift(e,2,-31);PioneerSfpuShift(e,2,int(bits)-1);PioneerSfpuAdd(e,0,2);
	return true;
}
// Same contract, fewer instructions. SFPEXEXP's unbiased signed result
// plus(bits-25) supplies the shift directly. Native zero then shifts the
// hidden bit just past the magnitude field; the magnitude mask removes it.
// Negative Inf shifts it out of32b entirely, keeping its raw sign code.
// SHFT's documented signed-amount/low5 shift behavior is essential here.
// SFPEXEXP Mod1=0 neither changes lane flags nor disables exponent debias;
// SFPEXMAN Mod1=0 pads the hidden1 even for zero/Inf intermediate bits.
template<class E> static inline bool PioneerQ38NativeRawRecover(E& e,unsigned bits)
{
	if(bits!=2 && bits!=4 && bits!=8)return false;
	e.Op(0x77000010); // SFPEXEXP VC0,VD1,Mod1=0; signed unbiased exponent
	PioneerSfpuAddI(e,1,int(bits)-25);
	e.Op(0x78000030); // SFPEXMAN VC0,VD3,Mod1=0; hidden1+mantissa
	e.Op(0x7a000130); // SFPSHFT VC1(amount),VD3(data/result),Mod1=0
	PioneerSfpuLoad32(e,7,(1U<<(bits-1))-1);PioneerSfpuAnd(e,3,3,7);
	PioneerSfpuShift(e,0,-31);PioneerSfpuShift(e,0,int(bits)-1);PioneerSfpuAdd(e,0,3);
	return true;
}
#endif
