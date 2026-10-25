/* SPDX-License-Identifier: MIT */
// Pure observation DATA only. NO driver/model/source/IO/cleanup permission.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerReadbackPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputReadbackPlan.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <set>
int main(){
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 std::array<uint32_t,1920> sources;std::array<uint32_t,119> sequences={};
 for(unsigned w=0;w<120;w++)for(unsigned i=0;i<16;i++)sources[w*16+i]=0x12340001+w*32+i;
 unsigned verified=0;
 for(unsigned bank=0;bank<7;bank++){
  auto layer=std::make_unique<PioneerQ38ResidentLayerPlan>();
  assert(layer->Build(*canonical,0,0,bank,0,248319,19,{1,2,3,4},sources.data(),sequences.data()));
  PioneerQ38ResidentLayerReadbackPlan plan;PioneerQ38ResidentLayerReadbackPage page;
  assert(!plan.PageCount() && !plan.Page(0,page) && plan.Build(*layer) && !plan.Build(*layer)
   && plan.PageCount()==1592);
  unsigned byKind[7]={};std::set<uint64_t> pages;
  for(unsigned index=0;index<1592;index++){
   assert(plan.Page(index,page) && page.firstWord==index*1024 && page.words==1024 && page.kind<7
    && pages.insert(uint64_t(page.bank)<<32|page.address).second);byKind[page.kind]++;verified++;
   const auto* m=layer->Manifest();
   if(index<56){assert(page.bank==bank && page.route==PioneerQ38ProjectionRoute(bank));
    unsigned offsets[]={0,0x120000,0x130000,0x150000,0x100000},first[]={0,5,15,45,51};
    assert(page.address==m[8]+offsets[page.kind]+(index-first[page.kind])*4096);
   }else{
    unsigned slot=(index-56)/768,at=(index-56)%768,half=at/8,part=at%8;
    const auto* d=PioneerQ38CohortGdn(m,half+1)+64;
    assert(page.kind==(slot?plan.NextState:plan.CurrentState) && page.bank==d[4+slot*2]
     && page.address==d[5+slot*2]+part*4096 && page.route==d[12+slot]);
   }
  }
  const unsigned counts[]={5,10,30,6,5,768,768};for(unsigned i=0;i<7;i++)assert(byKind[i]==counts[i]);
  auto before=page;for(unsigned bad:{1592U,1593U,UINT32_MAX}){assert(!plan.Page(bad,page) && !memcmp(&before,&page,sizeof(page)));}
  auto* m=const_cast<uint32_t*>(layer->Manifest());m[16]^=1;assert(!plan.Page(0,page) && !memcmp(&before,&page,sizeof(page)));m[16]^=1;
  auto* l=const_cast<uint32_t*>(layer->LayerManifest());l[13]+=4096;
  assert(PioneerQ38ResidentLayerShape(l,layer->Manifest(),layer->PostManifest()) && !layer->CanonicalRecipe()
   && !plan.Page(0,page) && !memcmp(&before,&page,sizeof(page)));l[13]-=4096;
  assert(plan.Page(0,page));
 }
 for(unsigned kind=0;kind<3;kind++){
  auto layer=std::make_unique<PioneerQ38ResidentLayerPlan>();
  assert(layer->Build(*canonical,kind==0?1:0,kind==1?1:0,0,kind==2?1:0,0,19,{1,2,3,4},sources.data(),sequences.data()));
  PioneerQ38ResidentLayerReadbackPlan plan;assert(!plan.Build(*layer) && !plan.PageCount());
 }
 static_assert(PioneerQ38ResidentLayerReadbackPlan::Words!=PioneerQ38ResidentOutputReadbackPlan::Words,"old prefix payload must NEVER be retagged as complete-layer evidence");
 printf("PASS NEW complete-layer readback DATA:7banks/11144unique4096Bpages,final hidden+preservedConv/history/GDN/projection+both original state slots,exact1630208words,foreign layer/position/slot/changedsource/invalid-page refusals/no output mutation;old partial prefix is NOT an alias;NO native readback/ORIGINAL model/physical/cleanup or speed grant\n");
 assert(verified==11144);
}
