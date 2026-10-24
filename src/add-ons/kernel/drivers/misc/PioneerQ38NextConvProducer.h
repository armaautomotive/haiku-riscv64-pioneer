/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NEXT_CONV_PRODUCER_H
#define PIONEER_Q38_NEXT_CONV_PRODUCER_H
#if !defined(PIONEER_Q38_NEXT_CONV_NATIVE) || PIONEER_Q38_NEXT_CONV_NATIVE != 1 || !defined(PIONEER_Q38_NEXT_PROJECTION_NATIVE) || PIONEER_Q38_NEXT_PROJECTION_NATIVE != 1 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_PROJECTION_NEXT_ROOT)
#error Exact SAME native60 only,never CPU reference/Work/image facade
#endif
#include "PioneerQ38GdnIdentity.h"
class PioneerQ38ResidentDecoderOwner;
class PioneerQ38NextConvState;
class PioneerQ38NextProjectionProducer;
class PioneerQ38CanonicalModelPlan;
struct PioneerQ38DispatchLease;
// Borrowed layer1 convolution and PRIVATE next-history workspace. Neither
// canonical current history nor GDN/token state has been committed.
class PioneerQ38NextConvProducer {
 friend class PioneerQ38ResidentDecoderOwner;
 const PioneerQ38ResidentDecoderOwner* owner=nullptr;
 const PioneerQ38NextConvState* state=nullptr;
 const PioneerQ38NextProjectionProducer* parent=nullptr;
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 const uint8_t* ledger=nullptr;
 PioneerQ38GdnIdentity identity={};
 unsigned pages=0,token=UINT32_MAX,position=UINT32_MAX,bank=7,layer=64,phase=0;
 uint32_t outputAddress=0,nextHistoryAddress=0,residualAddress=0,source[16]={};
 PioneerQ38NextConvProducer()=default;
public:
 PioneerQ38NextConvProducer(const PioneerQ38NextConvProducer&)=delete;
 PioneerQ38NextConvProducer& operator=(const PioneerQ38NextConvProducer&)=delete;
 bool Current()const;bool MatchesCohort(const PioneerQ38DispatchLease&)const;
};
#endif
