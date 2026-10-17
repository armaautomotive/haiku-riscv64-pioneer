/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QKV_DECODER_PROOF_H
#define PIONEER_QKV_DECODER_PROOF_H
#include "PioneerDecoderLayout.h"
#include "PioneerDecoderStageProfile.h"
#include "PioneerPairedQkvPlan.h"
// OFFLINE private11 prerequisite ONLY. No driver,registry or client selects
// this schedule. Same fixed shape/owners/batch/EOG/arithmetic,distinct cursors.
static_assert(sizeof(PioneerDecoderJob)==4*(16+28*11+119+128),"private11 word-complete snapshot");
static inline bool PioneerQkvDecoderJobValid(const volatile PioneerDecoderJob& job)
{
	PioneerDecoderJob base; // EVERY word assigned below;no freestanding memset.
	for (unsigned i=0;i<sizeof(base)/4;i++) reinterpret_cast<uint32_t*>(&base)[i]=reinterpret_cast<const volatile uint32_t*>(&job)[i];
	if (base.version!=11 || !base.reserved[0] || base.reserved[0]>2) return false;
	for (unsigned i=0;i<119;i++) {
		unsigned advance=PioneerPairedQkvSequenceAdvance(i)*base.reserved[0];
		if (!base.sequence[i] || base.sequence[i]>0x7fffffff-advance) return false;
		base.sequence[i]=1; // Reuse ALL v10 shape/arena/EOG checks,not its old cursors.
	}
	base.version=10;
	return PioneerDecoderJobValid(base,10);
}
static inline bool PioneerQkvDecoderStageValid(const tt_decoder_stage_profile& p,unsigned nonce,unsigned tokens,unsigned positions=1)
{
	if (p.version!=6 || !positions || positions>2 || p.calls[0]!=112*positions) return false;
	auto baseline=p; baseline.version=5; baseline.calls[0]=168*positions;
	return PioneerDecoderStageValid(baseline,nonce,tokens,5,positions);
}
static inline bool PioneerQkvDecoderSummary(const uint32_t* s,unsigned positions,unsigned nonce,
	const uint32_t* before,const uint32_t* after,unsigned operation)
{
	if (!s || !before || !after || !positions || positions>2 || !nonce || nonce>=0x7fffffff
		|| (operation!=8 && operation!=9 && operation!=10) || (operation!=10 && positions!=1)
		|| s[4]!=119 || s[5]!=151936 || s[6]!=nonce || s[7]!=operation
		|| before[2]!=before[6] || after[2]!=after[6]) return false;
	// Exactly252 fewer math jobs/position. EACH removes one doorbell and
	// two fixed read packets/flits;ALL4096 Q/V/K output data flits remain.
	unsigned minimum=19786,rf=61056,w=9360,wf=9361;
	if (operation==8) { minimum+=118; rf+=9494; w+=118; wf+=9494; }
	if (operation==10) {
		minimum=19785*positions+64*(positions-1)+1;
		rf=61054*positions+576*(positions-1)+4;
		w=9359*positions+1; wf=9359*positions+4;
	}
	unsigned r=s[0];
	if (r<minimum || r>minimum+2U*65535*8967*positions || ((r-minimum)&1)
		|| s[1]!=rf+(r-minimum) || s[2]!=w || s[3]!=wf) return false;
	const unsigned delta[]={r,s[1],r+w,r,w,wf,r+w,w};
	for (unsigned i=0;i<8;i++) if (s[8+i]!=before[i] || s[16+i]!=after[i] || uint32_t(after[i]-before[i])!=delta[i]) return false;
	return true;
}
#endif
