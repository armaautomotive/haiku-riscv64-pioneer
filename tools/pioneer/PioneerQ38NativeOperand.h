/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NATIVE_OPERAND_H
#define PIONEER_Q38_NATIVE_OPERAND_H
#include "PioneerQ38VectorOperand.h"
#ifdef PIONEER_Q38_WORK_SIGNED_MAG2
#include "PioneerQ38SignedMagnitude2.h"
#endif
// OFFLINE prerequisite ONLY;NO image/driver selects this experiment.
// Keep qualified integer ops for TC->SM;do NOT guess newer SFPCAST modes.
// Blackhole SFPSTORE mode5 can encode that SM value directly as Integer8:
// https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/TensixTile/TensixCoprocessor/SFPSTORE.md
// Use physical HIGH halves of the EXISTING initialized Dst32 rows64..79.
// LOW halves MUST remain zero. Thus existing MOVD2A/CSR/format/source-bank
// interpretation stays EXACTLY unchanged,not a guessed Dst16 MOV mode.
// Caller owes physical initialization,all lane writes,drain/MOV scheduling,
// source ownership and strict UNKNOWN retention. No address/NoC authority.
template<class E> static void PioneerQ38NativeSignMagnitude(E& e)
{
	PioneerSfpuMove(e,2,0); PioneerSfpuShift(e,2,-31,true);
	PioneerSfpuXor(e,0,2); PioneerSfpuMove(e,3,2);
	PioneerSfpuShift(e,3,-31); PioneerSfpuAdd(e,0,3);
	PioneerSfpuShift(e,2,31); PioneerSfpuXor(e,0,2);
}
template<class E>
#ifdef PIONEER_Q38_WORK_SIGNED_MAG2
__attribute__((noinline))
#endif
static bool PioneerQ38NativeOperand(E& e,unsigned type,unsigned panel
#ifdef PIONEER_Q38_WORK_SIGNED_MAG2
	,bool signedMagnitude2=false
#endif
)
{
	if ((type!=8 && type!=12 && type!=14) || panel>=(type==8?2U:16U)) return false;
	if (type==8) {
		PioneerSfpuShift(e,0,24); PioneerSfpuShift(e,0,-24,true);
#ifdef PIONEER_Q38_WORK_SIGNED_MAG2
		if(signedMagnitude2)PioneerQ38SignedMagnitude2(e);else
#endif
		PioneerQ38NativeSignMagnitude(e);
	} else if (type==12) {
		if (panel&2) PioneerSfpuShift(e,0,-4);
		PioneerSfpuLoad32(e,4,15); PioneerSfpuAnd(e,0,0,4);
	} else {
		if (panel&4) PioneerSfpuShift(e,0,-4);
		PioneerSfpuLoad32(e,4,15); PioneerSfpuAnd(e,0,0,4);
		switch (panel&6) {
			case 2:PioneerSfpuShift(e,1,-2);break;
			case 4:PioneerSfpuShift(e,1,-4);break;
			case 6:PioneerSfpuShift(e,1,-6);break;
		}
		PioneerSfpuLoad32(e,4,3); PioneerSfpuAnd(e,1,1,4); PioneerSfpuShift(e,1,4);
		PioneerSfpuXor(e,0,1); PioneerSfpuAddI(e,0,-32);
#ifdef PIONEER_Q38_WORK_SIGNED_MAG2
		if(signedMagnitude2)PioneerQ38SignedMagnitude2(e);else
#endif
		PioneerQ38NativeSignMagnitude(e);
	}
	return true;
}
static inline unsigned PioneerQ38NativeOperandCount(unsigned type,unsigned panel)
{
	if ((type!=8 && type!=12 && type!=14) || panel>=(type==8?2U:16U)) return 0;
	return type==8?10:type==12?2+unsigned(bool(panel&2)):15+unsigned(bool(panel&4))+unsigned(bool(panel&6));
}
static inline unsigned PioneerQ38NativeOperandRow(unsigned block)
{
	return block<16 && !(block&3)?128+block+(block>=8?8:0):~0U;
}
template<class E> static bool PioneerQ38NativeOperandStore(E& e,unsigned block,unsigned parity)
{
	unsigned row=PioneerQ38NativeOperandRow(block);
	if (row==~0U || (parity!=0 && parity!=2)) return false;
	e.Op(0x72050000|row|parity); return true;
}
#endif
