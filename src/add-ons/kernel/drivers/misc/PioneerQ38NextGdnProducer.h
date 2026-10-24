/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NEXT_GDN_PRODUCER_H
#define PIONEER_Q38_NEXT_GDN_PRODUCER_H
#if !defined(PIONEER_Q38_NEXT_GDN_NATIVE) || PIONEER_Q38_NEXT_GDN_NATIVE != 1 || !defined(PIONEER_Q38_NEXT_CONV_NATIVE) || PIONEER_Q38_NEXT_CONV_NATIVE != 1 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_PROJECTION_NEXT_ROOT)
#error Separate exact native61 only,never CPU reference/Work/image alias
#endif
#include "PioneerQ38GdnIdentity.h"
class PioneerQ38ResidentDecoderOwner;class PioneerQ38NextGdnState;
class PioneerQ38NextConvProducer;class PioneerQ38CanonicalModelPlan;struct PioneerQ38DispatchLease;
// Private layer1 gated output and retained raw residual ONLY. Neither
// canonical current history/state nor token position has been committed.
class PioneerQ38NextGdnProducer {
 friend class PioneerQ38ResidentDecoderOwner;
 const PioneerQ38ResidentDecoderOwner* owner=nullptr;const PioneerQ38NextGdnState* state=nullptr;
 const PioneerQ38NextConvProducer* parent=nullptr;const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 const uint8_t* ledger=nullptr;PioneerQ38GdnIdentity identity={};
 unsigned pages=0,token=UINT32_MAX,position=UINT32_MAX,bank=7,layer=64,phase=0;
 uint32_t inputAddress=0,residualAddress=0,source[16]={};
 PioneerQ38NextGdnProducer()=default;
public:
 PioneerQ38NextGdnProducer(const PioneerQ38NextGdnProducer&)=delete;
 PioneerQ38NextGdnProducer& operator=(const PioneerQ38NextGdnProducer&)=delete;
 bool Current()const;bool MatchesCohort(const PioneerQ38DispatchLease&)const;
};
#endif
