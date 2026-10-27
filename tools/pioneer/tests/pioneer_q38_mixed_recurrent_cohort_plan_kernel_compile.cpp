/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38MixedRecurrentCohortPlan.h"
extern "C" bool pioneer_q38_cohort_plan_build(PioneerQ38MixedRecurrentCohortPlan& heap,const PioneerQ38CanonicalModelPlan& canonical,
 unsigned layer,unsigned position,unsigned bank,unsigned slot,unsigned token,unsigned phase,unsigned request,
 const PioneerQ38GdnIdentity& id,const uint32_t* sources){
 return heap.Build(canonical,layer,position,bank,slot,token,phase,request,id,sources);
}
extern "C" bool pioneer_q38_cohort_plan_match(const PioneerQ38MixedRecurrentCohortPlan& heap,const PioneerQ38CanonicalModelPlan& canonical){
 return heap.Matches(canonical);
}
extern "C" bool pioneer_q38_cohort_plan_pages(const PioneerQ38MixedRecurrentCohortPlan& heap,const uint8_t* ledger,unsigned pages){
 return heap.LearnedPagesPresent(ledger,pages);
}
