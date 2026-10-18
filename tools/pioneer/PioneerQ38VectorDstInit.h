/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_VECTOR_DST_INIT_H
#define PIONEER_Q38_VECTOR_DST_INIT_H
// OFFLINE ONLY,AFTER Setup and full activation validation,BEFORE first raw
// unpack. Pinned ZEROACC/SFPLOAD/SFPSTORE require DEFINED destination rows
// before partial-column vector writes. Initialize COMPLETE FP32 rows64..79
// and,ONLY for metadata/accumulator users,96..111 with full-row UNPACK.
// Source a0000..a0400 is an already validated part of the original input.
// Each SM16 pair interpreted as FP32 is finite;AllDatumsAreZero(bit4)
// overrides it to EXACT0. No uninitialized read/extra L1 zero buffer.
template<class E>
#if defined(PIONEER_Q38_SFPU_ACCUMULATOR) && defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("Os")))
#endif
static void PioneerQ38VectorDstInit(E& e,bool bfloat,bool metadata,bool fixed)
{
	e.Op(0x3700000f); e.Op(0x5e23fc00); e.Op(0x5120000a); e.Op(0x5420000f);
	e.Op(0xb2050000); e.Drain();
	e.Config(1,bfloat?0x20200000:0xa0200000); e.Config(49,0);
	e.Config(64,0x100010); e.Config(65,16); e.Config(72,0); e.Config(73,0x13); e.Config(76,0x9fff);
	for (unsigned group=0;group<(metadata?2U:1U);group++) {
		e.Config(84,0x00400000+((group?96:64)+4)*16);
		e.Op(0x42000091); e.Drain(); // ALL256 datums zero,ALL16 columns each row,no flip
	}
	e.Config(1,bfloat?0x203c8000:fixed?0xa03c8000:0xa0220000); e.Config(64,(bfloat || fixed)?0x10001e:0x100011);
	e.Config(72,(bfloat || fixed)?14:1); e.Config(73,(bfloat || fixed)?0x13:3); e.Config(76,0x7070); e.Config(84,0x00400040);
	e.Op(0xb2050004); e.Drain();
}
#endif
