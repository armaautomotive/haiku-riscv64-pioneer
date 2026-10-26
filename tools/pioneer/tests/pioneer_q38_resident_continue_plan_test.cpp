/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentContinuePlan.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>
#define PIONEER_ATTENTION_FREESTANDING
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputProof.h"
int main(){
 uint32_t random=1;
 for(unsigned i=0;i<1000000;i++){
  random=random*1664525U+1013904223U;
  assert(PioneerQ38RootMultipleOf3(random)==(random%3==0));
 }
 for(uint32_t x:{0U,1U,2U,3U,0x80000000U,0xfffffffeU,0xffffffffU})assert(PioneerQ38RootMultipleOf3(x)==(x%3==0));
 auto c=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(c->Build());
 auto other=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(other->Build());
 std::array<uint32_t,1920> source;for(unsigned i=0;i<source.size();i++)source[i]=0x16005+i*256;
 std::array<uint32_t,119> zero={},actual;for(unsigned w=0;w<119;w++)actual[w]=14+unsigned(w<20);
 unsigned accepted=0,refused=0,mutations=0;
 for(unsigned layer=0;layer<63;layer++){
  if(layer%4==3)continue;
  auto parent=std::make_unique<PioneerQ38ResidentLayerPlan>();
  assert(parent->Build(*c,layer,layer&1?8191:0,layer%7,layer&1,248319,17,{1,2,3,4},source.data(),zero.data()));
  PioneerQ38ResidentLayerLinkPlan link;assert(link.Build(*c,*parent));PioneerQ38ResidentAdvancePlan advance;
  bool next=advance.Build(*c,*parent,link);auto p=std::make_unique<PioneerQ38ResidentContinuePlan>();
  if(!next){assert(!p->Build(*c,*parent,link,advance) && !p->Descriptor());refused++;continue;}
  assert(p->Build(*c,*parent,link,advance) && p->Matches(*c,*parent,link,advance) && !p->Matches(*other,*parent,link,advance)
   && !p->Build(*c,*parent,link,advance));accepted++;
  const auto* d=p->Descriptor();const auto* n=p->Next();assert(n && n->CanonicalRecipe());
  assert(PioneerQ38ResidentContinueRecipe(d,n->Manifest(),n->PostManifest(),n->LayerManifest(),parent->Manifest(),nullptr));
  if(layer)continue;
  auto* edit=const_cast<uint32_t*>(d);
  for(unsigned i=0;i<128;i++){edit[i]^=1;assert(!p->Matches(*c,*parent,link,advance));edit[i]^=1;mutations++;}
  std::vector<uint32_t> m(n->Manifest(),n->Manifest()+TT_Q38_COHORT_WORDS);
  for(unsigned w=0;w<119;w++)m[64+w]=actual[w];
  assert(PioneerQ38ResidentContinueRecipe(d,m.data(),n->PostManifest(),n->LayerManifest(),parent->Manifest(),actual.data())
   && !PioneerQ38ResidentContinueRecipe(d,m.data(),n->PostManifest(),n->LayerManifest(),parent->Manifest(),nullptr));
  for(unsigned w=0;w<119;w++){m[64+w]^=1;assert(!PioneerQ38ResidentContinueRecipe(d,m.data(),n->PostManifest(),n->LayerManifest(),parent->Manifest(),actual.data()));m[64+w]^=1;mutations++;}
  std::vector<uint8_t> pages(PioneerResidentBanks::Pages,1);assert(p->LearnedPagesPresent(pages.data(),unsigned(pages.size())));
  const auto* l=n->LayerManifest();unsigned first=l[12]*unsigned(c->Capacity/4096)+(l[13]-c->Base)/4096;
  for(unsigned missing:{0U,2U}){pages[first]=uint8_t(missing);assert(!p->LearnedPagesPresent(pages.data(),unsigned(pages.size())));pages[first]=1;}
 }
 assert(accepted==32 && refused==16 && mutations==247);
 puts("PASS continuation canonical DATA32/16dense-boundary refusals/128header+119actual-sequence corruptions/foreign canonical+duplicate binding/next learned-page absence/1000007freestanding-mod3 equivalence cases;285184-byte fixed recipe,119live sequences ONLY admitted on card;NO native/initialized/producer/full-model grant");
}
