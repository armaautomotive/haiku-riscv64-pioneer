/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38MixedRecurrentProjectionPlan.h"
extern "C" bool pioneer_q38_recurrent_plan_build(PioneerQ38MixedRecurrentProjectionPlan& heap,const PioneerQ38CanonicalModelPlan& canonical,
 unsigned layer,unsigned role,unsigned position,unsigned token,unsigned bank,unsigned phase,unsigned request,
 const PioneerQ38GdnIdentity& id,const uint32_t* sources){
 return heap.Build(canonical,layer,role,position,token,bank,phase,request,id,sources);
}
extern "C" bool pioneer_q38_recurrent_plan_match(const PioneerQ38MixedRecurrentProjectionPlan& heap,const PioneerQ38CanonicalModelPlan& canonical){
 return heap.Matches(canonical);
}
extern "C" bool pioneer_q38_recurrent_plan_pages(const PioneerQ38MixedRecurrentProjectionPlan& heap,const uint8_t* ledger,unsigned pages){
 return heap.LearnedPagesPresent(ledger,pages);
}
