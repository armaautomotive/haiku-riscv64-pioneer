/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RECURRENT_PREFIX_COUNTERS_H
#define PIONEER_Q38_RECURRENT_PREFIX_COUNTERS_H
#include "PioneerQ38RecurrentPrefixPlan.h"
#include "PioneerQ38MixedRecurrentBlockCounters.h"
// Cumulative immutable packet arithmetic ONLY. No native/source/STOP grant.
// Three genuine GDN rendezvous counts each lie in1..256; the aggregate does
// not pretend to recover their individual values or substitute a GPU receipt.
static inline bool PioneerQ38RecurrentPrefixCounterDelta(const PioneerQ38RecurrentPrefixPlan& plan,
 const PioneerQ38CanonicalModelPlan& canonical,unsigned worker,unsigned polls,bool beforeLastDown,uint32_t* output){
 if(!output || !worker || worker>=120 || !plan.Matches(canonical)
  || (worker<=96?(polls<3 || polls>3*TT_Q38_GDN_GATED_POLLS):polls!=0))return false;
 uint32_t stage[8]={};
 for(unsigned layer=0;layer<3;layer++){
  uint32_t delta[8];if(!PioneerQ38RecurrentBlockCounterDelta(*plan.Layer(layer),canonical,worker,worker<=96?1:0,beforeLastDown && layer==2,delta))return false;
  for(unsigned i=0;i<8;i++)stage[i]+=delta[i];
 }
 if(worker<=96)for(unsigned i=0;i<8;i++)if(i==0 || i==1 || i==2 || i==3 || i==6)stage[i]+=polls-3;
 for(unsigned i=0;i<8;i++)output[i]=stage[i];
 return true;
}
static inline bool PioneerQ38RecurrentPrefixCounterProof(const PioneerQ38RecurrentPrefixPlan& plan,
 const PioneerQ38CanonicalModelPlan& canonical,unsigned worker,const uint32_t* before,const uint32_t* after,
 bool beforeLastDown,unsigned& inferredPolls){
 if(!before || !after || !worker || worker>=120 || before[2]!=before[6] || after[2]!=after[6])return false;
 uint32_t delta[8];unsigned polls=worker<=96?3:0;
 if(!PioneerQ38RecurrentPrefixCounterDelta(plan,canonical,worker,polls,beforeLastDown,delta))return false;
 if(worker<=96){
  polls=uint32_t(after[0]-before[0])-delta[0]+3;
  if(polls<3 || polls>3*TT_Q38_GDN_GATED_POLLS
   || !PioneerQ38RecurrentPrefixCounterDelta(plan,canonical,worker,polls,beforeLastDown,delta))return false;
 }
 for(unsigned i=0;i<8;i++)if(uint32_t(after[i]-before[i])!=delta[i])return false;
 inferredPolls=polls;return true; // output untouched on refusal, NOT numerical/lifetime proof
}
#endif
