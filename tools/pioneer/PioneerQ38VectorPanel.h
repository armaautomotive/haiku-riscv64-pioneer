/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_VECTOR_PANEL_H
#define PIONEER_Q38_VECTOR_PANEL_H
#include "PioneerQ38VectorOperand.h"
#ifdef PIONEER_Q38_WORK_Q4_DIRECT_DECODE
#if !defined(PIONEER_Q38_WORK_Q4_NIBBLE) || !defined(PIONEER_Q38_NATIVE_OPERAND_STORE) || !defined(PIONEER_Q38_WORK_DIRECT_SRCB) || !defined(PIONEER_Q38_REPLAY_OPERAND) || !defined(PIONEER_Q38_STATIC_VECTOR)
#error Direct Q4 decoder is a separate OFFLINE nibble/native-store/static Work peer only
#endif
#include "PioneerQ38Q4DirectDecode.h"
#endif
#ifdef PIONEER_Q38_WORK_RAW_UNPACK_CACHE
#include "PioneerQ38RawUnpackCache.h"
#endif
#ifdef PIONEER_Q38_NATIVE_OPERAND_STORE
#if !defined(PIONEER_Q38_STATIC_VECTOR) || !defined(PIONEER_Q38_REPLAY_OPERAND)
#error Native operand store requires the fixed static replayed vector peer
#endif
#include "PioneerQ38NativeOperand.h"
#endif
#ifdef PIONEER_Q38_REPLAY_OPERAND
#include "PioneerQ38OperandReplay.h"
#endif
#ifdef PIONEER_Q38_WORK_Q4_NIBBLE
#include "PioneerQ38Q4Nibble.h"
#if !defined(PIONEER_Q38_NATIVE_OPERAND_STORE) || !defined(PIONEER_Q38_REPLAY_OPERAND) || !defined(PIONEER_Q38_WORK_DIRECT_SRCB)
#error Q4 nibble decoding requires the separate original-layout native-store replay Work peer
#endif
#endif
#if defined(PIONEER_Q38_WORK_SIGNED_MAG2) && (!defined(PIONEER_Q38_NATIVE_OPERAND_STORE) || !defined(PIONEER_Q38_REPLAY_OPERAND) || !defined(PIONEER_Q38_WORK_DIRECT_SRCB))
#error Signed magnitude decoding requires the separate original-layout native-store replay Work peer
#endif
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
#ifdef PIONEER_Q38_PREPARED_Q4_AFFINE
	// Separate OFFLINE format:only Q4's original metadata prefix is
	// replaced. ALL packed quant bytes and per-panel nibble order match.
	if (type==12) s.low-=256;
#endif
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
bool PioneerQ38VectorChunk(E& e,unsigned type,unsigned panel,bool second
#ifdef PIONEER_Q38_WORK_SIGNED_MAG2
	,bool signedMagnitude2
#endif
#ifdef PIONEER_Q38_WORK_Q4_NIBBLE
	,bool q4Nibble
#endif
#ifdef PIONEER_Q38_WORK_RAW_UNPACK_CACHE
	,bool rawUpper
#endif
#ifdef PIONEER_Q38_REPLAY_OPERAND
	,unsigned replay
#endif
)
{
	static_assert(Block<16 && !(Block&3) && (Parity==0 || Parity==2),"fixed owned row/column selectors");
#ifdef PIONEER_Q38_WORK_RAW_UNPACK_CACHE
	if(rawUpper)e.Op(0x700d0010|Block|Parity);else
#endif
	e.Op(0x700d0000|Block|Parity);
	if (second) e.Op(0x701d0000|(16+Block)|Parity);
#ifdef PIONEER_Q38_REPLAY_OPERAND
	if (Block==0 && Parity==0) {
		e.Op(replay|3); // record AND execute, before exactly Count register ops
#ifdef PIONEER_Q38_NATIVE_OPERAND_STORE
#ifdef PIONEER_Q38_WORK_Q4_NIBBLE
		if(q4Nibble) { if(!PioneerQ38Q4NibbleDecode(e,panel))return false; }
		else
#endif
		if (!PioneerQ38NativeOperand(e,type,panel
#ifdef PIONEER_Q38_WORK_SIGNED_MAG2
			,signedMagnitude2
#endif
		)) return false;
#else
		if (!PioneerQ38VectorOperand(e,type,panel)) return false;
#endif
	} else e.Op(replay);
#else
	if (!PioneerQ38VectorOperand(e,type,panel)) return false;
#endif
#ifdef PIONEER_Q38_NATIVE_OPERAND_STORE
	return PioneerQ38NativeOperandStore(e,Block,Parity);
#else
	e.Op(0x72070000|(64+Block)|Parity); return true;
#endif
}
#endif
template<class E> static bool PioneerQ38VectorTransform(E& e,unsigned type,unsigned panel
#ifdef PIONEER_Q38_WORK_SIGNED_MAG2
	,bool signedMagnitude2Work=false
#endif
#ifdef PIONEER_Q38_WORK_Q4_NIBBLE
	,bool q4NibbleWork=false
#endif
#ifdef PIONEER_Q38_WORK_RAW_UNPACK_CACHE
	,bool rawCacheWork=false
#endif
)
{
	PioneerQ38VectorPanelSpan span; if (!PioneerQ38VectorPanel(type,panel,span)) return false;
#ifdef PIONEER_Q38_STATIC_VECTOR
#ifdef PIONEER_Q38_WORK_SIGNED_MAG2
	const bool signedMagnitude2=signedMagnitude2Work && type!=12;
#define PIONEER_Q38_SIGNED_ARGUMENT ,signedMagnitude2
#else
#define PIONEER_Q38_SIGNED_ARGUMENT
#endif
#ifdef PIONEER_Q38_WORK_Q4_NIBBLE
	const bool q4Nibble=q4NibbleWork && type==12;
	if(q4Nibble && !PioneerQ38Q4NibbleSetup(e,panel))return false;
#ifdef PIONEER_Q38_WORK_Q4_DIRECT_DECODE
#define PIONEER_Q38_NIBBLE_ARGUMENT ,false
#else
#define PIONEER_Q38_NIBBLE_ARGUMENT ,q4Nibble
#endif
#else
#define PIONEER_Q38_NIBBLE_ARGUMENT
#endif
#ifdef PIONEER_Q38_WORK_RAW_UNPACK_CACHE
	const bool rawUpper=PioneerQ38RawCacheUpper(type,panel,rawCacheWork);
#define PIONEER_Q38_RAW_CACHE_ARGUMENT ,rawUpper
#else
#define PIONEER_Q38_RAW_CACHE_ARGUMENT
#endif
#ifdef PIONEER_Q38_REPLAY_OPERAND
	unsigned count=PioneerQ38OperandReplayCount(type,panel); if (!count) return false;
#ifdef PIONEER_Q38_WORK_SIGNED_MAG2
	if(signedMagnitude2) { if(count<6)return false;count-=6; }
#endif
#ifdef PIONEER_Q38_WORK_Q4_NIBBLE
	if(q4Nibble)count=1;
#endif
	unsigned replay=0x04000000|(count<<4);
#define PIONEER_Q38_REPLAY_ARGUMENT ,replay
#else
#define PIONEER_Q38_REPLAY_ARGUMENT
#endif
	// SAME exact lane order and values; compile-time addresses eliminate
	// dynamic instruction-buffer stores. No RWC/config/format changes.
#ifdef PIONEER_Q38_WORK_Q4_DIRECT_DECODE
	if(q4Nibble) { if(!PioneerQ38Q4DirectDecode(e,panel))return false; }
	else
#endif
	if (!PioneerQ38VectorChunk<E,0,0>(e,type,panel,span.second PIONEER_Q38_SIGNED_ARGUMENT PIONEER_Q38_NIBBLE_ARGUMENT PIONEER_Q38_RAW_CACHE_ARGUMENT PIONEER_Q38_REPLAY_ARGUMENT) || !PioneerQ38VectorChunk<E,0,2>(e,type,panel,span.second PIONEER_Q38_SIGNED_ARGUMENT PIONEER_Q38_NIBBLE_ARGUMENT PIONEER_Q38_RAW_CACHE_ARGUMENT PIONEER_Q38_REPLAY_ARGUMENT)
		|| !PioneerQ38VectorChunk<E,4,0>(e,type,panel,span.second PIONEER_Q38_SIGNED_ARGUMENT PIONEER_Q38_NIBBLE_ARGUMENT PIONEER_Q38_RAW_CACHE_ARGUMENT PIONEER_Q38_REPLAY_ARGUMENT) || !PioneerQ38VectorChunk<E,4,2>(e,type,panel,span.second PIONEER_Q38_SIGNED_ARGUMENT PIONEER_Q38_NIBBLE_ARGUMENT PIONEER_Q38_RAW_CACHE_ARGUMENT PIONEER_Q38_REPLAY_ARGUMENT)
		|| !PioneerQ38VectorChunk<E,8,0>(e,type,panel,span.second PIONEER_Q38_SIGNED_ARGUMENT PIONEER_Q38_NIBBLE_ARGUMENT PIONEER_Q38_RAW_CACHE_ARGUMENT PIONEER_Q38_REPLAY_ARGUMENT) || !PioneerQ38VectorChunk<E,8,2>(e,type,panel,span.second PIONEER_Q38_SIGNED_ARGUMENT PIONEER_Q38_NIBBLE_ARGUMENT PIONEER_Q38_RAW_CACHE_ARGUMENT PIONEER_Q38_REPLAY_ARGUMENT)
		|| !PioneerQ38VectorChunk<E,12,0>(e,type,panel,span.second PIONEER_Q38_SIGNED_ARGUMENT PIONEER_Q38_NIBBLE_ARGUMENT PIONEER_Q38_RAW_CACHE_ARGUMENT PIONEER_Q38_REPLAY_ARGUMENT) || !PioneerQ38VectorChunk<E,12,2>(e,type,panel,span.second PIONEER_Q38_SIGNED_ARGUMENT PIONEER_Q38_NIBBLE_ARGUMENT PIONEER_Q38_RAW_CACHE_ARGUMENT PIONEER_Q38_REPLAY_ARGUMENT)) return false;
#undef PIONEER_Q38_REPLAY_ARGUMENT
#undef PIONEER_Q38_RAW_CACHE_ARGUMENT
#undef PIONEER_Q38_NIBBLE_ARGUMENT
#undef PIONEER_Q38_SIGNED_ARGUMENT
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
