/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_SIGNED_MAGNITUDE2_H
#define PIONEER_Q38_SIGNED_MAGNITUDE2_H
// OFFLINE arithmetic prerequisite ONLY; no firmware/driver selects it.
// Input LReg0 is a signed TC integer (Q8 -128..127 or Q6 -32..31).
// Integer ABS writes magnitude into LReg2 WITHOUT overwriting input0.
// SETSGN then combines magnitude FROM2 with original sign FROM DEST0.
// Important: SETSGN's sign comes from its old destination, not VC.
// This replaces eight register-only ops with two. No Dst/source/config/
// conditional-state/NoC/ownership change. INT32_MIN is OUTSIDE the domain.
// Blackhole explicitly shares these two functional models with Wormhole:
// https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/TensixTile/TensixCoprocessor/SFPABS.md
// https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/TensixTile/TensixCoprocessor/SFPSETSGN.md
// Encodings: main/Diagrams/Out/Bits32_SFP{ABS,SETSGN}.svg in that repository.
template<class E> static inline void PioneerQ38SignedMagnitude2(E& e)
{
	e.Op(0x7d000020); // SFPABS VC0,VD2,INT0; retain the original signed input0
	e.Op(0x89000200); // SFPSETSGN VC2,VD0,register0; retain OLD DEST0's sign
}
#endif
