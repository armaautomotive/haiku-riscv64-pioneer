/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentAdvancePlan.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>
int main(){
 auto c=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(c->Build());
 auto other=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(other->Build());
 std::array<uint32_t,1920> source;source.fill(0x16005);std::array<uint32_t,119> sequence={};
 unsigned accepted=0,rejected=0,mutations=0,pages=0;
 for(unsigned layer=0;layer<63;layer++){
  if(layer%4==3)continue;
  auto p=std::make_unique<PioneerQ38ResidentLayerPlan>();
  assert(p->Build(*c,layer,layer&1?8191:0,layer%7,layer&1,layer&1?248319:0,17,{1,2,3,4},source.data(),sequence.data()));
  PioneerQ38ResidentLayerLinkPlan link;assert(link.Build(*c,*p));auto advance=std::make_unique<PioneerQ38ResidentAdvancePlan>();
  bool ok=advance->Build(*c,*p,link);assert(ok==((layer+1)%4!=3));
  if(!ok){assert(!advance->Descriptor() && !advance->Build(*c,*p,link));rejected++;continue;}
  assert(advance->Matches(*c,*p,link) && !advance->Matches(*other,*p,link) && !advance->Build(*c,*p,link));
  auto bad=std::make_unique<PioneerQ38ResidentAdvancePlan>();assert(!bad->Build(*other,*p,link));
  const auto* d=advance->Descriptor();unsigned jobs=0;
  for(unsigned role=0;role<4;role++){const auto* r=PioneerQ38ResidentAdvanceRecord(d,role);jobs+=r[2];assert(r[1]==8 && r[3]==5120);}
  assert(jobs==244 && d[13]==20);accepted++;
  if(layer)continue;
  auto* edit=const_cast<uint32_t*>(d);std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,1);
  assert(advance->LearnedPagesPresent(ledger.data(),unsigned(ledger.size())));
  for(unsigned word=0;word<TT_Q38_ADVANCE_WORDS;word++){
   edit[word]^=1;assert(!advance->Matches(*c,*p,link) && !advance->LearnedPagesPresent(ledger.data(),unsigned(ledger.size())));edit[word]^=1;mutations++;
  }
  for(unsigned w=0;w<3;w++)edit[64+64+w*8]+=64;
  assert(PioneerQ38ResidentAdvanceShape(d,link.Descriptor(),p->Manifest()) && !advance->Matches(*c,*p,link));
  for(unsigned w=0;w<3;w++)edit[64+64+w*8]-=64;
  for(unsigned role:{10U,11U,5U,6U})for(unsigned stripe=0;;stripe++){
   const auto* span=c->ProjectionStripe(1,role,stripe);if(!span)break;
   unsigned first=span->bank*unsigned(c->Capacity/4096)+span->offset/4096,last=first+span->allocated/4096-1;
   for(unsigned at:{first,last})for(unsigned missing:{0U,2U}){
    ledger[at]=uint8_t(missing);assert(!advance->LearnedPagesPresent(ledger.data(),unsigned(ledger.size())));ledger[at]=1;pages++;
   }
  }
 }
 assert(accepted==32 && rejected==16 && mutations==4672 && pages==64);
 printf("PASS advance canonical DATA32 GDN next-layer recipes/16 dense-boundary refusals/4672 descriptor mutations/64 missing-or-foreign edge-page refusals/foreign canonical and one-shot binding;244 matrix jobs share20 input producers;NO native/initialized/model readiness grant\n");
}
