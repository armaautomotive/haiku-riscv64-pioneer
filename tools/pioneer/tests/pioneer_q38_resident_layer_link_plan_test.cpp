/* SPDX-License-Identifier: MIT */
// Pure canonical DATA tests: never source, producer or native authority.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerLinkPlan.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>
int main(){
 auto c=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(c->Build());
 auto foreign=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(foreign->Build());
 std::array<uint32_t,1920> src;src.fill(0x16005);std::array<uint32_t,119> seq={};
 unsigned recipes=0,mutations=0,pages=0;
 for(unsigned layer=0;layer<63;layer++){
  if(layer%4==3)continue;
  for(unsigned bank:{0U,6U}){
   auto p=std::make_unique<PioneerQ38ResidentLayerPlan>();
   assert(p->Build(*c,layer,bank?8191:0,bank,bank?1:0,bank?248319:0,17,{1,2,3,4},src.data(),seq.data()));
   PioneerQ38ResidentLayerLinkPlan link;
   assert(link.Build(*c,*p) && link.Matches(*c,*p) && link.Descriptor()[4]==layer+1
    && link.Descriptor()[69]==layer+1 && link.Descriptor()[71]==0 && !link.Build(*c,*p) && !link.Matches(*foreign,*p));
   PioneerQ38ResidentLayerLinkPlan wrong;assert(!wrong.Build(*foreign,*p) && !wrong.Descriptor() && !wrong.Build(*c,*p));
   std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,0);const auto* d=link.Descriptor();const auto* n=d+64;
   unsigned first=n[10]*unsigned(c->Capacity/4096)+(n[11]-c->Base)/4096;
   for(unsigned i=0;i<5;i++)ledger[first+i]=1;
   assert(link.LearnedPagesPresent(ledger.data(),unsigned(ledger.size()))); // ONLY added norm, not ALL parent pages
   for(unsigned i=0;i<5;i++)for(unsigned bad:{0U,2U}){
    ledger[first+i]=uint8_t(bad);assert(!link.LearnedPagesPresent(ledger.data(),unsigned(ledger.size())));ledger[first+i]=1;pages++;
   }
   if(!layer && !bank){
    auto* edit=const_cast<uint32_t*>(d);
    for(unsigned i=0;i<128;i++){edit[i]^=1;assert(!link.Matches(*c,*p) && !link.LearnedPagesPresent(ledger.data(),unsigned(ledger.size())));edit[i]^=1;mutations++;}
    // Shape-valid shifted learned norm is not the retained canonical range.
    edit[75]+=4096;assert(PioneerQ38ResidentLayerLinkShape(d,p->LayerManifest()) && !link.Matches(*c,*p));edit[75]-=4096;
    assert(!link.LearnedPagesPresent(nullptr,unsigned(ledger.size())) && !link.LearnedPagesPresent(ledger.data(),unsigned(ledger.size()-1)));
   }
   assert(link.Matches(*c,*p));recipes++;
  }
 }
 assert(recipes==96 && mutations==128 && pages==960);
 printf("PASS layer-link canonical DATA:96 next-layer recipes including dense-attention boundaries/128 descriptor mutations/960 actual-page missing-or-foreign refusals/foreign canonical pointer/one-shot binding;NO initialized or native producer/IO grant\n");
}
