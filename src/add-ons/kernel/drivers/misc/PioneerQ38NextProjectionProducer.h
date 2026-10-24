/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NEXT_PROJECTION_PRODUCER_H
#define PIONEER_Q38_NEXT_PROJECTION_PRODUCER_H
#if !defined(PIONEER_Q38_NEXT_PROJECTION_NATIVE) || PIONEER_Q38_NEXT_PROJECTION_NATIVE != 1 || !defined(PIONEER_Q38_NEXT_RMS_NATIVE) || PIONEER_Q38_NEXT_RMS_NATIVE != 1 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_PROJECTION_NEXT_ROOT)
#error Exact SAME native59 only,never image aliases/CPU reference/Work
#endif
#include "PioneerQ38GdnIdentity.h"
class PioneerQ38ResidentDecoderOwner;
class PioneerQ38NextProjectionState;
class PioneerQ38NextRmsProducer;
class PioneerQ38CanonicalModelPlan;
struct PioneerQ38DispatchLease;
// Borrowed ALL4 completed on-card projections+unchanged fresh residual.
// Not an old ProjectionGraphOwner/initial RMS facade or a public launch grant.
class PioneerQ38NextProjectionProducer {
 friend class PioneerQ38ResidentDecoderOwner;
 const PioneerQ38ResidentDecoderOwner* owner=nullptr;
 const PioneerQ38NextProjectionState* state=nullptr;
 const PioneerQ38NextRmsProducer* parent=nullptr;
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 const uint8_t* ledger=nullptr;
 PioneerQ38GdnIdentity identity={};
 unsigned pages=0,token=UINT32_MAX,position=UINT32_MAX,bank=7,layer=64,phase=0;
 uint32_t resultAddress=0,residualAddress=0,source[16]={};
 PioneerQ38NextProjectionProducer()=default;
public:
 PioneerQ38NextProjectionProducer(const PioneerQ38NextProjectionProducer&)=delete;
 PioneerQ38NextProjectionProducer& operator=(const PioneerQ38NextProjectionProducer&)=delete;
 bool Current()const;bool MatchesCohort(const PioneerQ38DispatchLease&)const;
};
#endif
