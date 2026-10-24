/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_HIDDEN_PRODUCER_H
#define PIONEER_Q38_FFN_HIDDEN_PRODUCER_H
#if !defined(PIONEER_Q38_FFN_RESIDUAL_NATIVE) || PIONEER_Q38_FFN_RESIDUAL_NATIVE != 1 || !defined(PIONEER_Q38_RESIDUAL_RMS_NATIVE) || PIONEER_Q38_RESIDUAL_RMS_NATIVE != 1 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE)
#error Separate exact SAME-owner native57 hidden producer only
#endif
#include "PioneerQ38GdnIdentity.h"
class PioneerQ38ResidentDecoderOwner;
class PioneerQ38CanonicalModelPlan;
class PioneerQ38PostAttentionProducer;
class PioneerQ38FfnResidualState;
struct PioneerQ38DispatchLease;
// Lives INSIDE SAME actual decoder. This is completed layer0 hidden at+0,
// NOT an unchanged residual at+0x18000 (FFN scratch has overwritten that).
// Next-layer RMS must create its own fresh residual producer. Current is
// a CPU relation ONLY;actual scoped native/physical checks still required.
class PioneerQ38FfnHiddenProducer {
 friend class PioneerQ38ResidentDecoderOwner;
 const PioneerQ38ResidentDecoderOwner* owner=nullptr;
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 const PioneerQ38PostAttentionProducer* parent=nullptr;
 const PioneerQ38FfnResidualState* state=nullptr;
 const uint8_t* ledger=nullptr;
 PioneerQ38GdnIdentity identity={};
 unsigned pages=0,token=UINT32_MAX,position=UINT32_MAX,bank=7,phase=0,nextLayer=64;
 uint32_t hiddenAddress=0,source[16]={};
 PioneerQ38FfnHiddenProducer()=default;
public:
 PioneerQ38FfnHiddenProducer(const PioneerQ38FfnHiddenProducer&)=delete;
 PioneerQ38FfnHiddenProducer& operator=(const PioneerQ38FfnHiddenProducer&)=delete;
 bool Current()const;
 bool MatchesCohort(const PioneerQ38DispatchLease&)const;
};
#endif
