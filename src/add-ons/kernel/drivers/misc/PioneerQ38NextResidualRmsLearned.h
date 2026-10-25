/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NEXT_RESIDUAL_RMS_LEARNED_H
#define PIONEER_Q38_NEXT_RESIDUAL_RMS_LEARNED_H
#include "PioneerQ38CanonicalModelPlan.h"
// Exact ORIGINAL layer1 F32 post-attention norm upload DATA,not a source,
// completed output/raw-residual producer,initialization or launch grant.
static __attribute__((noinline)) bool PioneerQ38NextResidualRmsLearned(const PioneerQ38CanonicalModelPlan& plan,const uint8_t* ledger,unsigned pages){
 if(!plan.Ready() || !ledger || pages!=PioneerResidentBanks::Pages)return false;
 unsigned found=0;
 for(unsigned i=0;i<plan.Count();i++){
  const auto* p=plan.Part(i);if(p->kind!=plan.WeightKind || p->layer!=1 || p->role!=1)continue;
  if(found++ || p->type || p->bank>=7 || p->firstRow || p->rows!=1 || p->rowBytes!=20480 || p->bytes!=20480
   || p->allocated!=20480 || (p->offset&4095) || p->offset>plan.Capacity || p->allocated>plan.Capacity-p->offset)return false;
  unsigned first=p->bank*unsigned(plan.Capacity/4096)+p->offset/4096;
  if(first>=pages || pages-first<5)return false;
  for(unsigned page=0;page<5;page++)if(ledger[first+page]!=1)return false;
 }
 return found==1;
}
#endif
