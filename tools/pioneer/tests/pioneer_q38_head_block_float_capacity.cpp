/* SPDX-License-Identifier: MIT */
// Fixed candidate's bank-capacity arithmetic only: not a relocated plan,
// allocation, source authority, uploaded data, executable kernel or device.
#define PIONEER_Q38_HYBRID_LAYOUT_DIAGNOSTIC 1
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include "../PioneerQ38HybridModelPlan.h"
#include <cassert>
#include <cstdio>
#include <memory>

int main(){
 auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->BuildHybridDiagnostic());
 uint64_t extra[7]={},reduced[7]={};unsigned rows=0,seen=0;
 for(unsigned s=0;s<7;s++){
  const auto* p=plan->ProjectionStripe(64,1,s);
  assert(p && p->kind==plan->WeightKind && p->layer==64 && p->role==1 && p->type==14
   && p->bank<7 && p->firstRow==rows && p->rows && !(p->rows%32) && p->rowBytes==4240);
  rows+=p->rows;seen|=1U<<p->bank;
  const uint64_t bfp4=(uint64_t(p->rows)*2880+4095)&~uint64_t(4095);
  const uint64_t bfp8=(uint64_t(p->rows)*5440+4095)&~uint64_t(4095);
  assert(bfp4<=p->allocated && bfp8>=p->allocated);
  reduced[p->bank]+=p->allocated-bfp4;extra[p->bank]+=bfp8-p->allocated;
 }
 assert(rows==248320 && seen==127);
 bool fits=true;
 for(unsigned b=0;b<7;b++){
  assert(plan->Used(b)<=plan->Capacity && reduced[b]<=plan->Used(b));
  const uint64_t free=plan->Capacity-plan->Used(b);fits&=extra[b]<=free;
  printf("bank %u used=%llu free=%llu bfp8_extra=%llu bfp4_saved=%llu bfp8_fits=%u\n",b,
   (unsigned long long)plan->Used(b),(unsigned long long)free,(unsigned long long)extra[b],
   (unsigned long long)reduced[b],unsigned(extra[b]<=free));
 }
 printf("RESULT all_bfp8_banks_fit=%u; arithmetic only, full repack required, NO allocation or device\n",unsigned(fits));
 // Recompute the COMPLETE seven-bank plan while lowering the FFN BFP8 mask.
 // This does not imply those changed FFN formats are numerically acceptable.
 auto original=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(original->Build());
 for(unsigned drop=0;drop<=3;drop++){
  uint64_t mask=UINT64_C(0x1fff000000000000);
  for(unsigned layer=48;layer<48+drop;layer++)mask&=~(UINT64_C(1)<<layer);
  PioneerQ38HybridModelPlan::Proposal proposal;assert(PioneerQ38HybridModelPlan::Build(*original,mask,proposal));
  uint64_t needed[7]={};unsigned headStripes=0;
  for(unsigned i=0;i<original->Count();i++){
   const auto* p=original->Part(i);assert(p);
   if(p->kind!=original->WeightKind || p->layer!=64 || p->role!=1)continue;
   assert(p->bank<7 && proposal.parts[i].allocated==p->allocated);
   const uint64_t newSize=(uint64_t(p->rows)*5440+4095)&~uint64_t(4095);
   assert(newSize>=p->allocated);needed[p->bank]+=newSize-p->allocated;headStripes++;
  }
  assert(headStripes==7);bool sameBankFits=true;
  for(unsigned b=0;b<7;b++)sameBankFits &= needed[b]<=original->Capacity-proposal.used[b];
  if(!drop)assert(!sameBankFits);
  printf("ALTERNATIVE dropped_layers=%u mask=%016llx head_bfp8_same_bank_capacity=%u; DATA_ONLY not a numeric qualification\n",
   drop,(unsigned long long)mask,unsigned(sameBankFits));
 }
}
