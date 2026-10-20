/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DELTA_VECTOR_H
#define PIONEER_Q38_DELTA_VECTOR_H
#include "PioneerSfpuDivide.h"
#include "PioneerQ38VectorAffine.h"
// LOCAL arithmetic prerequisite. The pre-scaled variant has native fixture
// qualification;the post-dot-scaled variant needs its own qualification.
// Neither variant is yet a resident GGML capability. One head's64 columns
// using32 SFPU lanes and two parities,not scalar RISC learned products.
//
// Entry: ALL512 logical FP32 Dst rows are DEFINED by a completed full-row
// import, with128 keys x64 values in key-major order. LRegs6/7 hold the64
// corresponding V values in the standard even/odd SFPU lane mapping.
// Q is normalized;default mode additionally requires PRE-SCALED Q.
// PostScale=true instead applies the pinned128-key GGML output scale AFTER
// the completed query dot. Never silently interchange these conventions.
// K normalized;state immutable in L1.
// The caller owns/proves formats,predication,Dst/source banks,and fixed
// local Q/K metadata. QueryBits/KeyBits return scalar DATA,not addresses.
// Exit: Dst contains next state,4/5 contain64 outputs;real final drain.
// No host or NoC memory,address,allocation,reset,restore or END authority.
// A future resident registry MUST adapt GGML's transposed state layout;
// this key-major primitive is NOT directly a GGML buffer implementation.
// Preserve scalar non-fused FP32 rounding in both prediction and update.
// Numerical qualification below is for normal-or-zero intermediates. Native
// SFPU arithmetic/FP32 stores flush denormals;this is NOT an arbitrary-input
// bit-exact GGML guarantee. Such coverage needs separate numerical work.
static inline bool PioneerQ38DeltaVectorGate(uint32_t bits)
{
	return bits<=0x3f800000U && (!bits || (bits&0x7f800000));
}
template<bool PostScale=false,class E> static bool PioneerQ38DeltaVectorHalf(E& e,uint32_t decay,uint32_t beta)
{
	// Nonnegative IEEE encodings are monotonic. Refuse before ANY issue.
	if (!PioneerQ38DeltaVectorGate(decay) || !PioneerQ38DeltaVectorGate(beta)) return false;
	e.Op(0x8a000002); // all32 lanes,not a data-dependent conditional flag
	PioneerSfpuLoad32(e,2,decay);
	PioneerSfpuLoad32(e,4,0); PioneerSfpuLoad32(e,5,0);
	for (unsigned key=0;key<128;key++) {
		PioneerSfpuLoad32(e,3,e.KeyBits(key));
		for (unsigned parity=0;parity<4;parity+=2) {
			e.Op(0x70030000|(key*4)|parity);
			PioneerQ38VectorAffineMul(e,0,2,0);
			PioneerQ38VectorAffineMul(e,0,3,0);
			PioneerQ38VectorAffineAdd(e,parity?5:4,0,parity?5:4);
		}
	}
	PioneerSfpuLoad32(e,3,beta); PioneerSfpuLoad32(e,1,0x80000000);
	PioneerSfpuXor(e,4,1); PioneerSfpuXor(e,5,1);
	PioneerQ38VectorAffineAdd(e,6,4,6); PioneerQ38VectorAffineAdd(e,7,5,7);
	PioneerQ38VectorAffineMul(e,6,3,6); PioneerQ38VectorAffineMul(e,7,3,7);
	PioneerSfpuLoad32(e,4,0); PioneerSfpuLoad32(e,5,0);
	for (unsigned key=0;key<128;key++) {
		PioneerSfpuLoad32(e,3,e.KeyBits(key)); uint32_t q=e.QueryBits(key);
		for (unsigned parity=0;parity<4;parity+=2) {
			e.Op(0x70030000|(key*4)|parity);
			PioneerQ38VectorAffineMul(e,0,2,0);
			PioneerQ38VectorAffineMul(e,3,parity?7:6,1);
			PioneerQ38VectorAffineAdd(e,0,1,0);
			e.Op(0x72030000|(key*4)|parity);
			PioneerSfpuLoad32(e,1,q);
			PioneerQ38VectorAffineMul(e,0,1,0);
			PioneerQ38VectorAffineAdd(e,parity?5:4,0,parity?5:4);
		}
	}
	if (PostScale) {
		// Exact F32 encoding of1.f/sqrtf(128.f) in the pinned GGML CPU op.
		// It scales only the64 outputs,NEVER the next state or prior Q/K.
		PioneerSfpuLoad32(e,1,0x3db504f3);
		PioneerQ38VectorAffineMul(e,4,1,4);
		PioneerQ38VectorAffineMul(e,5,1,5);
	}
	e.Drain(); return true;
}
#endif
