/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_Q38_FIXTURE_ONLY
#include "pioneer_qwen38_gguf_test.cpp"
#include "../PioneerQ38MixedLiveBlockPlan.h"
#include "../PioneerQwen38FleetPlan.h"
#include <array>
#include <memory>
using C=PioneerQ38CanonicalModelPlan;
using P=PioneerQ38MixedLiveBlockPlan;
namespace L=PioneerQ38MixedLiveBlockRootLayout;
static void Compare(const C& c,const PioneerQwen38PackedPlan& h){
 assert(c.Count()==h.Parts().size());
 for(unsigned i=0;i<c.Count();i++){
  const auto& a=*c.Part(i);const auto& b=h.Parts()[i];
  assert(a.kind==b.kind && a.layer==b.layer && a.role==b.role && a.bank==b.bank && a.firstRow==b.firstRow && a.rows==b.rows
   && a.rowBytes==b.rowBytes && c.Base+a.offset==b.proposedAddress && a.bytes==b.bytes && a.allocated==b.allocated);
 }
}
static void Matrix(const P& p,unsigned slot,const PioneerQwen38Gguf& model,const PioneerQwen38PackedPlan& packed){
 const auto* r=p.Recipe(slot);PioneerQ38Tensor expected;assert(PioneerQ38Expected(r[42],r[43],expected));
 const auto* t=model.Find(expected.name.c_str());assert(t);PioneerQwen38FleetPlan fleet;
 assert(fleet.Build(model,packed,unsigned(t-model.Tensors().data()),119) && fleet.Jobs().size()==r[7]);
 for(unsigned w=0;w<119;w++){
  const auto* q=r+80+w*8;
  if(w>=r[7]){for(unsigned i=0;i<8;i++)assert(!q[i]);assert(!r[1032+w]);continue;}
  const auto& h=fleet.Jobs()[w];
  assert(!h.wave && q[0]==C::Base+h.proposedOffset && q[1]==h.bank && q[2]==h.groups && q[3]==0
   && q[4]==PioneerQ38ProjectionRoute(h.bank) && q[5]==p.Boot(w+1)[16] && !q[6] && !q[7] && r[1032+w]==h.firstRow);
 }
}
static void Cache(const P& p,const PioneerQwen38PackedPlan& packed){
 for(unsigned w=0;w<96;w++){
  const auto* d=p.Dense(w);unsigned group=w/24;
  for(unsigned kind=0;kind<2;kind++){
   unsigned row=group+kind*4,found=0;
   for(const auto& part:packed.Parts())if(part.kind==PIONEER_Q38_KV_STATE && part.layer==d[3]
    && row>=part.firstRow && row-part.firstRow<part.rows){
    assert(d[8+kind]==part.proposedAddress+(row-part.firstRow)*part.rowBytes && d[10+kind]==part.bank);found++;
   }
   assert(found==1);
  }
  assert(d[40]==p.Outer()[6]+3 && d[4]==p.Outer()[4] && d[15]==w+1);
 }
}
int main(int argc,char** argv){
 assert(argc<=2);Fixture fixture;FILE* file=argc==2?fopen(argv[1],"rb"):fixture.file;assert(file);
 PioneerQwen38Gguf model;PioneerQwen38PackedPlan packed;assert(model.Open(file) && packed.Build(model,8192,true,true));
 auto c=std::make_unique<C>(),foreign=std::make_unique<C>();assert(c->Build() && foreign->Build());Compare(*c,packed);
 std::array<uint32_t,1920> sources;for(unsigned i=0;i<sources.size();i++)sources[i]=0x16005+i*131;
 std::vector<uint8_t> pages(PioneerResidentBanks::Pages,0);
 for(unsigned i=0;i<c->Count();i++){
  const auto* part=c->Part(i);if(part->kind!=c->WeightKind)continue;
  unsigned first=part->bank*unsigned(c->Capacity/4096)+part->offset/4096;
  for(unsigned j=0;j<part->allocated/4096;j++)pages[first+j]=1;
 }
 unsigned plans=0,matrices=0,mutations=0,refusals=0;
 for(unsigned layer=3;layer<64;layer+=4)for(unsigned bank=0;bank<7;bank++)for(unsigned pos:{0U,8191U}){
  auto p=std::make_unique<P>();unsigned input=c->Base+c->Workspace(bank)->offset+L::FinalOffset;
  assert(p->Build(*c,layer,pos,248319,bank,7000,1,{1,2,3,4},sources.data(),bank,input)
   && p->Matches(*c) && !p->Matches(*foreign) && p->LearnedPagesPresent(pages.data(),unsigned(pages.size())));plans++;
  assert(!p->Build(*c,layer,pos,248319,bank,7000,1,{1,2,3,4},sources.data(),bank,input));
  assert(!p->Boot(120) && !p->Recipe(7) && !p->Dense(96));
  for(unsigned slot=0;slot<7;slot++){Matrix(*p,slot,model,packed);matrices++;}Cache(*p,packed);
  if(layer!=3 || bank || pos)continue;
  auto mutate=[&](const uint32_t* words,unsigned n){
   for(unsigned i=0;i<n;i++){
    auto* changed=const_cast<uint32_t*>(words)+i;*changed^=1;assert(!p->Matches(*c));*changed^=1;mutations++;
   }
   assert(p->Matches(*c));
  };
  mutate(p->Outer(),128);for(unsigned w:{0U,1U,64U,119U})mutate(p->Boot(w),64);
  for(unsigned slot=0;slot<7;slot++){
   mutate(p->Recipe(slot),80);
   for(unsigned at:{80U,83U,1031U,1032U,1150U,1151U,1158U,1159U,1279U})mutate(p->Recipe(slot)+at,1);
  }
  for(unsigned w:{0U,23U,24U,71U,95U})mutate(p->Dense(w),64);
  // Each learned tensor/stripe rejects missing,foreign and non-one pages,
  // including both RMS weights and Q/K norm weights,not just matrices.
  for(unsigned i=0;i<c->Count();i++){
   const auto* part=c->Part(i);if(part->kind!=c->WeightKind || part->layer!=layer)continue;
   unsigned first=part->bank*unsigned(c->Capacity/4096)+part->offset/4096;
   for(unsigned offset:{0U,part->allocated/4096-1})for(uint8_t invalid:{uint8_t(0),uint8_t(2),uint8_t(255)}){
    pages[first+offset]=invalid;assert(!p->LearnedPagesPresent(pages.data(),unsigned(pages.size())));pages[first+offset]=1;refusals++;
   }
  }
 }
 for(unsigned bad=0;bad<12;bad++){
  auto p=std::make_unique<P>();const uint32_t input=c->Base+c->Workspace(0)->offset+L::FinalOffset;
  assert(!p->Build(*c,bad==0?0:bad==1?64:3,bad==2?8192:0,bad==3?248320:0,bad==4?7:0,bad==5?0:bad==6?0x7ffffff7:17,
   bad==7?0:bad==8?0x7fffffff:1,{1,2,3,4},sources.data(),bad==9?7:0,bad==10?input+1:bad==11?0xfffff000:input));
  assert(!p->Outer() && !p->Boot(0) && !p->Recipe(0) && !p->Dense(0));refusals++;
 }
 if(argc==2)fclose(file);
 printf("PASS QBL1 canonical DATA plans%u/matrices%u/all16 dense layers7banks/context endpoints/independent%sGGUF tiled loader+119job coordinates/K0..3,V4..7 cache rows/mutations%u/learned-page+geometry refusals%u;NO IO/native/source-auth/inputproducer/statecommit/inference-speed grant\n",
  plans,matrices,argc==2?" ORIGINAL local ":" fixture ",mutations,refusals);
}
