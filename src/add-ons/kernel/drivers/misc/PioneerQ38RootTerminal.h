/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_ROOT_TERMINAL_H
#define PIONEER_Q38_ROOT_TERMINAL_H
// NEW private root image only. Both words jump to PARK;the speculative
// frontend snapshot may name either. Exact DONE/count,code/source/counter
// proofs remain mandatory. Fault code is disjoint and NEVER accepted.
enum { TT_Q38_DISPATCH_PARK=0x72038,TT_Q38_DISPATCH_PARK_FETCH=0x7203c,
	TT_Q38_DISPATCH_FAULT_ENTRY=0x72060,TT_Q38_DISPATCH_FAULT_PARK=0x7207c };
static inline bool PioneerQ38RootTerminalPc(unsigned pc)
{ return pc==TT_Q38_DISPATCH_PARK || pc==TT_Q38_DISPATCH_PARK_FETCH; }
#endif
