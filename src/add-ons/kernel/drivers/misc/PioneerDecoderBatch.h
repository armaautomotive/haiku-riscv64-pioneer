/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_DECODER_BATCH_H
#define PIONEER_DECODER_BATCH_H
#include "PioneerDecoderLayout.h"
// OFFLINE protocol prerequisite ONLY. No ioctl or native image selects it.
// Start with at most TWO positions:the existing worst-case65535 polling
// rounds/job then fits unambiguously in one unsigned32 NoC counter delta.
// Larger batches require explicit carry/epoch proofs,not relaxed bounds.
enum { TT_DECODER_BATCH_MAX=2,TT_DECODER_BATCH_WORDS=64,
	TT_DECODER_BATCH_MAGIC=0x31424451,TT_DECODER_BATCH_OPERATION=10 };
struct PioneerDecoderBatchPlan {
	uint32_t firstTokens=0,requested=0,lastLimit=0,nonce=0;
	uint32_t firstSequence[119]={},lastSequence[119]={};
};
static inline bool PioneerDecoderPlanBatch(unsigned firstTokens,unsigned requested,unsigned nonce,
	const uint32_t* current,PioneerDecoderBatchPlan& plan)
{
	plan={};
	if (!current || !firstTokens || firstTokens>256 || !requested || requested>TT_DECODER_BATCH_MAX
		|| requested>257-firstTokens || !nonce || nonce>=0x7fffffff) return false;
	for (unsigned w=0;w<119;w++) {
		unsigned advance=PioneerDecoderSequenceAdvance(w,true,true,true)*requested;
		if (current[w]>0x7ffffffe - advance) return false;
	}
	plan.firstTokens=firstTokens; plan.requested=requested; plan.lastLimit=firstTokens+requested-1; plan.nonce=nonce;
	for (unsigned w=0;w<119;w++) {
		plan.firstSequence[w]=current[w]+1;
		plan.lastSequence[w]=current[w]+PioneerDecoderSequenceAdvance(w,true,true,true)*requested;
	}
	return true;
}
static inline bool PioneerDecoderBatchPacket(const uint32_t* packet,unsigned nonce,unsigned firstTokens,
	unsigned requested,unsigned eos0,unsigned eos1,unsigned& completed)
{
	completed=0;
	if (!packet || !nonce || nonce>=0x7fffffff || !firstTokens || firstTokens>256 || !requested || requested>TT_DECODER_BATCH_MAX
		|| requested>257-firstTokens || eos0>=151936 || eos1>=151936 || eos0==eos1
		|| packet[0]!=TT_DECODER_BATCH_MAGIC || packet[1]!=nonce || packet[2]!=151936
		|| !packet[3] || packet[3]>requested || packet[4]!=firstTokens || packet[5]!=firstTokens+packet[3]-1
		|| packet[6]!=119 || packet[7]!=TT_DECODER_BATCH_OPERATION) return false;
	unsigned count=packet[3];
	for (unsigned i=0;i<16;i++) {
		if (i>=count) { if (packet[8+i] || packet[24+i]) return false; continue; }
		unsigned token=packet[8+i];
		if (token>=151936 || (packet[24+i]&0x7f800000)==0x7f800000
			|| (i+1<count && (token==eos0 || token==eos1))) return false;
	}
	if (count<requested && packet[8+count-1]!=eos0 && packet[8+count-1]!=eos1) return false;
	for (unsigned i=40;i<64;i++) if (packet[i]) return false;
	completed=count; return true;
}
static inline bool PioneerDecoderBatchSummary(const uint32_t* summary,unsigned completed,unsigned nonce,
	const uint32_t* before,const uint32_t* after)
{
	if (!summary || !before || !after || !completed || completed>TT_DECODER_BATCH_MAX || !nonce || nonce>=0x7fffffff
		|| summary[4]!=119 || summary[5]!=151936 || summary[6]!=nonce || summary[7]!=TT_DECODER_BATCH_OPERATION
		|| before[2]!=before[6] || after[2]!=after[6]) return false;
	// Per-position31 minus its host packet;two512B immutable RoPE reads
	// EVERY position;64x576B embedding reads BETWEEN positions;one256B
	// ordered host write/read at the END. No per-token host transfer.
	unsigned minimum=29116*completed+64*(completed-1)+1;
	unsigned readFlits=61558*completed+576*(completed-1)+4;
	unsigned writes=9611*completed+1,writeFlits=9611*completed+4,r=summary[0];
	if (r<minimum || r>minimum+2U*65535*9219*completed || ((r-minimum)&1)
		|| summary[1]!=readFlits+(r-minimum) || summary[2]!=writes || summary[3]!=writeFlits) return false;
	const unsigned delta[]={r,summary[1],r+writes,r,writes,writeFlits,r+writes,writes};
	for (unsigned i=0;i<8;i++) if (summary[8+i]!=before[i] || summary[16+i]!=after[i] || uint32_t(after[i]-before[i])!=delta[i]) return false;
	return true;
}
#endif
