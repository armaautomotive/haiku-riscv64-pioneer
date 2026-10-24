/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NEXT_GDN_OUTPUT_LEARNED_H
#define PIONEER_Q38_NEXT_GDN_OUTPUT_LEARNED_H
#include "PioneerQ38CanonicalModelPlan.h"
// Original layer1 ssm_out page-ledger DATA only; no numeric products,
// source/producer grant, device IO, or initialized/token publication.
static inline bool PioneerQ38NextGdnOutputLearned(const PioneerQ38CanonicalModelPlan& plan,const uint8_t* ledger,unsigned pages){
 if(!plan.Ready() || !ledger || pages!=PioneerResidentBanks::Pages)return false;
 unsigned stripes=0,rows=0,mask=0,total=0;
 for(unsigned i=0;i<plan.Count();i++){
  const auto* p=plan.Part(i);if(p->kind!=plan.WeightKind || p->layer!=1 || p->role!=13)continue;
  if(p->type!=8 || p->rowBytes!=6912 || p->bank>=7 || p->firstRow!=rows || !p->rows || p->rows%16
   || p->bytes!=p->rows*6912 || !p->allocated || p->allocated%4096 || p->offset%4096
   || p->offset>=plan.Capacity || p->allocated>plan.Capacity-p->offset || (mask&(1U<<p->bank)))return false;
  unsigned first=p->bank*unsigned(plan.Capacity/4096)+p->offset/4096,count=p->allocated/4096;
  if(first>pages || count>pages-first)return false;
  for(unsigned page=0;page<count;page++)if(ledger[first+page]!=1)return false;
  stripes++;rows+=p->rows;total+=count;mask|=1U<<p->bank;
 }
 return stripes==7 && rows==5120 && total==8640 && mask==127;
}
#endif
