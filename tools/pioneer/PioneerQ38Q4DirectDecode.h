/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_Q4_DIRECT_DECODE_H
#define PIONEER_Q38_Q4_DIRECT_DECODE_H
#include "PioneerQ38NativeOperand.h"
// OFFLINE ONLY. The caller independently supplies one freshly UNPACKed
// Q4 panel, all enabled lanes, zero RWCs, and the same low-nibble mask4.
// A one-instruction decoder needs NO REPLAY:emit its constant instruction
// directly between the unchanged SFPLOAD and native SFPSTORE. Neither the
// register-only affine replay slots12..31 nor any memory/config/ownership
// scheduling changes. The existing caller still owes its final full drain.
template<class E,unsigned Block,unsigned Parity,bool Upper>
static __attribute__((always_inline)) inline bool PioneerQ38Q4DirectChunk(E& e)
{
	static_assert(Block<16 && !(Block&3) && (Parity==0 || Parity==2),"same fixed Q4 rows/columns");
	e.Op(0x700d0000|Block|Parity);
	if(Upper)PioneerSfpuShift(e,0,-4);else PioneerSfpuAnd(e,0,0,4);
	return PioneerQ38NativeOperandStore(e,Block,Parity);
}
template<class E,bool Upper>
static __attribute__((always_inline)) inline bool PioneerQ38Q4DirectChunks(E& e)
{
	return PioneerQ38Q4DirectChunk<E,0,0,Upper>(e) && PioneerQ38Q4DirectChunk<E,0,2,Upper>(e)
		&& PioneerQ38Q4DirectChunk<E,4,0,Upper>(e) && PioneerQ38Q4DirectChunk<E,4,2,Upper>(e)
		&& PioneerQ38Q4DirectChunk<E,8,0,Upper>(e) && PioneerQ38Q4DirectChunk<E,8,2,Upper>(e)
		&& PioneerQ38Q4DirectChunk<E,12,0,Upper>(e) && PioneerQ38Q4DirectChunk<E,12,2,Upper>(e);
}
template<class E> static __attribute__((noinline)) bool PioneerQ38Q4DirectDecode(E& e,unsigned panel)
{
	if(panel>=16)return false;
	return panel&2?PioneerQ38Q4DirectChunks<E,true>(e):PioneerQ38Q4DirectChunks<E,false>(e);
}
#endif
