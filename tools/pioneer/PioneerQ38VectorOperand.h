/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_VECTOR_OPERAND_H
#define PIONEER_Q38_VECTOR_OPERAND_H
#include "PioneerSfpuDivide.h"
// OFFLINE arithmetic prerequisite ONLY;NO installed driver selects this helper.
// ALL32 lanes enabled;LReg0=UNSIGNED raw low byte,LReg1=unsigned Q6 high
// byte. The future caller owns/proves UNPACK-to-Dst,disjoint Dst16/Dst32
// windows,load/store/move scheduling and exact same source panel indices.
// Output0 is OPAQUE Dst32 bits whose high16,after MOVD2A's FP16 shuffle,
// equal the original lossless signed integer8 overlay. No floating-point
// approximation,CAST/rounding/conditional flags,NoC/CSR or host products.
// SFPIADD/SFPSHFT/AND/XOR/LOADI/MOV encodings are shared with the existing
// strict SFPU emitter. ISA pin8a34e25e:Dst,SFPLOAD,SFPSTORE,SFPSHFT,SFPIADD.
template<class E> static inline void PioneerQ38VectorSignedOverlay(E& e)
{
	PioneerSfpuMove(e,2,0); PioneerSfpuShift(e,2,-31,true); // TC sign mask
	PioneerSfpuXor(e,0,2);
	PioneerSfpuMove(e,3,2); PioneerSfpuShift(e,3,-31); // 1 if negative
	PioneerSfpuAdd(e,0,3); // absolute magnitude,including original -128
	PioneerSfpuShift(e,2,31); PioneerSfpuShift(e,0,21);
	PioneerSfpuXor(e,0,2); // disjoint sign/magnitude fields
	PioneerSfpuLoad32(e,4,0x00100000); PioneerSfpuXor(e,0,4); // fixed raw exponent16
}
template<class E>
#if defined(PIONEER_Q38_STATIC_VECTOR) && defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("O2")))
#endif
static bool PioneerQ38VectorOperand(E& e,unsigned type,unsigned panel)
{
	if ((type!=8 && type!=12 && type!=14) || panel>=(type==8?2U:16U)) return false;
	if (type==8) {
		PioneerSfpuShift(e,0,24); PioneerSfpuShift(e,0,-24,true); // unsigned byte -> exact signed TC8
		PioneerQ38VectorSignedOverlay(e);
	} else if (type==12) {
		if (panel&2) PioneerSfpuShift(e,0,-4);
		PioneerSfpuLoad32(e,4,15); PioneerSfpuAnd(e,0,0,4);
		PioneerSfpuShift(e,0,21); PioneerSfpuLoad32(e,4,0x00100000); PioneerSfpuXor(e,0,4);
	} else {
		if (panel&4) PioneerSfpuShift(e,0,-4);
		PioneerSfpuLoad32(e,4,15); PioneerSfpuAnd(e,0,0,4);
#ifdef PIONEER_Q38_STATIC_VECTOR
		// Keep the SAME shift, but a constant opcode on each branch can use
		// direct Tensix issue rather than a dynamic instruction-buffer store.
		switch (panel&6) {
			case 2:PioneerSfpuShift(e,1,-2);break;
			case 4:PioneerSfpuShift(e,1,-4);break;
			case 6:PioneerSfpuShift(e,1,-6);break;
		}
#else
		if (panel&6) PioneerSfpuShift(e,1,-int(panel&6));
#endif
		PioneerSfpuLoad32(e,4,3); PioneerSfpuAnd(e,1,1,4); PioneerSfpuShift(e,1,4);
		PioneerSfpuXor(e,0,1); PioneerSfpuAddI(e,0,-32);
		PioneerQ38VectorSignedOverlay(e);
	}
	return true;
}
#endif
