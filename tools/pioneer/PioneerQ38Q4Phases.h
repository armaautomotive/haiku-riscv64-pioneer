/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_Q4_PHASES_H
#define PIONEER_Q38_Q4_PHASES_H
// Separate exact INTEGER candidate, not the BF16 experiment. The shared
// Blackhole/Wormhole MVMUL ISA masks abs(SrcA) to 224 in phases0/2 and31
// in phases1/3. Q4_K's decoded SrcA is0..15, so phases0/2 add EXACT zero.
// SrcB remains the original signed activation; BOTH its magnitude slices
// are consumed. Q6 includes -32 and Q8 includes -128: keep ALL4 for them.
// https://github.com/tenstorrent/tt-isa-documentation/blob/main/WormholeB0/TensixTile/TensixCoprocessor/SrcASrcB.md#fidelity-phases-integer
// Setup ONCE per job. SETRWC still resets all RWCs before EVERY panel.
// AddrMod1 has zero AB/Dst/Bias increments; only fidelity increments2(Q4)
// or1(other). The native physical ZEROACC fix and EACH matrix drain stay.
template<class E> static void PioneerQ38Q4PhasesSetup(E& e,unsigned type)
{
	e.Drain(); e.Op(type==12?0xb20b0001:0xb20b0000);
	e.Op(type==12?0xb21d4000:0xb21d2000); e.Drain();
}
template<class E> static void PioneerQ38Q4Phases(E& e,unsigned type)
{
	// Constant instructions retain direct inline Tensix issue. A dynamic
	// last-phase ternary falls back to the slower instruction-buffer store.
	if (type!=12) { e.Op(0x26004020); e.Drain(); e.Op(0x26004020); e.Drain(); }
	e.Op(0x26004020); e.Drain(); e.Op(0x26c04020); e.Drain();
}
template<class E> static void PioneerQ38QueuedPhases(E& e,unsigned type)
{
	// Separate OFFLINE candidate: SAME ordered MVMULs/AddrMod/final flip,
	// but drain the SAME matrix unit ONCE after its complete phase batch.
	// No source/config/NoC/Dst access may interleave with this batch. The
	// final real drain still precedes ALL scalar/SFPU result consumption.
	// Vendor Blackhole matmul MOP similarly queues adjacent MVMULs:
	// https://github.com/tenstorrent/tt-metal/blob/main/tt_metal/tt-llk/tt_llk_blackhole/llk_lib/llk_math_matmul.h
	if (type!=12) { e.Op(0x26004020); e.Op(0x26004020); }
	e.Op(0x26004020); e.Op(0x26c04020); e.Drain();
}
#endif
