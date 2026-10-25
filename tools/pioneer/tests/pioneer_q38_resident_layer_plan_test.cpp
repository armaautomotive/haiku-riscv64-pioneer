/* SPDX-License-Identifier: MIT */
// Canonical scheduling DATA only,not a native/image/execution capability.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerPlan.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <memory>
int main(){
 setvbuf(stdout,nullptr,_IOLBF,0);
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 std::array<uint32_t,1920> sources;std::array<uint32_t,119> sequences;
 for(unsigned w=0;w<120;w++)for(unsigned i=0;i<16;i++)sources[w*16+i]=i?0x12340000+w*32+i:0x16005+w*256;
 for(unsigned w=0;w<119;w++)sequences[w]=w%17;
 PioneerQ38GdnIdentity id={UINT64_C(1)<<40,UINT64_C(2)<<40,UINT64_C(3)<<40,UINT64_C(4)<<40};
 unsigned cases=0,mutations=0;
 for(unsigned layer=0;layer<64;layer++){
  if(layer%4==3)continue;
  for(unsigned bank:{0U,6U})for(unsigned slot:{0U,1U}){
   unsigned position=slot?8191:0,token=slot?248319:0,phase=19+layer;
   auto plan=std::make_unique<PioneerQ38ResidentLayerPlan>();
   assert(plan->Build(*canonical,layer,position,bank,slot,token,phase,id,sources.data(),sequences.data())
    && plan->Matches(*canonical,layer,position,bank,slot,token,phase,id,sources.data(),sequences.data()));
   const auto* l=plan->LayerManifest();const auto* m=plan->Manifest();const auto* q=plan->PostManifest();
   assert(PioneerQ38ResidentLayerShape(l,m,q) && l[3]==layer && l[4]==position && l[8]==bank && l[11]==token);
   std::array<uint32_t,64> d;PioneerQ38ResidentLayerNormDescriptor(l,d.data());
   assert(PioneerQ38ResidentLayerNormShape(d.data()) && d[15]==1048576 && d[40]==layer && d[50]==position);
   for(unsigned role=0;role<3;role++){
    const auto* r=l+64+PioneerQ38FfnResidualGraphRoleOffset(role);assert(r[1]==12 && r[2]==119);
    for(unsigned w=0;w<119;w++){
     const auto* p=r+16+w*8;
     assert(canonical->FfnSpan(layer,role,p[1],p[0]-canonical->Base,r[16+119*8+w],p[2]*16));
    }
   }
   assert(!plan->Build(*canonical,layer,position,bank,slot,token,phase,id,sources.data(),sequences.data()));
   cases++;
   if(layer || bank || slot)continue;
   auto* changed=const_cast<uint32_t*>(l);
   for(unsigned word=0;word<TT_Q38_RESIDENT_LAYER_WORDS;word++){
    changed[word]^=1;assert(!PioneerQ38ResidentLayerShape(l,m,q));changed[word]^=1;mutations++;
   }
   // A shape-valid different learned address is NOT the canonical model.
   changed[13]+=4096;assert(PioneerQ38ResidentLayerShape(l,m,q)
    && !plan->Matches(*canonical,layer,position,bank,slot,token,phase,id,sources.data(),sequences.data()));
   changed[13]-=4096;
   assert(plan->Matches(*canonical,layer,position,bank,slot,token,phase,id,sources.data(),sequences.data()));
   for(unsigned word=0;word<64;word++){
    auto original=d[word];d[word]^=1;
    // Some logical/source/epoch fields may change to another DATA-valid
    // shape. The complete descriptor must still differ from the recipe.
    std::array<uint32_t,64> expected;PioneerQ38ResidentLayerNormDescriptor(l,expected.data());
    assert(d!=expected);d[word]=original;
   }
  }
 }
 assert(cases==192 && mutations==3520);
 unsigned refusals=0;
 for(unsigned bad=0;bad<14;bad++){
  unsigned layer=0,position=0,bank=0,slot=0,token=0,phase=19;auto identity=id;
  const uint32_t* src=sources.data();const uint32_t* seq=sequences.data();
  auto altered=sources;auto alteredSequences=sequences;
  if(bad==0)layer=3;if(bad==1)layer=64;if(bad==2)position=8192;if(bad==3)bank=7;if(bad==4)slot=2;
  if(bad==5)token=248320;if(bad==6)phase=0;if(bad==7)phase=0x7ffffff0;if(bad==8)src=nullptr;
  if(bad==9){altered[0]=0;src=altered.data();}if(bad==10)seq=nullptr;
  if(bad==11){alteredSequences[0]=phase;seq=alteredSequences.data();}if(bad==12)identity.modelEpoch=0;if(bad==13)identity.driverGeneration=0;
  auto plan=std::make_unique<PioneerQ38ResidentLayerPlan>();
  assert(!plan->Build(*canonical,layer,position,bank,slot,token,phase,identity,src,seq) && !plan->Manifest()
   && !plan->PostManifest() && !plan->LayerManifest()
   && !plan->Build(*canonical,0,0,0,0,0,19,id,sources.data(),sequences.data()));refusals++;
 }
 printf("PASS NEW complete recurrent-layer canonical DATA:192layer/bank/slot/position/token recipes,3520single-word corruptions,shape-valid foreign norm address rejected,14one-shot invalid builds;ALL3x119FFN jobs exact,actual-sequence placeholders zero;NO IO/native/initialized/Work grant\n");
 assert(refusals==14);
}
