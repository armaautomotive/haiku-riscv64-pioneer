/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NEXT_FFN_PRODUCER_H
#define PIONEER_Q38_NEXT_FFN_PRODUCER_H
#if !defined(PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE) || PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE != 1 || !defined(PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE) || PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE != 1 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE)
#error Exact private native63 consumer of SAME genuine native62 required
#endif
#include "PioneerQ38GdnIdentity.h"
class PioneerQ38ResidentDecoderOwner;class PioneerQ38NextResidualRmsState;
class PioneerQ38NextGdnOutputProducer;class PioneerQ38CanonicalModelPlan;struct PioneerQ38DispatchLease;
// SAME decoder alone publishes layer1 FFN input after real QRR2+ROOT hold.
// Raw residual is the new sum in GDDR,not a retagged initial RMS lineage.
class PioneerQ38NextFfnProducer {
 friend class PioneerQ38ResidentDecoderOwner;
 const PioneerQ38ResidentDecoderOwner* owner=nullptr;const PioneerQ38NextResidualRmsState* state=nullptr;
 const PioneerQ38NextGdnOutputProducer* parent=nullptr;const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 const uint8_t* ledger=nullptr;PioneerQ38GdnIdentity identity={};
 unsigned pages=0,token=UINT32_MAX,position=UINT32_MAX,bank=7,layer=64,phase=0;
 uint32_t normalizedAddress=0,residualAddress=0,source[16]={};
 PioneerQ38NextFfnProducer()=default;
public:
 PioneerQ38NextFfnProducer(const PioneerQ38NextFfnProducer&)=delete;
 PioneerQ38NextFfnProducer& operator=(const PioneerQ38NextFfnProducer&)=delete;
 bool Current()const;bool MatchesCohort(const PioneerQ38DispatchLease&)const;
};
#endif
