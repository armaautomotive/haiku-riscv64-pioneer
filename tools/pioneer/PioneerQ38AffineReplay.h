/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_AFFINE_REPLAY_H
#define PIONEER_Q38_AFFINE_REPLAY_H
#ifndef PIONEER_Q38_WORK_AFFINE_REPLAY
#error Separate OFFLINE Work affine replay only
#endif
#include "PioneerQ38ZeroMinimumAffine.h"
#include "PioneerSfpuDivide.h"
// OFFLINE ONLY. Slots12..31:register-only exact affine,NEVER memory,
// source publication/config/drain/NoC or nested REPLAY. Q4 operand decoder
// uses0..2,Q8 TC decoder0..9;native-SM7 Q8 uses none. Q6 is NOT eligible.
// Record+execute first parity of first panel EACH OUTPUT GROUP,including
// the first group EACH JOB;never depend on a prior job's program. Fresh
// dynamic LRegs0/1/2/3/4/5 are supplied OUTSIDE replay.
// Canonical mode retains signed-zero canonicalization;the separately proved
// raw-dot mode elides ONLY that. Non-fused rounding and MAD latency remain.
// Blackhole links to this shared ISA description:
// https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/TensixTile/TensixCoprocessor/REPLAY.md
template<class E> static __attribute__((always_inline)) inline void PioneerQ38AffineReplay(E& e,bool zeroMinimum,bool record) {
	unsigned count=zeroMinimum?13:20;
#ifdef PIONEER_Q38_WORK_DOT_ZERO_ELISION
	// Separate OFFLINE proof:actual Work accumulator starts at +0 and
	// remains normal/+0. SFPCAST may retain raw dot -0;its sign can affect
	// only zero intermediates,not the FINAL old+affine result. Arbitrary
	// negative-zero accumulators are expressly NOT covered. All original
	// FP32 rounding and MAD latency instructions remain below.
	count-=6;
#endif
	if(!record){e.Op(0x04000000|(12U<<14)|(count<<4));return;}
	e.Op(0x04000000|(12U<<14)|(count<<4)|3U);
#ifndef PIONEER_Q38_WORK_DOT_ZERO_ELISION
	PioneerSfpuLoad32(e,6,0x7fffffff);PioneerSfpuAnd(e,7,0,6);
	PioneerSfpuAdd(e,7,6);PioneerSfpuShift(e,7,-31,true);PioneerSfpuAnd(e,0,0,7);
#endif
	if(zeroMinimum)PioneerQ38ZeroMinimumAffine(e);else PioneerQ38VectorAffine(e);
}
#endif
