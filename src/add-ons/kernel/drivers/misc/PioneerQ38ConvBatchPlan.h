/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_CONV_BATCH_PLAN_H
#define PIONEER_Q38_CONV_BATCH_PLAN_H
#include "PioneerQ38ConvPlan.h"
#include "PioneerQ38ConvBatchShape.h"
// HEAP ONLY canonical immutable DATA recipe,not native source/initialized state/
// private QPS1 proof. Actual joint convolution-history reservation OWED.
class PioneerQ38ConvBatchPlan {
 const PioneerQ38CanonicalModelPlan* plan=nullptr;PioneerQ38ConvPlan jobs[6];
 uint32_t records[384]={};unsigned count=0;bool attempted=false,ready=false;
public:
 PioneerQ38ConvBatchPlan()=default;
 PioneerQ38ConvBatchPlan(const PioneerQ38ConvBatchPlan&)=delete;
 PioneerQ38ConvBatchPlan& operator=(const PioneerQ38ConvBatchPlan&)=delete;
 bool Build(const PioneerQ38CanonicalModelPlan& canonical,unsigned layer,unsigned position,unsigned bank,unsigned worker,
  const PioneerQ38GdnIdentity& id,const uint32_t* source,unsigned phase){
  if(attempted)return false;
  attempted=true;plan=&canonical;count=PioneerQ38ConvBatchCount(worker);if(!count)return false;
  for(unsigned j=0;j<count;j++){
   if(!jobs[j].Build(canonical,layer,(worker+j*120)*16,position,bank,worker,id,source,phase))return false;
   const auto* d=jobs[j].Descriptor();for(unsigned i=0;i<64;i++)records[j*64+i]=d[i];
  }
  ready=PioneerQ38ConvBatchShape(records);return ready;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& canonical,unsigned layer,unsigned position,unsigned bank,unsigned worker,
  const PioneerQ38GdnIdentity& id,const uint32_t* source,unsigned phase)const{
  if(!ready || plan!=&canonical || count!=PioneerQ38ConvBatchCount(worker) || !PioneerQ38ConvBatchShape(records))return false;
  for(unsigned j=0;j<count;j++){
   if(!jobs[j].Matches(canonical,layer,(worker+j*120)*16,position,bank,worker,id,source,phase))return false;
   const auto* d=jobs[j].Descriptor();for(unsigned i=0;i<64;i++)if(records[j*64+i]!=d[i])return false;
  }
  return true;
 }
 bool LearnedPagesPresent(const uint8_t* pages,unsigned size)const{
  if(!ready || !PioneerQ38ConvBatchShape(records))return false;
  for(unsigned j=0;j<count;j++)if(!jobs[j].LearnedPagesPresent(pages,size))return false;
  return true;
 }
 const uint32_t* Records()const{return ready?records:nullptr;}
 unsigned Count()const{return ready?count:0;}
};
static_assert(sizeof(PioneerQ38ConvBatchPlan)>3072 && sizeof(PioneerQ38ConvBatchPlan)<4096,"complete batch DATA recipe is HEAP ONLY");
#endif
