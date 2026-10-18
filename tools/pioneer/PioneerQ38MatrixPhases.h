/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MATRIX_PHASES_H
#define PIONEER_Q38_MATRIX_PHASES_H
// OFFLINE ONLY. Pinned ISA8a34e25e RWCs/ApplyAddrMod and cfg81989dcdb:
// modifier1:AB[13]=0,Dst[29]=0x2000,Bias[48]=0;ONLY fidelity increments1.
// Setup's thread bank was zeroed. Matrix() STILL resets ALL RWCs per panel.
// Keep a complete drain after EACH matrix operation. This removes repeated
// SETC11 phase writes and their waits,NOT any fidelity phase or matrix wait.
template<class E> static void PioneerQ38MatrixPhasesSetup(E& e)
{
	e.Drain(); e.Op(0xb21d2000); e.Drain();
}
template<class E> static void PioneerQ38MatrixPhases(E& e)
{
	for (unsigned phase=0;phase<4;phase++) {
		e.Op(phase==3?0x26c04020:0x26004020); e.Drain();
	}
}
#endif
