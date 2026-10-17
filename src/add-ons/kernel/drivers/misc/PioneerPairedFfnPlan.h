/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_PAIRED_FFN_PLAN_H
#define PIONEER_PAIRED_FFN_PLAN_H
// OFFLINE prerequisite. Fixed Qwen3-0.6B gate/up,each3072 rows/K1024.
// Every peer owns ONE contiguous span of ONE immutable tensor. No span
// crosses a tensor boundary or exceeds the existing32/64-row math shapes.
struct PioneerPairedFfnSpan { unsigned tensor,first,rows; };
static inline bool PioneerPairedFfnWorker(unsigned worker,PioneerPairedFfnSpan& span)
{
	if (!worker || worker>119) return false;
	unsigned tensor=worker>60,ordinal=tensor?worker-61:worker-1,extra=tensor?37:36;
	unsigned preceding=ordinal<extra?ordinal:extra;
	span={tensor,(ordinal+preceding)*32,(ordinal<extra?2U:1U)*32};
	return true;
}
#endif
