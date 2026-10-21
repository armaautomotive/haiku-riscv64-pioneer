/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_VECTOR_FIXED_FORMAT_H
#define PIONEER_Q38_VECTOR_FIXED_FORMAT_H
#include "PioneerQ38VectorPanel.h"
#ifdef PIONEER_Q38_WORK_BATCH_RAW_INPUT
#ifndef PIONEER_Q38_WORK_BATCH4_FETCH
#error Direct batch input is a separate bounded-batch OFFLINE peer experiment
#endif
#include "PioneerQ38Batch4Fetch.h"
#endif
// OFFLINE ONLY. ISA8a34e25e MVMUL/MOVD2A plus cfg_defines81989dcdb:
// Config0 bit4 forces SrcA format1,bit9 forces SrcB format1. Thread2/3=1
// disable implied formats. Config1's SrcA default17..20 and unsigned bit15
// affect UNPACR,but NOT these forced matrix/move interpretations. INT8 math
// and FP32 Dst bits31/29 stay set. Therefore leave raw unsigned unpack
// formatting fixed instead of rewriting12 configuration words each panel.
// Thread5 is STILL0 for UNPACK-to-Dst and STILL4 before SrcB UNPACK/MOV;
// do NOT rely on the vendor's unsupported SrcB-with-Thread5=0 case.
static inline bool PioneerQ38VectorFixedMatrixEquivalent(uint32_t c0,uint32_t c1,uint32_t thread2,uint32_t thread3)
{
	return c0==0x4231 && (c0&0x1f)==0x11 && (c0&0x3e0)==0x220
		&& c1==0xa03c8000 && ((c1^0xa0220000)&~uint32_t(0x1e8000))==0
		&& thread2==1 && thread3==1;
}
template<class E> static void PioneerQ38VectorFixedSetup(E& e)
{
	e.Drain();
	// Entry is the EXACT checked rows Setup:65=16 and84=400040 already
	// written/read back. Only change the fields that actually differ.
	e.Config(1,0xa03c8000); e.Config(49,0); e.Config(64,0x10001e);
	e.Config(72,14); e.Config(73,0x13);
	e.Drain();
}
template<class E> static bool PioneerQ38VectorFixedUnpack(E& e,unsigned type,unsigned panel
#ifdef PIONEER_Q38_PREPARED_Q4_BLOCK
	,bool preparedQ4=false
#endif
#ifdef PIONEER_Q38_WORK_BATCH_RAW_INPUT
	,unsigned rawBase
#endif
)
{
	PioneerQ38VectorPanelSpan span; if (!PioneerQ38VectorPanel(type,panel,span)) return false;
#ifdef PIONEER_Q38_WORK_BATCH_RAW_INPUT
	if(!PioneerQ38Batch4RawInput(type,rawBase))return false;
#else
	const unsigned rawBase=0x90000;
#endif
#ifdef PIONEER_Q38_PREPARED_Q4_BLOCK
	if(preparedQ4) { if(type!=12 || span.second || span.low<256)return false;span.low-=256; }
#endif
	e.Op(0x3700000f); e.Op(0x5e23fc00); e.Op(0x5120000a); e.Op(0x5420000f);
	e.Op(0xb2050000); e.Drain();
	if (span.second) e.Config(84,0x00400040);
	e.Config(76,(rawBase+span.low-16)/16);
	e.Op(span.second?0x42000081:0x420000c1); e.Drain();
	if (span.second) {
		e.Config(84,0x00400140); e.Config(76,(rawBase+span.high-16)/16);
		e.Op(0x420000c1); e.Drain();
	}
	e.Op(0xb2050004); e.Drain(); // preserve ordinary SrcB unpack's supported setting
	e.Op(0x3700000f); e.Op(0x8a000002);
	if (!PioneerQ38VectorTransform(e,type,panel)) return false;
	e.Drain(); return true;
}
#endif
