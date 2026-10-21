/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_QUEUED_MATRIX_ENTRY_H
#define PIONEER_Q38_QUEUED_MATRIX_ENTRY_H
// Separate OFFLINE candidate. SAME physical dual-half ZEROACC, fixed
// SrcB unpack and source bank flips. No Config/NoC/CPU Dst accesses here.
// SETRWC/ZEROACC/MVMUL share the FPU execution unit. UNPACR publishes SrcB;
// MVMUL's documented Wait Gate waits for MatrixUnit ownership of BOTH
// sources. The existing final phase-batch drain MUST precede any scalar
// dot read/SFPU/config change. Never drop that drain or a metadata wait.
// https://github.com/tenstorrent/tt-isa-documentation/blob/main/WormholeB0/TensixTile/TensixCoprocessor/MVMUL.md
// https://github.com/tenstorrent/tt-isa-documentation/blob/main/WormholeB0/TensixTile/TensixCoprocessor/SETRWC.md
template<class E> static inline void PioneerQ38QueuedMatrixEntry(E& e)
{
	e.Op(0x3700000f);
	e.Op(0x10080004); e.Op(0x10080005);
	e.Op(0x5160000a); e.Op(0x5460000f);
	e.Op(0x428000c1);
}
#ifdef PIONEER_Q38_Q4_GROUP32
// OFFLINE ONLY:the preceding drained Q4 panel's SM32 dot remains in
// Dst32..47. Reset source/fidelity RWCs and publish FRESH SrcB as before,
// but deliberately do NOT ZEROACC until the next32-input group starts.
template<class E> static inline void PioneerQ38QueuedMatrixContinue(E& e)
{
	e.Op(0x3700000f);
	e.Op(0x5160000a); e.Op(0x5460000f); e.Op(0x428000c1);
}
#endif
#endif
