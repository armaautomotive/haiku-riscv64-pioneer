/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_RECURRENT_BLOCK_COUNTERS_H
#define PIONEER_Q38_MIXED_RECURRENT_BLOCK_COUNTERS_H
#include "PioneerQ38MixedRecurrentBlockPlan.h"
// Metadata arithmetic ONLY for the future native close proof. A GDN peer
// may issue1..256 genuine rendezvous polls. Never assume one poll or accept
// arbitrary current counters. All other packet deltas are fixed by the
// immutable complete-layer plan. No source/STOP/native/Work grant here.
static inline bool PioneerQ38RecurrentBlockCounterDelta(const PioneerQ38MixedRecurrentBlockPlan& plan,
 const PioneerQ38CanonicalModelPlan& canonical,unsigned worker,unsigned polls,bool beforeDown,uint32_t* output){
 if(!output || !worker || worker>=120 || !plan.Matches(canonical)
  || (worker<=96?(!polls || polls>TT_Q38_GDN_GATED_POLLS):polls!=0))return false;
 uint32_t delta[8]={};
 for(unsigned slot=0;slot<(beforeDown?7U:8U);slot++){
  const auto* d=plan.Recipe(slot);if(worker-1>=d[7])continue;
  const auto* p=d+80+(worker-1)*8;unsigned k=d[14],blocks=k>>(d[15]==8?5:8),tile=d[15]==8?576:2304;
  unsigned reads=p[2]*blocks+(k*2+16383)/16384+1;
  unsigned flits=p[2]*blocks*tile/64+(k*2+((blocks*4+63)&~63U))/64;
  for(unsigned i=0;i<8;i++)delta[i]+=i==1?flits:i==0 || i==2 || i==3 || i==6?reads:0;
 }
 unsigned groups=PioneerQ38ConvBatchCount(worker);
 const unsigned conv[]={groups*3,groups*8,groups*5,groups*3,groups*2,groups*4,groups*5,groups*2};
 for(unsigned i=0;i<8;i++)delta[i]+=conv[i];
 if(worker<=96){
  // Seven raw imports,2 CURRENT reads,2 NEXT writes;gated phase adds
  // polls+3 reads and1 write. Local CSR/ready publications are NOT NoC.
  const unsigned gdn[]={polls+12,polls+556,polls+15,polls+12,3,516,polls+15,3};
  for(unsigned i=0;i<8;i++)delta[i]+=gdn[i];
 }
 for(unsigned i=0;i<8;i++)output[i]=delta[i];
 return true;
}
static inline bool PioneerQ38RecurrentBlockCounterProof(const PioneerQ38MixedRecurrentBlockPlan& plan,
 const PioneerQ38CanonicalModelPlan& canonical,unsigned worker,const uint32_t* before,const uint32_t* after,
 bool beforeDown,unsigned& inferredPolls){
 if(!before || !after || !worker || worker>=120 || before[2]!=before[6] || after[2]!=after[6])return false;
 uint32_t delta[8];unsigned polls=worker<=96?1:0;
 if(!PioneerQ38RecurrentBlockCounterDelta(plan,canonical,worker,polls,beforeDown,delta))return false;
 if(worker<=96){
  // Modulo32-bit deltas are required even when the baseline wraps. The
  // independent finite poll bound prevents an arbitrary wrap from passing.
  polls=uint32_t(after[0]-before[0])-delta[0]+1;
  if(!polls || polls>TT_Q38_GDN_GATED_POLLS
   || !PioneerQ38RecurrentBlockCounterDelta(plan,canonical,worker,polls,beforeDown,delta))return false;
 }
 for(unsigned i=0;i<8;i++)if(uint32_t(after[i]-before[i])!=delta[i])return false;
 inferredPolls=polls;return true; // output unchanged on failure;not a numerical or execution receipt
}
#endif
