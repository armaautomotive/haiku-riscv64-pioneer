/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BFLOAT_OPERAND_H
#define PIONEER_Q38_BFLOAT_OPERAND_H
#include "PioneerQ38VectorOperand.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerInt8Operand.h"
// OFFLINE ARITHMETIC PREREQUISITE ONLY. NOT an image/driver selection.
// These bounded integers are EXACT in BF16;no re-quantization or -128 clip.
// CAST is canonical SM32->FP32 RNE,then exact BF16 high16 has zero low16.
// A future caller owes FP32 SFPSTORE/MOVD2A BF16 format/ownership proofs,
// activation conversion,canonical integer zero,full-K/native qualification.
template<class E> static bool PioneerQ38BFloatOperand(E& e,unsigned type,unsigned panel)
{
	if ((type!=8 && type!=12 && type!=14) || panel>=(type==8?2U:16U)) return false;
	if (type==8) {
		PioneerSfpuShift(e,0,24); PioneerSfpuShift(e,0,-24,true);
	} else if (type==12) {
		if (panel&2) PioneerSfpuShift(e,0,-4);
		PioneerSfpuLoad32(e,4,15); PioneerSfpuAnd(e,0,0,4);
	} else {
		if (panel&4) PioneerSfpuShift(e,0,-4);
		PioneerSfpuLoad32(e,4,15); PioneerSfpuAnd(e,0,0,4);
		if (panel&6) PioneerSfpuShift(e,1,-int(panel&6));
		PioneerSfpuLoad32(e,4,3); PioneerSfpuAnd(e,1,1,4); PioneerSfpuShift(e,1,4);
		PioneerSfpuXor(e,0,1); PioneerSfpuAddI(e,0,-32);
	}
	PioneerSfpuMove(e,2,0); PioneerSfpuShift(e,2,-31,true);
	PioneerSfpuXor(e,0,2); PioneerSfpuMove(e,3,2); PioneerSfpuShift(e,3,-31);
	PioneerSfpuAdd(e,0,3); PioneerSfpuShift(e,2,31); PioneerSfpuXor(e,0,2);
	e.Op(0x90000000); e.Op(0x8f000000); // exact CAST plus explicit latency slot
	return true;
}
static inline bool PioneerQ38BFloatActivation(uint16_t input,uint16_t& output)
{
	if (!PioneerInt8OperandValid(input)) return false;
	int value=int(input&1023)*((input&0x8000)?-1:1);
	float exact=float(value); uint32_t bits; __builtin_memcpy(&bits,&exact,4);
	output=uint16_t(bits>>16); return true;
}
#endif
