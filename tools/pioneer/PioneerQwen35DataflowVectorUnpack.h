/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_VECTOR_UNPACK_H
#define PIONEER_QWEN35_DATAFLOW_VECTOR_UNPACK_H

#include "PioneerQwen35DataflowProgram.h"
#include "PioneerQ38VectorPanel.h"

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
	const bool repackedQ5Panel=type==8 && panel==0
		&& rawBase==PioneerQwen35::DataflowQ5PanelScratchAddress;
	if (!PioneerQ38VectorPanel(type,panel,span) || (rawBase&63)
		|| (!weightPage && !repackedQ5Panel)) return false;
	const unsigned tileBytes=type==8?576:type==12?2304:3392;
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
	if (!PioneerQ38VectorTransform(e,type,panel)) return false;
	e.Drain();
	return true;
}

#endif
