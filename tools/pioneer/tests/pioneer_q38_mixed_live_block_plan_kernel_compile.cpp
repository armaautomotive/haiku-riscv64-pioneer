/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38MixedLiveBlockPlan.h"
extern "C" bool pioneer_q38_live_plan_build(PioneerQ38MixedLiveBlockPlan& heap,const PioneerQ38CanonicalModelPlan& canonical,
 unsigned layer,unsigned position,unsigned token,unsigned bank,unsigned phase,unsigned request,const PioneerQ38GdnIdentity& id,
 const uint32_t* sources,unsigned inputBank,uint32_t inputAddress){
 return heap.Build(canonical,layer,position,token,bank,phase,request,id,sources,inputBank,inputAddress);
}
extern "C" bool pioneer_q38_live_plan_match(const PioneerQ38MixedLiveBlockPlan& heap,const PioneerQ38CanonicalModelPlan& canonical){
 return heap.Matches(canonical);
}
extern "C" bool pioneer_q38_live_plan_pages(const PioneerQ38MixedLiveBlockPlan& heap,const uint8_t* ledger,unsigned pages){
 return heap.LearnedPagesPresent(ledger,pages);
}
