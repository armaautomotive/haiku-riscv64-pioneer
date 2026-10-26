/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DenseProjectionPlan.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>
int main(){
 auto c=std::make_unique<PioneerQ38CanonicalModelPlan>(),foreign=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(c->Build() && foreign->Build());
 std::array<uint32_t,1920> sources;for(unsigned i=0;i<sources.size();i++)sources[i]=0x16005+i*131;
 unsigned recipes=0,mutations=0,refusals=0;
 for(unsigned layer=3;layer<64;layer+=4)for(unsigned bank=0;bank<7;bank++){
  auto p=std::make_unique<PioneerQ38DenseProjectionPlan>();
  assert(p->Build(*c,layer,bank&1?8191:0,248319,bank,1000,{1,2,3,4},sources.data()) && p->Matches(*c) && !p->Matches(*foreign));
  assert(!p->Build(*c,layer,0,0,bank,1000,{1,2,3,4},sources.data()));
  for(unsigned w=0;w<120;w++)for(unsigned i=0;i<16;i++)assert(p->Source(w)[i]==sources[w*16+i]);
  for(unsigned slot=0;slot<4;slot++){
   const auto* d=p->Frame(slot);assert(d && d[42]==layer && d[43]==p->Role(slot) && d[7]==(slot==1 || slot==2?64U:119U));
   assert(PioneerQ38DenseProjectionRecipe(d,p->Header(slot),p->Banks(),p->Parts(slot),p->Rows(slot),false));recipes++;
   uint32_t runtime[119][8];memcpy(runtime,p->Parts(slot),sizeof(runtime));
   for(unsigned w=0;w<d[7];w++)runtime[w][3]=w+1;
   assert(PioneerQ38DenseProjectionMatrixShape(d,p->Header(slot),p->Banks(),runtime,p->Rows(slot))
    && !PioneerQ38DenseProjectionRecipe(d,p->Header(slot),p->Banks(),runtime,p->Rows(slot),false));
   for(unsigned w=0;w<119;w++)assert(p->Parts(slot)[w][3]==0); // runtime is a SEPARATE mailbox
  }
  if(layer!=3 || bank)continue;
  std::vector<uint8_t> pages(PioneerResidentBanks::Pages,1);assert(p->LearnedPagesPresent(pages.data(),unsigned(pages.size())));
  for(unsigned slot=0;slot<4;slot++){
   for(unsigned i=0;i<64;i++){auto* d=const_cast<uint32_t*>(p->Frame(slot));d[i]^=1;assert(!p->Matches(*c));d[i]^=1;mutations++;}
   for(unsigned i=0;i<16;i++){auto* h=const_cast<uint32_t*>(p->Header(slot));h[i]^=1;assert(!p->Matches(*c));h[i]^=1;mutations++;}
   for(unsigned w=0;w<119;w++)for(unsigned i=0;i<8;i++){
    auto* q=const_cast<uint32_t*>(p->Parts(slot)[w]);q[i]^=1;assert(!p->Matches(*c));q[i]^=1;mutations++;
   }
   for(unsigned w=0;w<119;w++){auto* rows=const_cast<uint32_t*>(p->Rows(slot));rows[w]^=16;assert(!p->Matches(*c));rows[w]^=16;mutations++;}
   const auto* stripe=c->ProjectionStripe(3,p->Role(slot),0);unsigned at=stripe->bank*unsigned(c->Capacity/4096)+stripe->offset/4096;
   for(unsigned missing:{0U,2U}){pages[at]=uint8_t(missing);assert(!p->LearnedPagesPresent(pages.data(),unsigned(pages.size())));pages[at]=1;refusals++;}
  }
  assert(!p->Frame(4) && !p->Header(4) && !p->Parts(4) && !p->Rows(4) && !p->Source(120));
 }
 for(unsigned layer:{0U,1U,2U,4U,62U,64U}){
  auto p=std::make_unique<PioneerQ38DenseProjectionPlan>();assert(!p->Build(*c,layer,0,0,0,17,{1,2,3,4},sources.data()));refusals++;
 }
 printf("PASS canonical denseQG/K/V/Q6-O DATA recipes%u/16layers7banks4roles/mutations%u/absent-or-foreign-page+layer-refusals%u/foreigncanonical+duplicatebuild/ZERO immutable sequences andSEPARATE runtime grammar;NO source/init/inputproducer/native/Work grant\n",recipes,mutations,refusals);
}
