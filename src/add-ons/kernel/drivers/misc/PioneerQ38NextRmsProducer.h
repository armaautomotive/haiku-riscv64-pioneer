/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NEXT_RMS_PRODUCER_H
#define PIONEER_Q38_NEXT_RMS_PRODUCER_H
#if !defined(PIONEER_Q38_NEXT_RMS_NATIVE) || PIONEER_Q38_NEXT_RMS_NATIVE != 1 || !defined(PIONEER_Q38_FFN_RESIDUAL_NATIVE) || PIONEER_Q38_FFN_RESIDUAL_NATIVE != 1 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE)
#error Exact SAME-owner native58 next-layer norm only
#endif
#include "PioneerQ38GdnIdentity.h"
class PioneerQ38ResidentDecoderOwner;
class PioneerQ38CanonicalModelPlan;
class PioneerQ38FfnHiddenProducer;
class PioneerQ38NextRmsState;
struct PioneerQ38DispatchLease;
// A borrowed completed normalized input AND freshly preserved raw residual.
// Neither is an old initial-RMS/FFN scratch producer or a public launch grant.
class PioneerQ38NextRmsProducer {
 friend class PioneerQ38ResidentDecoderOwner;
 const PioneerQ38ResidentDecoderOwner* owner=nullptr;
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 const PioneerQ38FfnHiddenProducer* parent=nullptr;
 const PioneerQ38NextRmsState* state=nullptr;
 const uint8_t* ledger=nullptr;
 PioneerQ38GdnIdentity identity={};
 unsigned pages=0,token=UINT32_MAX,position=UINT32_MAX,bank=7,layer=64,phase=0;
 uint32_t normalizedAddress=0,residualAddress=0,source[16]={};
 PioneerQ38NextRmsProducer()=default;
public:
 PioneerQ38NextRmsProducer(const PioneerQ38NextRmsProducer&)=delete;
 PioneerQ38NextRmsProducer& operator=(const PioneerQ38NextRmsProducer&)=delete;
 bool Current()const;
 bool MatchesCohort(const PioneerQ38DispatchLease&)const;
};
#endif
