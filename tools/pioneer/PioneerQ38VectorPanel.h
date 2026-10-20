/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_VECTOR_PANEL_H
#define PIONEER_Q38_VECTOR_PANEL_H
#include "PioneerQ38VectorOperand.h"
// OFFLINE fixed-layout prerequisite. NO installed image/handler selects it.
// The raw planes each contain sixteen consecutive16-byte row slices from
// the original lossless tiled block. These are DATA OFFSETS,not addresses.
struct PioneerQ38VectorPanelSpan { unsigned low=0,high=0; bool second=false; };
static inline bool PioneerQ38VectorPanel(unsigned type,unsigned panel,PioneerQ38VectorPanelSpan& result)
{
	if ((type!=8 && type!=12 && type!=14) || panel>=(type==8?2U:16U)) return false;
	PioneerQ38VectorPanelSpan s;
	if (type==8) s.low=(2+panel*16)*16;
	else if (type==12) s.low=(16+(panel/4)*32+(panel%2)*16)*16;
	else {
		unsigned half=panel/8,plane=(panel%8)/2;
		s.low=(half*64+(plane%2)*32+(panel%2)*16)*16;
		s.high=(128+half*32+(panel%2)*16)*16; s.second=true;
	}
	unsigned bytes=type==8?576:type==12?2304:3392;
	if ((s.low&15) || s.low>bytes-256 || (s.second && ((s.high&15) || s.high>bytes-256))) return false;
	result=s; return true;
}
// Future caller has ALREADY completed raw unsigned UNPACK-to-Dst and
// handed SrcA to MatrixUnit,restored explicit FP16 source interpretation,
// and reset all address RWCs. No host/NoC/CSR/config/reset accesses here.
// This exact sequence is software-tested but has NO native image/authority.
#ifdef PIONEER_Q38_STATIC_VECTOR
template<class E,unsigned Block,unsigned Parity> static __attribute__((always_inline)) inline
bool PioneerQ38VectorChunk(E& e,unsigned type,unsigned panel,bool second)
{
	static_assert(Block<16 && !(Block&3) && (Parity==0 || Parity==2),"fixed owned row/column selectors");
	e.Op(0x700d0000|Block|Parity);
	if (second) e.Op(0x701d0000|(16+Block)|Parity);
	if (!PioneerQ38VectorOperand(e,type,panel)) return false;
	e.Op(0x72070000|(64+Block)|Parity); return true;
}
#endif
template<class E> static bool PioneerQ38VectorTransform(E& e,unsigned type,unsigned panel)
{
	PioneerQ38VectorPanelSpan span; if (!PioneerQ38VectorPanel(type,panel,span)) return false;
#ifdef PIONEER_Q38_STATIC_VECTOR
	// SAME exact lane order and values; compile-time addresses eliminate
	// dynamic instruction-buffer stores. No RWC/config/format changes.
	if (!PioneerQ38VectorChunk<E,0,0>(e,type,panel,span.second) || !PioneerQ38VectorChunk<E,0,2>(e,type,panel,span.second)
		|| !PioneerQ38VectorChunk<E,4,0>(e,type,panel,span.second) || !PioneerQ38VectorChunk<E,4,2>(e,type,panel,span.second)
		|| !PioneerQ38VectorChunk<E,8,0>(e,type,panel,span.second) || !PioneerQ38VectorChunk<E,8,2>(e,type,panel,span.second)
		|| !PioneerQ38VectorChunk<E,12,0>(e,type,panel,span.second) || !PioneerQ38VectorChunk<E,12,2>(e,type,panel,span.second)) return false;
	e.Op(0x08002040); e.Op(0x08082044); e.Op(0x08102048); e.Op(0x0818204c);
#else
	for (unsigned block=0;block<16;block+=4) for (unsigned parity=0;parity<4;parity+=2) {
		e.Op(0x700d0000|block|parity); // INT8_COMP keeps ALL10 magnitude bits,raw unsigned0..255
		if (span.second) e.Op(0x701d0000|(16+block)|parity);
		if (!PioneerQ38VectorOperand(e,type,panel)) return false;
		e.Op(0x72070000|(64+block)|parity); // LReg0 opaque32 HI16 into disjoint Dst32
	}
	// FPU automatically interlocks Dst RAW hazards. All output cells were
	// written above;caller still owes its explicit SrcA ownership proof.
	for (unsigned block=0;block<16;block+=4) e.Op(0x08002000|(block<<17)|(64+block));
#endif
	return true;
}
#endif
