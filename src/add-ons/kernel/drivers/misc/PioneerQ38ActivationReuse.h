/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_ACTIVATION_REUSE_H
#define PIONEER_Q38_ACTIVATION_REUSE_H
#include "PioneerQ38DispatchProtocol.h"
#ifndef PIONEER_Q38_COMPACT_ACTIVATIONS
#error Activation reuse requires the separately selected compact root image
#endif
// Kernel-private, heap-owned hint, NEVER a source/completion authority. Only
// a completely proved, held RUN may commit it. Every hit still validates on
// the root and compares ALL original input/scale bits before publication.
struct PioneerQ38ActivationReuse {
	uint32_t activation[TT_Q38_COHORT_INPUT_WORDS]={},scales[TT_Q38_COHORT_SCALE_WORDS]={};
	uint32_t type=0,inputs=0,rootNonce=0;
	uint64_t epoch=0;
	bool valid=false;
	bool Matches(const tt_q38_dispatch_probe& job,uint32_t nonce,uint64_t currentEpoch) const {
#ifdef PIONEER_Q38_REUSE_CROSS_QUANT
		if (!valid || !currentEpoch || currentEpoch==UINT64_MAX || epoch!=currentEpoch
			|| rootNonce!=nonce || inputs!=job.inputs) return false;
		// Q4_K/Q6_K share the SAME256-input quantizer/layout. Weight format
		// is not input authority:all original input/padded-scale bits still
		// have to match. Q8_0's32-input scale geometry NEVER aliases them.
		if (PioneerQ38CohortBlockInputs(type)!=PioneerQ38CohortBlockInputs(job.type)) return false;
#else
		if (!valid || !currentEpoch || currentEpoch==UINT64_MAX || epoch!=currentEpoch
			|| rootNonce!=nonce || type!=job.type || inputs!=job.inputs) return false;
#endif
		bool same=true;
		for (unsigned w=0;w<inputs/2;w++) same &= activation[w]==job.activation[w];
		unsigned padded=(inputs/PioneerQ38CohortBlockInputs(type)+15)&~15U;
		for (unsigned w=0;w<padded;w++) same &= scales[w]==job.scales[w];
		return same; // exact comparison, not a hash or userspace assertion
	}
	void Commit(const tt_q38_dispatch_probe& job,uint32_t nonce,uint64_t currentEpoch) {
		valid=false;
		if (!currentEpoch || currentEpoch==UINT64_MAX) return; // saturation disables reuse
		type=job.type; inputs=job.inputs; rootNonce=nonce; epoch=currentEpoch;
		for (unsigned w=0;w<inputs/2;w++) activation[w]=job.activation[w];
		unsigned padded=(inputs/PioneerQ38CohortBlockInputs(type)+15)&~15U;
		for (unsigned w=0;w<padded;w++) scales[w]=job.scales[w];
		valid=true;
	}
};
#endif
