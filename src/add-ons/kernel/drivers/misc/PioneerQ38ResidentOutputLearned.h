/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_OUTPUT_LEARNED_H
#define PIONEER_Q38_RESIDENT_OUTPUT_LEARNED_H
#include "PioneerQ38CanonicalModelPlan.h"
// DATA preflight only. Match the existing native handoff's complete layer0
// upload requirement BEFORE publishing a model/epoch or initializing state.
// Presence is not authentication,initialization,ownership or an IO grant.
static inline bool PioneerQ38ResidentOutputLearned(const PioneerQ38CanonicalModelPlan& plan,
 const uint8_t* ledger,unsigned pages)
{
 if(!plan.Ready() || !ledger || pages!=PioneerResidentBanks::Pages)return false;
 unsigned parts=0;
 for(unsigned i=0;i<plan.Count();i++){
  const auto* p=plan.Part(i);
  if(p->kind!=plan.WeightKind || p->layer || p->role>13)continue;
  if(p->bank>=7 || (p->offset&4095) || (p->allocated&4095) || !p->allocated)return false;
  const unsigned first=p->bank*unsigned(plan.Capacity/4096)+p->offset/4096,count=p->allocated/4096;
  if(first>=pages || count>pages-first)return false;
  for(unsigned page=0;page<count;page++)if(ledger[first+page]!=1)return false;
  parts++;
 }
 return parts==56; // ALL first-layer norm/FFN/projection/Conv/GDN/output stripes
}
#endif
