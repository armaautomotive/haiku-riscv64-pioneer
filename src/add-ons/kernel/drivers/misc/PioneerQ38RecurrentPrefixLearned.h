/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RECURRENT_PREFIX_LEARNED_H
#define PIONEER_Q38_RECURRENT_PREFIX_LEARNED_H
#include "PioneerQ38CanonicalModelPlan.h"
// DATA coverage of ALL three recurrent layers, not the old layer0 helper.
// No upload authentication, completed embedding, initialized state or IO grant.
static inline bool PioneerQ38RecurrentPrefixLearned(const PioneerQ38CanonicalModelPlan& plan,
 const uint8_t* ledger,unsigned pages)
{
 if(!plan.Ready() || !ledger || pages!=PioneerResidentBanks::Pages)return false;
 unsigned parts[3]={};uint64_t bytes[3]={};
 for(unsigned i=0;i<plan.Count();i++){
  const auto* p=plan.Part(i);
  if(!p)return false;
  if(p->kind!=plan.WeightKind || p->layer>=3)continue;
  if(p->role>13 || p->bank>=7 || (p->offset&4095) || (p->allocated&4095) || !p->allocated
   || !p->bytes || p->bytes>p->allocated || p->offset>plan.Capacity || p->allocated>plan.Capacity-p->offset)return false;
  const unsigned first=p->bank*unsigned(plan.Capacity/4096)+p->offset/4096,count=p->allocated/4096;
  if(first>=pages || count>pages-first)return false;
  for(unsigned page=0;page<count;page++)if(ledger[first+page]!=1)return false;
  parts[p->layer]++;bytes[p->layer]+=p->bytes;
 }
 return parts[0]==56 && parts[1]==56 && parts[2]==56 && bytes[0]==280925056
  && bytes[1]==bytes[0] && bytes[2]==bytes[0];
}
#endif
