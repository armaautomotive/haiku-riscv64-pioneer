/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38MixedRecurrentBlockPlan.h"
extern "C" bool pioneer_q38_recurrent_block_plan_build(PioneerQ38MixedRecurrentBlockPlan& heap,const PioneerQ38CanonicalModelPlan& c,
 unsigned layer,unsigned pos,unsigned token,unsigned bank,unsigned slot,unsigned phase,unsigned request,
 const PioneerQ38GdnIdentity& id,const uint32_t* sources,unsigned inputBank,unsigned inputAddress){
 return heap.Build(c,layer,pos,token,bank,slot,phase,request,id,sources,inputBank,inputAddress);
}
extern "C" bool pioneer_q38_recurrent_block_plan_match(const PioneerQ38MixedRecurrentBlockPlan& heap,const PioneerQ38CanonicalModelPlan& c){return heap.Matches(c);}
extern "C" bool pioneer_q38_recurrent_block_plan_pages(const PioneerQ38MixedRecurrentBlockPlan& heap,const uint8_t* pages,unsigned count){return heap.LearnedPagesPresent(pages,count);}
