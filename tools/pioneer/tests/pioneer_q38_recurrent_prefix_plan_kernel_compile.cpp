/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../PioneerQ38RecurrentPrefixPlan.h"
extern "C" bool pioneer_q38_prefix_plan_build(PioneerQ38RecurrentPrefixPlan& plan,const PioneerQ38CanonicalModelPlan& c,
 unsigned bank,const PioneerQ38GdnIdentity& id,const uint32_t* sources){return plan.Build(c,bank,id,sources);}
extern "C" bool pioneer_q38_prefix_plan_match(const PioneerQ38RecurrentPrefixPlan& plan,const PioneerQ38CanonicalModelPlan& c,
 unsigned bank,const PioneerQ38GdnIdentity& id,const uint32_t* sources){return plan.MatchesBinding(c,bank,id,sources);}
extern "C" bool pioneer_q38_prefix_plan_read(const PioneerQ38RecurrentPrefixPlan& plan,unsigned layer,unsigned offset,uint8_t* bytes,unsigned count){
 return plan.ReadPackage(layer,offset,bytes,count);
}
extern "C" bool pioneer_q38_prefix_plan_ledger(const PioneerQ38RecurrentPrefixPlan& plan,const uint8_t* ledger,unsigned count){
 return plan.LearnedPagesPresent(ledger,count);
}
