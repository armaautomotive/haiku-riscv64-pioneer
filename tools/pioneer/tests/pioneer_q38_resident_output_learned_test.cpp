/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputLearned.h"
#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>
int main(){
 auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();
 std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,0);
 assert(!PioneerQ38ResidentOutputLearned(*plan,ledger.data(),unsigned(ledger.size())) && plan->Build());
 assert(!PioneerQ38ResidentOutputLearned(*plan,nullptr,unsigned(ledger.size()))
  && !PioneerQ38ResidentOutputLearned(*plan,ledger.data(),unsigned(ledger.size())-1)
  && !PioneerQ38ResidentOutputLearned(*plan,ledger.data(),unsigned(ledger.size())+1));
 unsigned count=0,pages=0,ffn=0;
 for(unsigned i=0;i<plan->Count();i++){
  const auto* p=plan->Part(i);if(p->kind!=plan->WeightKind || p->layer || p->role>13)continue;
  const unsigned first=p->bank*unsigned(plan->Capacity/4096)+p->offset/4096;
  for(unsigned page=0;page<p->allocated/4096;page++){assert(!ledger[first+page]);ledger[first+page]=1;pages++;}
  if(p->role>=1 && p->role<=4)ffn+=p->allocated/4096;
  count++;
 }
 assert(count==56 && pages==68591 && ffn==36725
  && PioneerQ38ResidentOutputLearned(*plan,ledger.data(),unsigned(ledger.size())));
 const auto original=ledger;unsigned refused=0;
 for(unsigned i=0;i<plan->Count();i++){
  const auto* p=plan->Part(i);if(p->kind!=plan->WeightKind || p->layer || p->role>13)continue;
  const unsigned first=p->bank*unsigned(plan->Capacity/4096)+p->offset/4096,n=p->allocated/4096;
  for(unsigned at:{0U,n/2,n-1})for(uint8_t bad:{uint8_t(0),uint8_t(2),uint8_t(255)}){
   ledger[first+at]=bad;
   assert(!PioneerQ38ResidentOutputLearned(*plan,ledger.data(),unsigned(ledger.size())) && ledger[first+at]==bad);
   ledger[first+at]=1;refused++;
  }
 }
 assert(ledger==original && refused==504);
 // Reproduce the incomplete new diagnostic fixture: its numerically unused
 // FFN/post-norm stripes still cannot bypass the native handoff contract.
 for(unsigned i=0;i<plan->Count();i++){
  const auto* p=plan->Part(i);if(p->kind!=plan->WeightKind || p->layer || p->role<1 || p->role>4)continue;
  const unsigned first=p->bank*unsigned(plan->Capacity/4096)+p->offset/4096;
  for(unsigned page=0;page<p->allocated/4096;page++)ledger[first+page]=0;
 }
 assert(!PioneerQ38ResidentOutputLearned(*plan,ledger.data(),unsigned(ledger.size())));
 printf("PASS DATA complete layer0 learned preflight:56parts/68591pages/36725FFN+postnorm pages;504first-middle-last bad-presence refusals;incomplete prior fixture refuses;NO IO/epoch/owner/native or numeric grant\n");
}
