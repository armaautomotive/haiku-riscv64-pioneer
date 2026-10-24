/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NEXT_GDN_OUTPUT_PRODUCER_H
#define PIONEER_Q38_NEXT_GDN_OUTPUT_PRODUCER_H
#if !defined(PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE) || PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE != 1 || !defined(PIONEER_Q38_NEXT_GDN_NATIVE) || PIONEER_Q38_NEXT_GDN_NATIVE != 1 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE)
#error Exact private native62 retained ssm_out producer,not CPU/Work
#endif
#include "PioneerQ38GdnIdentity.h"
class PioneerQ38ResidentDecoderOwner;class PioneerQ38NextGdnOutputState;
class PioneerQ38NextGdnProducer;class PioneerQ38CanonicalModelPlan;struct PioneerQ38DispatchLease;
// Constructed ONLY by SAME decoder after actual QOP2/119STOP/120holds.
// Raw residual and result stay in GDDR; no current-state/token commit.
class PioneerQ38NextGdnOutputProducer {
 friend class PioneerQ38ResidentDecoderOwner;
 const PioneerQ38ResidentDecoderOwner* owner=nullptr;const PioneerQ38NextGdnOutputState* state=nullptr;
 const PioneerQ38NextGdnProducer* parent=nullptr;const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 const uint8_t* ledger=nullptr;PioneerQ38GdnIdentity identity={};
 unsigned pages=0,token=UINT32_MAX,position=UINT32_MAX,bank=7,layer=64,phase=0;
 uint32_t resultAddress=0,residualAddress=0,source[16]={};
 PioneerQ38NextGdnOutputProducer()=default;
public:
 PioneerQ38NextGdnOutputProducer(const PioneerQ38NextGdnOutputProducer&)=delete;
 PioneerQ38NextGdnOutputProducer& operator=(const PioneerQ38NextGdnOutputProducer&)=delete;
 bool Current()const;bool MatchesCohort(const PioneerQ38DispatchLease&)const;
};
#endif
