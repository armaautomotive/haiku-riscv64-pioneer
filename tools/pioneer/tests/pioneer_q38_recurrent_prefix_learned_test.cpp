/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38RecurrentPrefixLearned.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputLearned.h"
#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>
int main(){
 auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,0);
 assert(!PioneerQ38RecurrentPrefixLearned(*plan,ledger.data(),unsigned(ledger.size())) && plan->Build());
 assert(!PioneerQ38RecurrentPrefixLearned(*plan,nullptr,unsigned(ledger.size()))
  && !PioneerQ38RecurrentPrefixLearned(*plan,ledger.data(),unsigned(ledger.size())-1));
 unsigned parts[3]={},pages=0,refused=3;uint64_t bytes[3]={};
 for(unsigned layer=0;layer<3;layer++){
  for(unsigned i=0;i<plan->Count();i++){
   const auto& p=*plan->Part(i);if(p.kind!=plan->WeightKind || p.layer!=layer)continue;
   parts[layer]++;bytes[layer]+=p.bytes;
   const unsigned first=p.bank*unsigned(plan->Capacity/4096)+p.offset/4096,count=p.allocated/4096;
   for(unsigned at=0;at<count;at++){assert(!ledger[first+at]);ledger[first+at]=1;pages++;}
  }
  if(layer<2){assert(!PioneerQ38RecurrentPrefixLearned(*plan,ledger.data(),unsigned(ledger.size())));refused++;}
  if(!layer)assert(PioneerQ38ResidentOutputLearned(*plan,ledger.data(),unsigned(ledger.size())));
 }
 assert(parts[0]==56 && parts[1]==56 && parts[2]==56 && bytes[0]==280925056 && bytes[1]==bytes[0] && bytes[2]==bytes[0]);
 assert(PioneerQ38RecurrentPrefixLearned(*plan,ledger.data(),unsigned(ledger.size())));
 for(unsigned i=0;i<plan->Count();i++){
  const auto& p=*plan->Part(i);if(p.kind!=plan->WeightKind || p.layer>=3)continue;
  const unsigned first=p.bank*unsigned(plan->Capacity/4096)+p.offset/4096,count=p.allocated/4096;
  for(unsigned at:{0U,count/2,count-1})for(uint8_t bad:{uint8_t(0),uint8_t(2),uint8_t(255)}){
   ledger[first+at]=bad;assert(!PioneerQ38RecurrentPrefixLearned(*plan,ledger.data(),unsigned(ledger.size())));refused++;
   ledger[first+at]=1;assert(PioneerQ38RecurrentPrefixLearned(*plan,ledger.data(),unsigned(ledger.size())));
  }
 }
 printf("PASS ALL3 learned DATA coverage/168parts/%u allocated pages/842775168 raw bytes/refusals%u;old layer0 coverage NOT prefix permission,NO IO or source authentication\n",pages,refused);
}
