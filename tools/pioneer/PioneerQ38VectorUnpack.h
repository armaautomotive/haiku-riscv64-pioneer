/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_VECTOR_UNPACK_H
#define PIONEER_Q38_VECTOR_UNPACK_H
#include "PioneerQ38VectorPanel.h"
// OFFLINE instruction-sequence prerequisite ONLY. No image/driver selects
// this helper. Fixed owned local block90000,not caller-provided addresses.
// Entry:qualified rows Setup,all prior matrix work drained,both source
// banks released,ADCs/RWCs private. Exit:the CURRENT MatrixUnit SrcA bank
// contains the exact signed integer-overlay panel and is MatrixUnit-owned.
// Caller must NOT issue the old ordinary SrcA UNPACR again;SrcB and the
// existing four fidelity phases/affine accumulation remain its obligation.
// ISA8a34e25e UNPACR_Regular:unsigned INT8-to-Dst,context0,header+16,
// SetOvrdWithAddr=0,first Q6 plane DOES NOT publish/flip its source bank.
template<class E> static bool PioneerQ38VectorUnpack(E& e,unsigned type,unsigned panel)
{
	PioneerQ38VectorPanelSpan span;
	if (!PioneerQ38VectorPanel(type,panel,span)) return false;
	e.Drain();
	e.Op(0x3700000f); // all RWCs/fidelity zero;address modifiers remain zero
	e.Op(0x5e23fc00); // U0 X0=0/X1=255,exactly256 raw bytes
	e.Op(0x5120000a); e.Op(0x5420000f); // U0 Y/Z/W reset,DO NOT reset X1
	e.Op(0xb2050000); e.Drain(); // Thread5 SetOvrdWithAddr MUST be zero
	e.Config(1,0xa03c8000); // INT8/FP32 engine,SrcA INT8,unsigned;SrcB FP16
	e.Config(49,0); e.Config(64,0x10001e); e.Config(65,16);
	e.Config(72,14); e.Config(73,0x13); // context0 INT8 output,to-Dst selector
	e.Config(84,0x00400040); // context0 output64 => Dst16 row0
	e.Config(76,(0x90000+span.low-16)/16);
	e.Op(span.second?0x42000081:0x420000c1); e.Drain();
	if (span.second) {
		e.Config(84,0x00400140); // output320 => Dst16 row16,not row0
		e.Config(76,(0x90000+span.high-16)/16);
		e.Op(0x420000c1); e.Drain(); // ONLY now publish the same SrcA bank
	}
	// Restore signed FP16 overlay interpretation BEFORE MOVD2A,including
	// implied-format disable inherited from Setup's Thread2=1. SrcA was
	// clobbered only as documented;ONLY the current owned bank
	// is rebuilt. Dst32 decoded rows64..79 do not alias raw Dst16 rows0..31.
	e.Config(1,0xa0220000); e.Config(64,0x100011); e.Config(72,1);
	e.Config(73,3); e.Config(76,0x7070); e.Config(84,0x00400040);
	e.Op(0xb2050004); e.Drain();
	e.Op(0x3700000f);
	if (!PioneerQ38VectorTransform(e,type,panel)) return false;
	e.Drain();
	return true;
}
#endif
