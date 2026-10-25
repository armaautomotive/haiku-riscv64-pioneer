/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE) || PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE != 1 || !defined(PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE) || PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE != 1 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE)
#error Exact private native65 SAME native63 layer1 FFN consumer required
#endif
#ifndef PIONEER_Q38_NEXT_FFN_HIDDEN_PRODUCER_H
#define PIONEER_Q38_NEXT_FFN_HIDDEN_PRODUCER_H
#include "PioneerQ38GdnIdentity.h"
class PioneerQ38ResidentDecoderOwner;class PioneerQ38CanonicalModelPlan;
class PioneerQ38NextFfnProducer;class PioneerQ38NextFfnResidualState;
struct PioneerQ38DispatchLease;
// Completed layer1 hidden remains in GDDR at workspace+0. The old raw
// residual has been consumed/overwritten;it is NOT a layer2 RMS receipt.
// Only the SAME decoder can publish this lineage after real execution,
// independent119 STOP receipts,ALL120 holds and one bounded deadline.
class PioneerQ38NextFfnHiddenProducer {
 friend class PioneerQ38ResidentDecoderOwner;
 const PioneerQ38ResidentDecoderOwner* owner=nullptr;
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 const PioneerQ38NextFfnProducer* parent=nullptr;
 const PioneerQ38NextFfnResidualState* state=nullptr;
 const uint8_t* ledger=nullptr;PioneerQ38GdnIdentity identity={};
 unsigned pages=0,token=UINT32_MAX,position=UINT32_MAX,bank=7,phase=0,nextLayer=64;
 uint32_t hiddenAddress=0,source[16]={};
 PioneerQ38NextFfnHiddenProducer()=default;
public:
 PioneerQ38NextFfnHiddenProducer(const PioneerQ38NextFfnHiddenProducer&)=delete;
 PioneerQ38NextFfnHiddenProducer& operator=(const PioneerQ38NextFfnHiddenProducer&)=delete;
 bool Current()const;bool MatchesCohort(const PioneerQ38DispatchLease&)const;
};
#endif
