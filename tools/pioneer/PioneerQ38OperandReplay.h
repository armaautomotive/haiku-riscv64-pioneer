/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_OPERAND_REPLAY_H
#define PIONEER_Q38_OPERAND_REPLAY_H
#include "PioneerQ38VectorOperand.h"
// OFFLINE candidate. Exclusive thread replay entries0..20 contain ONLY
// the unchanged register-only operand decoder. Record+execute on the first
// chunk; subsequent chunks load fresh raw lanes, then replay that decoder.
// No address/config/NoC/Dst store/drain/nested REPLAY is recorded. A full
// existing drain after the transform proves expansion finished before any
// source reuse, matrix consumption, publication or eventual STOP.
// Pinned vendor model and encoding (Blackhole links to this shared file):
// https://github.com/tenstorrent/tt-isa-documentation/blob/main/WormholeB0/TensixTile/TensixCoprocessor/REPLAY.md
static inline unsigned PioneerQ38OperandReplayCount(unsigned type,unsigned panel)
{
	if ((type!=8 && type!=12 && type!=14) || panel>=(type==8?2U:16U)) return 0;
	// Opcode04,index bits18:14,count bits9:4,Exec bit1,Load bit0.
	// Count<=21, index0, no uninitialized replay entry can be consumed.
	return type==8?14:type==12?6+unsigned(bool(panel&2)):19+unsigned(bool(panel&4))+unsigned(bool(panel&6));
}
#endif
