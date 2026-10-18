/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BFLOAT_PANEL_H
#define PIONEER_Q38_BFLOAT_PANEL_H
#include "PioneerQ38BFloatOperand.h"
#include "PioneerQ38VectorPanel.h"
// OFFLINE ONLY. Explicit A/B BF16 overrides ignore raw-UNPACK default and
// unsigned fields. FP32 Dst enabled,INT8 math disabled. No implied formats.
static inline bool PioneerQ38BFloatMatrixEquivalent(uint32_t c0,uint32_t c1,uint32_t t2,uint32_t t3)
{
	return c0==0x42b5 && (c0&0x1f)==0x15 && (c0&0x3e0)==0x2a0
		&& c1==0x203c8000 && ((c1^0x20aa0000)&~uint32_t(0x1fe8000))==0
		&& t2==1 && t3==1;
}
template<class E> static bool PioneerQ38BFloatPanel(E& e,unsigned type,unsigned panel)
{
	PioneerQ38VectorPanelSpan span; if (!PioneerQ38VectorPanel(type,panel,span)) return false;
	e.Op(0x3700000f); e.Op(0x5e23fc00); e.Op(0x5120000a); e.Op(0x5420000f);
	e.Op(0xb2050000); e.Drain();
	if (span.second) e.Config(84,0x00400040);
	e.Config(76,(0x90000+span.low-16)/16);
	e.Op(span.second?0x42000081:0x420000c1); e.Drain();
	if (span.second) {
		e.Config(84,0x00400140); e.Config(76,(0x90000+span.high-16)/16);
		e.Op(0x420000c1); e.Drain();
	}
	e.Op(0xb2050004); e.Drain();
	e.Op(0x3700000f); e.Op(0x8a000002);
	for (unsigned block=0;block<16;block+=4) for (unsigned parity=0;parity<4;parity+=2) {
		e.Op(0x700d0000|block|parity);
		if (span.second) e.Op(0x701d0000|(16+block)|parity);
		if (!PioneerQ38BFloatOperand(e,type,panel)) return false;
		e.Op(0x72030000|(64+block)|parity); // FP32 encoding,NOT opaque integer overlay
	}
	for (unsigned block=0;block<16;block+=4) e.Op(0x08002000|(block<<17)|(64+block));
	e.Drain(); return true;
}
#endif
