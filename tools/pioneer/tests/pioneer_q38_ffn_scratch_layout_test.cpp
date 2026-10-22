/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_FIXTURE_ONLY
#include "pioneer_qwen38_gguf_test.cpp"
#include "../PioneerQ38FfnScratchLayout.h"
using Layout=PioneerQ38FfnScratchLayout;
static bool Same(const Layout& a,const Layout& b) {
 if(a.source!=b.source || a.epoch!=b.epoch || a.workspaceFragment!=b.workspaceFragment || a.bank!=b.bank || a.bytes!=b.bytes)return false;
 for(unsigned i=0;i<Layout::Slots;i++) {
  const auto& x=a.slice[i];const auto& y=b.slice[i];
  if(x.offset!=y.offset || x.bytes!=y.bytes || x.elements!=y.elements || x.encoding!=y.encoding
   || x.activationBytes!=y.activationBytes || x.scaleBytes!=y.scaleBytes)return false;
 }
 return true;
}
int main() {
 Fixture fixture;PioneerQwen38Gguf model;assert(model.Open(fixture.file));unsigned cases=0,negative=0;
 const unsigned expected[]={20480,10368,69632,69632,69632,35136,20480};
 for(unsigned context:{1U,8192U,32768U})for(bool two:{false,true}) {
  PioneerQwen38PackedPlan plan;assert(plan.Build(model,context,true,two));
  for(unsigned bank=0;bank<7;bank++) {
   Layout layout;assert(Layout::Describe(model,plan,bank,layout));cases++;
   assert(layout.source==&model && layout.epoch==model.Epoch() && layout.bank==bank && layout.bytes==295360);
   const auto& workspace=plan.Parts()[layout.workspaceFragment];assert(workspace.kind==PIONEER_Q38_WORKSPACE && workspace.bank==bank);
   uint32_t end=0;
   for(unsigned slot=0;slot<Layout::Slots;slot++) {
    const auto& s=layout.slice[slot];assert(s.offset==end && s.bytes==expected[slot] && !(s.offset&63) && !(s.bytes&63));end+=s.bytes;
    assert(end<=workspace.bytes && uint64_t(workspace.proposedAddress)+end<=uint64_t(PioneerQwen38PackedPlan::Base)+PioneerQwen38PackedPlan::Capacity);
    for(const auto& p:plan.Parts())if(p.bank==bank && &p!=&workspace) {
     uint64_t start=uint64_t(workspace.proposedAddress)+s.offset;
     assert(start+s.bytes<=p.proposedAddress || uint64_t(p.proposedAddress)+p.allocated<=start);
    }
    for(unsigned other=0;other<slot;other++)assert(layout.slice[other].offset+layout.slice[other].bytes<=s.offset);
    if(slot==Layout::QuantInput || slot==Layout::QuantActivated) {
     assert(s.encoding==Layout::NativeOperand16AndFloat32Scales && s.activationBytes==s.elements*2
      && s.scaleBytes==((s.elements/256*4+63)&~63U) && s.bytes==s.activationBytes+s.scaleBytes);
    } else assert(s.encoding==Layout::Float32 && !s.activationBytes && !s.scaleBytes && s.bytes==s.elements*4);
   }
   assert(end==295360 && workspace.bytes-end==66813504);
  }
  Layout out;assert(Layout::Describe(model,plan,0,out));auto before=out;
  for(unsigned bank:{7U,UINT32_MAX}){assert(!Layout::Describe(model,plan,bank,out) && Same(out,before));negative++;}
  PioneerQwen38Gguf other;assert(other.Open(fixture.file));assert(!Layout::Describe(other,plan,0,out) && Same(out,before));negative++;
  // Mutate only a TEST proposal through its const view. Actual plan owner
  // never offers mutable parts;these cases independently exercise refusals.
  auto& part=const_cast<PioneerQ38PackedPart&>(plan.Parts()[before.workspaceFragment]);const auto saved=part;
  for(unsigned fault=0;fault<10;fault++) {
   part=saved;
   if(fault==0)part.layer=0;if(fault==1)part.role=1;if(fault==2)part.tensor=0;if(fault==3)part.firstRow=16;
   if(fault==4)part.rows=2;if(fault==5)part.rowBytes--;if(fault==6)part.bytes--;if(fault==7)part.allocated--;
   if(fault==8)part.proposedAddress++;if(fault==9)part.proposedAddress=UINT32_MAX-4095;
   assert(!Layout::Describe(model,plan,0,out) && Same(out,before));negative++;
  }
  part=saved;
 }
 PioneerQwen38PackedPlan plan;assert(plan.Build(model,8192,false));Layout out={},before=out;
 assert(!Layout::Describe(model,plan,0,out) && Same(out,before));negative++;
 assert(plan.Build(model,8192,true) && Layout::Describe(model,plan,0,out));before=out;
 assert(model.Open(fixture.file));assert(!Layout::Describe(model,plan,0,out) && Same(out,before));negative++;
 assert(plan.Build(model,8192,true));assert(!ftruncate(fileno(fixture.file),off_t(PioneerQwen38Gguf::FullBytes)-1));
 assert(!Layout::Describe(model,plan,0,out) && Same(out,before));negative++;
 printf("PASS OFFLINE FFN scratch DATA cases%u refusals%u;7nonaliasing64Balignedslices/295360B inside EXISTING64MiBworkspace/all7banks/3contexts/1or2deltaslots/learned-state disjoint/atomic invalid-stale-source refusal;NO_ALLOCATION_ADDRESS_DEVICE_OR_CLEANUP_AUTHORITY\n",cases,negative);
}
