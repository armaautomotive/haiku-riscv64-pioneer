/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PEER_TERMINAL_H
#define PIONEER_Q38_PEER_TERMINAL_H
// Private NEW peer image only. BOTH accepted instruction words branch to
// STOP forever;the fault entry/park are disjoint and NEVER accepted.
// Snapshot PCs are speculative frontend observations,NOT retirement proof.
// Status nonce+STOP marker,exact sealed code/source/NoC proofs and ALL-role
// audits remain mandatory before any hold/restoration.
enum { TT_Q38_PEER_STOP=0x72060,TT_Q38_PEER_STOP_FETCH=0x72064,
	TT_Q38_PEER_FAULT_ENTRY=0x72080,TT_Q38_PEER_FAULT_PARK=0x7209c };
static inline bool PioneerQ38PeerTerminalPc(unsigned pc)
{ return pc==TT_Q38_PEER_STOP || pc==TT_Q38_PEER_STOP_FETCH; }
#endif
