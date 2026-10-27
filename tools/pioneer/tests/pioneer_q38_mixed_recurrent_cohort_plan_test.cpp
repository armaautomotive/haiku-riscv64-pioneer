/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../PioneerQ38MixedRecurrentCohortPlan.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>
int main(){
 using C=PioneerQ38CanonicalModelPlan;using P=PioneerQ38MixedRecurrentCohortPlan;
 auto c=std::make_unique<C>(),foreign=std::make_unique<C>();assert(c->Build() && foreign->Build());
 std::array<uint32_t,1920> sources;for(unsigned i=0;i<sources.size();i++)sources[i]=0x16005+i*131;
 std::vector<uint8_t> pages(PioneerResidentBanks::Pages,1);unsigned plans=0,mutations=0;
 for(unsigned layer=0;layer<64;layer++)if(layer%4!=3)for(unsigned bank:{0U,6U})for(unsigned pos:{0U,8191U})for(unsigned slot:{0U,1U}){
  auto p=std::make_unique<P>();assert(p->Build(*c,layer,pos,bank,slot,248319,1001,7,{1,2,3,4},sources.data()));
  assert(p->Matches(*c) && !p->Matches(*foreign) && p->LearnedPagesPresent(pages.data(),unsigned(pages.size())));plans++;
  assert(!p->Build(*c,layer,pos,bank,slot,248319,1001,7,{1,2,3,4},sources.data()));
  if(layer || bank || pos || slot)continue;
  auto mutate=[&](const uint32_t* words,unsigned n){
   for(unsigned i=0;i<n;i++){
    auto* word=const_cast<uint32_t*>(words)+i;*word^=1;assert(!p->Matches(*c));*word^=1;mutations++;
   }
   assert(p->Matches(*c));
  };
  mutate(p->Header(),64);
  for(unsigned w:{0U,1U,39U,40U,96U,119U})mutate(p->Conv()+w*384,384);
  for(unsigned w:{0U,1U,38U,95U})mutate(p->Gdn()+w*192,192);
 }
 for(unsigned bad=0;bad<10;bad++){
  auto p=std::make_unique<P>();
  assert(!p->Build(*c,bad==0?3:0,bad==1?8192:0,bad==2?7:0,bad==3?2:0,bad==4?248320:0,
   bad==5?0:1001,bad==6?0:bad==7?0x7fffffff:7,bad==8?PioneerQ38GdnIdentity{0,2,3,4}:PioneerQ38GdnIdentity{1,2,3,4},
   bad==9?nullptr:sources.data()));assert(!p->Header() && !p->Conv() && !p->Gdn());
 }
 printf("PASS QRC1 immutable HEAP DATA plans%u/mutations%u/refusals10/bytes%zu/disjoint shared peer state;NO IO/native/source/producer/fullmodel grant\n",plans,mutations,sizeof(P));
}
