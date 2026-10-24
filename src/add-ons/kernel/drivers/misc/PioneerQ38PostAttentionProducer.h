/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_POST_ATTENTION_PRODUCER_H
#define PIONEER_Q38_POST_ATTENTION_PRODUCER_H
#if !defined(PIONEER_Q38_RESIDUAL_RMS_NATIVE) || PIONEER_Q38_RESIDUAL_RMS_NATIVE != 1 || !defined(PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE) || PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE != 1 || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE)
#error Exact separate SAME-owner native56 post-attention producer only
#endif
#include "PioneerQ38GdnIdentity.h"
class PioneerQ38ResidentDecoderOwner;
class PioneerQ38CanonicalModelPlan;
class PioneerQ38LearnedRmsController;
class PioneerQ38ResidualRmsState;
struct PioneerQ38DispatchLease;
// Lives INSIDE the real decoder. Private publication binds BOTH original
// RMS residual lineage and actual completed QOP1/QRR1 execution. Metadata
// or copied result words cannot manufacture it. Current is a CPU relation,
// NOT a mapping/launch/restore grant;future FFN consumer still owes Scope.
class PioneerQ38PostAttentionProducer {
 friend class PioneerQ38ResidentDecoderOwner;
 const PioneerQ38ResidentDecoderOwner* owner=nullptr;
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 const PioneerQ38LearnedRmsController* raw=nullptr;
 const PioneerQ38ResidualRmsState* state=nullptr;
 const uint8_t* ledger=nullptr;
 PioneerQ38GdnIdentity identity={};
 unsigned pages=0,token=UINT32_MAX,position=UINT32_MAX,bank=7,phase=0;
 uint32_t normalizedAddress=0,residualAddress=0,source[16]={};
 PioneerQ38PostAttentionProducer()=default;
public:
 PioneerQ38PostAttentionProducer(const PioneerQ38PostAttentionProducer&)=delete;
 PioneerQ38PostAttentionProducer& operator=(const PioneerQ38PostAttentionProducer&)=delete;
 bool Current()const;
 bool MatchesCohort(const PioneerQ38DispatchLease&)const;
};
#endif
