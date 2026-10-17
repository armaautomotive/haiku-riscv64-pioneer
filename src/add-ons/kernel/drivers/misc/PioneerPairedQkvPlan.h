/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_PAIRED_QKV_PLAN_H
#define PIONEER_PAIRED_QKV_PLAN_H
// OFFLINE prerequisite ONLY. Fixed Qwen3-0.6B Q2048/V1024/K1024, K1024.
// ONE contiguous 32/64-row span of ONE initialized tensor per math peer.
// No driver/table/root selects this schedule; a distinct contract is required.
struct PioneerPairedQkvSpan { unsigned tensor,first,rows; };
static inline unsigned PioneerPairedQkvSequenceAdvance(unsigned index)
{
	return index>=119?0:(index<32?113:57)+(index<8?28:0)+(index<6?28:0);
}
static inline bool PioneerPairedQkvWorker(unsigned worker,PioneerPairedQkvSpan& span)
{
	if (!worker || worker>119) return false;
	unsigned tensor=worker<=60?0:worker<=90?1:2;
	unsigned ordinal=tensor==0?worker-1:tensor==1?worker-61:worker-91;
	unsigned extra=tensor==0?4:tensor==1?2:3;
	span={tensor,(ordinal+(ordinal<extra?ordinal:extra))*32,(ordinal<extra?2U:1U)*32};
	return true;
}
static inline unsigned PioneerPairedQkvDestination(const PioneerPairedQkvSpan& span)
{
	unsigned rows=span.tensor==0?2048:1024;
	if (span.tensor>2 || (span.first&31) || (span.rows!=32 && span.rows!=64)
		|| span.first>rows || span.rows>rows-span.first) return 0;
	unsigned base=span.tensor==0?0x87000:span.tensor==1?0x84000:0x86000;
	return base+span.first*4;
}
static inline bool PioneerPairedQkvPreservePrefix(const PioneerPairedQkvSpan& span)
{
	// Reverse gather of Q's first span borders ALREADY gathered K's last64B.
	return !span.tensor && !span.first && PioneerPairedQkvDestination(span);
}
#endif
