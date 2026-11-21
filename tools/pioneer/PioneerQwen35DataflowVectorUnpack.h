/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_VECTOR_UNPACK_H
#define PIONEER_QWEN35_DATAFLOW_VECTOR_UNPACK_H

#include "PioneerQwen35DataflowProgram.h"
#include "PioneerQ38VectorPanel.h"

// Q5_K keeps its low four bits in qs and one additional bit in qh. The
// per-row GGUF tile stores both planes as byte-major 16x16 windows. This
// span mirrors the source indexing in the offline Q5 reference without
// materializing a decoded panel in RISC-V memory.
static bool PioneerQwen35DataflowQ5PanelSpan(unsigned panel,
	PioneerQ38VectorPanelSpan& result)
{
	if (panel>=16) return false;
	PioneerQ38VectorPanelSpan span;
	span.low=(48+(panel/4)*32+(panel%2)*16)*16;
	span.high=(16+(panel%2)*16)*16;
	span.second=true;
	if ((span.low&15) || span.low>2816-256
		|| (span.high&15) || span.high>2816-256) return false;
	result=span;
	return true;
}

// LReg0 is the Q5 low-nibble plane and LReg1 is the high-bit plane. The
// packed qh bit selected by panel/2 is shifted into bit 4; the result is the
// same nonnegative integer overlay used by the Q4 integer matrix path.
template<class E> static bool PioneerQwen35DataflowQ5Operand(E& e,
	unsigned panel)
{
	if (panel>=16) return false;
	if (panel&2) PioneerSfpuShift(e,0,-4);
	PioneerSfpuLoad32(e,4,15);
	PioneerSfpuAnd(e,0,0,4);
	if (panel/2) PioneerSfpuShift(e,1,-int(panel/2));
	PioneerSfpuLoad32(e,4,1);
	PioneerSfpuAnd(e,1,1,4);
	PioneerSfpuShift(e,1,4);
	PioneerSfpuXor(e,0,1);
	PioneerSfpuShift(e,0,21);
	PioneerSfpuLoad32(e,4,0x00100000);
	PioneerSfpuXor(e,0,4);
	return true;
}

// The decoded operands stay in Dst and are handed directly to the matrix
// source path. There is no scalar Q5 expansion or CPU dot product here.
template<class E> static bool PioneerQwen35DataflowQ5Transform(E& e,
	unsigned panel)
{
	if (panel>=16) return false;
	for (unsigned block=0;block<16;block+=4) {
		for (unsigned parity=0;parity<4;parity+=2) {
			e.Op(0x700d0000|block|parity);
			e.Op(0x701d0000|(16+block)|parity);
			if (!PioneerQwen35DataflowQ5Operand(e,panel)) return false;
			e.Op(0x72070000|(64+block)|parity);
		}
	}
	for (unsigned block=0;block<16;block+=4)
		e.Op(0x08002000|(block<<17)|(64+block));
	return true;
}

// Rewrite-only counterpart to the legacy fixed-0x90000 helper. The reader's
// two-page queue alternates between the original Qwen35 prefetch slots, so the
// compute role must identify the exact page whose read completed.
template<class E> static bool PioneerQwen35DataflowVectorUnpack(E& e,
	unsigned type,unsigned panel,unsigned rawBase)
{
	PioneerQ38VectorPanelSpan span;
	const bool weightPage=rawBase==PioneerQwen35::DataflowWeightBufferAddress
		|| rawBase==PioneerQwen35::DataflowWeightBufferAddress
			+PioneerQwen35::DataflowWeightBufferPageStride;
	if ((type==13?!PioneerQwen35DataflowQ5PanelSpan(panel,span)
		:!PioneerQ38VectorPanel(type,panel,span))
		|| (rawBase&63) || !weightPage) return false;
	const unsigned tileBytes=type==8?576:type==12?2304:type==13?2816:3392;
	if (!PioneerQwen35::DataflowLocalRangeValid(rawBase,tileBytes)) return false;
	e.Drain();
	e.Op(0x3700000f);
	e.Op(0x5e23fc00);
	e.Op(0x5120000a); e.Op(0x5420000f);
	e.Op(0xb2050000); e.Drain();
	e.Config(1,0xa03c8000);
	e.Config(49,0); e.Config(64,0x10001e); e.Config(65,16);
	e.Config(72,14); e.Config(73,0x13);
	e.Config(84,0x00400040);
	e.Config(76,(rawBase+span.low-16)/16);
	e.Op(span.second?0x42000081:0x420000c1); e.Drain();
	if (span.second) {
		e.Config(84,0x00400140);
		e.Config(76,(rawBase+span.high-16)/16);
		e.Op(0x420000c1); e.Drain();
	}
	e.Config(1,0xa0220000); e.Config(64,0x100011); e.Config(72,1);
	e.Config(73,3); e.Config(76,0x7070); e.Config(84,0x00400040);
	e.Op(0xb2050004); e.Drain();
	e.Op(0x3700000f);
	e.Op(0x8a000002);
	if (type==13) {
		if (!PioneerQwen35DataflowQ5Transform(e,panel)) return false;
	} else if (!PioneerQ38VectorTransform(e,type,panel)) return false;
	e.Drain();
	return true;
}

#endif
