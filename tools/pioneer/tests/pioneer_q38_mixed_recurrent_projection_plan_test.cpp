/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_Q38_FIXTURE_ONLY
#include "pioneer_qwen38_gguf_test.cpp"
#include "../PioneerQ38MixedRecurrentProjectionPlan.h"
#include "../PioneerQwen38FleetPlan.h"
#include <array>
#include <memory>
using C=PioneerQ38CanonicalModelPlan;
using P=PioneerQ38MixedRecurrentProjectionPlan;
namespace L=PioneerQ38MixedRecurrentProjectionRootLayout;
int main(int argc,char** argv){
 assert(argc<=2);Fixture fixture;FILE* file=argc==2?fopen(argv[1],"rb"):fixture.file;assert(file);
 PioneerQwen38Gguf model;PioneerQwen38PackedPlan packed;assert(model.Open(file) && packed.Build(model,8192,true,true));
 auto c=std::make_unique<C>(),foreign=std::make_unique<C>();assert(c->Build() && foreign->Build());
 assert(c->Count()==packed.Parts().size());
 for(unsigned i=0;i<c->Count();i++){
  const auto& a=*c->Part(i);const auto& b=packed.Parts()[i];
  assert(a.kind==b.kind && a.layer==b.layer && a.role==b.role && a.bank==b.bank && a.firstRow==b.firstRow && a.rows==b.rows
   && a.rowBytes==b.rowBytes && C::Base+a.offset==b.proposedAddress && a.bytes==b.bytes && a.allocated==b.allocated);
 }
 std::array<uint32_t,1920> sources;for(unsigned i=0;i<sources.size();i++)sources[i]=0x16005+i*131;
 std::vector<uint8_t> pages(PioneerResidentBanks::Pages,1);
 unsigned plans=0,independent=0,mutations=0,refusals=0;
 for(unsigned layer=0;layer<64;layer++)if(layer%4!=3)for(unsigned role:{10U,11U,5U,6U,13U,2U,3U,4U})
 for(unsigned bank=0;bank<7;bank++)for(unsigned pos:{0U,8191U}){
  auto p=std::make_unique<P>();
  assert(p->Build(*c,layer,role,pos,248319,bank,7000,123,{1,2,3,4},sources.data()) && p->Matches(*c)
   && !p->Matches(*foreign) && p->LearnedPagesPresent(pages.data(),unsigned(pages.size())));plans++;
  assert(!p->Build(*c,layer,role,pos,248319,bank,7000,123,{1,2,3,4},sources.data()));
  if(!bank && !pos){
   PioneerQ38Tensor expected;assert(PioneerQ38Expected(layer,role,expected));
   const auto* t=model.Find(expected.name.c_str());assert(t);PioneerQwen38FleetPlan fleet;
   assert(fleet.Build(model,packed,unsigned(t-model.Tensors().data()),119) && fleet.Jobs().size()==p->Frame()[7]);
   for(unsigned w=0;w<119;w++){
    const auto* q=p->Parts()[w];
    if(w>=p->Frame()[7]){for(unsigned i=0;i<8;i++)assert(!q[i]);assert(!p->Rows()[w]);continue;}
    const auto& h=fleet.Jobs()[w];
    assert(!h.wave && q[0]==C::Base+h.proposedOffset && q[1]==h.bank && q[2]==h.groups && q[3]==0
     && q[4]==PioneerQ38ProjectionRoute(h.bank) && q[5]==sources[(w+1)*16] && !q[6] && !q[7] && p->Rows()[w]==h.firstRow);
   }
   independent++;
  }
  if(layer || bank || pos)continue;
  auto mutate=[&](const uint32_t* data,unsigned n){
   for(unsigned i=0;i<n;i++){
    auto* word=const_cast<uint32_t*>(data)+i;*word^=1;assert(!p->Matches(*c));*word^=1;mutations++;
   }
   assert(p->Matches(*c));
  };
  mutate(p->Frame(),64);mutate(p->Header(),16);mutate(p->Banks(),8);mutate(&p->Parts()[0][0],119*8);mutate(p->Rows(),119);
  for(unsigned i=0;i<c->Count();i++){
   const auto* part=c->Part(i);if(part->kind!=c->WeightKind || part->layer!=layer || part->role!=role)continue;
   unsigned first=part->bank*unsigned(c->Capacity/4096)+part->offset/4096;
   for(unsigned end:{0U,part->allocated/4096-1})for(uint8_t invalid:{uint8_t(0),uint8_t(2),uint8_t(255)}){
    pages[first+end]=invalid;assert(!p->LearnedPagesPresent(pages.data(),unsigned(pages.size())));pages[first+end]=1;refusals++;
   }
  }
 }
 for(unsigned bad=0;bad<14;bad++){
  auto p=std::make_unique<P>();auto changed=sources;if(bad==13)changed[119*16]=0;
  assert(!p->Build(*c,bad==0?3:bad==1?64:0,bad==2?7:bad==3?0:10,bad==4?8192:0,bad==5?248320:0,
   bad==6?7:0,bad==7?0:bad==8?0x7fffffff:17,bad==9?0:bad==10?0x7fffffff:1,
   bad==11?PioneerQ38GdnIdentity{0,2,3,4}:PioneerQ38GdnIdentity{1,2,3,4},bad==12?nullptr:changed.data()));
  assert(!p->Frame() && !p->Header() && !p->Parts() && !p->Rows() && !p->Banks());refusals++;
 }
 if(argc==2)fclose(file);
 printf("PASS recurrent projection DATA plans%u/48layers8roles7banks2positions/independent%sGGUF-fleet%u/all-word-mutations%u/page+geometry-refusals%u;NO IO/source-auth/producer/native/fullmodel/TPS\n",
  plans,argc==2?" ORIGINAL ":" fixture ",independent,mutations,refusals);
}
