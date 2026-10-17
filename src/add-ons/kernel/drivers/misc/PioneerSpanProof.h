/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_SPAN_PROOF_H
#define PIONEER_SPAN_PROOF_H
#include "PioneerCoordinatedFleet.h"
// Dedicated contract11 span predicate,not a learned-arithmetic validation.
struct tt_layer_span_result {
	tt_coordinator_probe_result master;
	uint32_t bytes,pages,matched_words,host_exact,summary[24];
};
static inline bool PioneerSpanSummary(const uint32_t* s,unsigned bytes,unsigned nonce,const uint32_t* before,const uint32_t* after)
{
	if (!s || !before || !after || !bytes || bytes>TT_WIDE_OUTPUT_BYTES || (bytes&4095) || !nonce || nonce>=0x7fffffff
		|| s[4]!=bytes/4096 || s[5]!=bytes || s[6]!=nonce || s[7]!=7) return false;
	unsigned chunks=(bytes+16383)/16384,r=4*chunks,rf=bytes/16,w=3*chunks,wf=3*(bytes/64);
	if (s[0]!=r || s[1]!=rf || s[2]!=w || s[3]!=wf || before[2]!=before[6] || after[2]!=after[6]) return false;
	const unsigned delta[]={r,rf,r+w,r,w,wf,r+w,w};
	for (unsigned i=0;i<8;i++) if (s[8+i]!=before[i] || s[16+i]!=after[i] || uint32_t(after[i]-before[i])!=delta[i]) return false;
	return true;
}
static inline bool PioneerSpanProof(const tt_layer_span_result& r,unsigned bytes,unsigned nonce)
{
	const auto& m=r.master;
	return r.bytes==bytes && r.pages==bytes/4096 && r.matched_words==bytes/4 && r.host_exact==1
		&& m.version==1 && m.state==TT_RUNTIME_PASSED && m.workers==1 && m.matched==1 && m.nonce==nonce
		&& m.prepared==1 && m.started==1 && m.completed==1 && m.stopped==1 && !m.recovery_required && m.polls
		&& m.phase==TT_LAYER_DONE && m.count==bytes/4096 && m.marker==TT_COORDINATOR_MARKER
		&& m.pc[0]==TT_LAYER_PARK && m.pc[1]==m.pc[0] && m.summary_verified==1
		&& PioneerSpanSummary(r.summary,bytes,nonce,m.before,m.after);
}
#endif
