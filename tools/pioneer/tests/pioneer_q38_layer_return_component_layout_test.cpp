/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../PioneerQ38MixedRecurrentBlockPlan.h"
#include "../PioneerQ38MixedLiveBlockPlan.h"
#include "../PioneerQ38LayerReturnComponentLayout.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <memory>
namespace A=PioneerQ38LayerReturnComponentLayout;
int main(){
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 std::array<uint32_t,1920> sources;for(unsigned i=0;i<sources.size();i++)sources[i]=0x16005+i*131;
 unsigned stackChecks=0;
 const uint32_t addresses[]={0,0x6dffc,0x6e000,0x6e001,0x6e004,0x6e400,0x6e404,0x6f000,0x70000-1028,0x70000-1024,0x70000,0xfffffff0};
 for(uint32_t parent:addresses)for(uint32_t local:addresses){
  const uint64_t pe=uint64_t(parent)+1028,le=uint64_t(local)+1028;
  bool expected=!((parent|local)&3) && parent>=0x6e000 && local>=0x6e000 && pe<=0x70000 && le<=0x70000
   && (pe<=local || le<=parent);
  assert(A::PrivateFrames(parent,local)==expected);stackChecks++;
 }
 unsigned frames=0,bootFaults=0,bindingFaults=0;
 for(bool dense:{false,true})for(unsigned layer:dense?std::array<unsigned,2>{3,63}:std::array<unsigned,2>{0,62})
  for(unsigned bank:{0U,6U})for(unsigned position:{0U,8191U}){
   auto recurrent=std::make_unique<PioneerQ38MixedRecurrentBlockPlan>();auto attention=std::make_unique<PioneerQ38MixedLiveBlockPlan>();
   const unsigned input=canonical->Base+canonical->Workspace(bank)->offset+A::R::FinalOffset;
   const uint32_t* outer;const uint32_t* originalBoot;
   if(dense){
    assert(attention->Build(*canonical,layer,position,248319,bank,17,5,{1,2,3,4},sources.data(),bank,input));
    outer=attention->Outer();originalBoot=attention->Boot(0);
   }else{
    assert(recurrent->Build(*canonical,layer,position,248319,bank,0,17,5,{1,2,3,4},sources.data(),bank,input));
    outer=recurrent->Outer();originalBoot=recurrent->Boot(0);
   }
   std::array<uint32_t,64> boot;for(unsigned i=0;i<64;i++)boot[i]=originalBoot[i];
   boot[0]=A::Magic;for(unsigned i=40;i<46;i++)boot[i]=A::BootWord(i);
   assert(A::BootShape(boot.data()) && A::Frame(boot.data(),outer,dense)
    && !A::Frame(boot.data(),outer,!dense) && !A::BootShape(originalBoot)
    && !A::R::BootShape(boot.data()) && !A::D::BootShape(boot.data()));frames++;
   assert(!A::Frame(nullptr,outer,dense) && !A::Frame(boot.data(),nullptr,dense));
   for(unsigned word=0;word<64;word++)for(unsigned bit=0;bit<32;bit++){
    boot[word]^=1U<<bit;assert(!A::Frame(boot.data(),outer,dense));boot[word]^=1U<<bit;bootFaults++;
   }
   std::array<uint32_t,128> changed;for(unsigned i=0;i<128;i++)changed[i]=outer[i];
   for(unsigned word=16;word<40;word++)for(unsigned bit=0;bit<32;bit++){
    changed[word]^=1U<<bit;assert(!A::Frame(boot.data(),changed.data(),dense));changed[word]^=1U<<bit;bindingFaults++;
   }
   assert(A::Frame(boot.data(),outer,dense));
  }
 printf("PASS QLC1 returning-component DATA frames%u/recurrent0+62,dense3+63/banks0+6/positions0+8191/new boot rejects old images/boot-bit refusals%u/source+identity bit refusals%u/private stack-bound+overlap cases%u/null+family isolation;NO execution/predecessor/native/state/token/physical/Work or TPS proof\n",frames,bootFaults,bindingFaults,stackChecks);
}
