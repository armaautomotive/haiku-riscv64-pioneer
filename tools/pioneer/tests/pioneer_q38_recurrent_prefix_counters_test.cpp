/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../PioneerQ38RecurrentPrefixCounters.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <memory>
int main(){
 auto c=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(c->Build());
 std::array<uint32_t,1920> sources;for(unsigned i=0;i<sources.size();i++)sources[i]=0x16005+i*131;
 auto p=std::make_unique<PioneerQ38RecurrentPrefixPlan>();assert(p->Build(*c,0,{1,2,3,4},sources.data()));
 unsigned cases=0,refusals=0;
 for(unsigned w=1;w<120;w++)for(bool before:{false,true})for(auto each:{std::array<unsigned,3>{1,1,1},{256,256,256},{1,19,256}}){
  if(w>96)each.fill(0);
  const unsigned total=each[0]+each[1]+each[2];uint32_t expected[8]={},actual[8];
  for(unsigned layer=0;layer<3;layer++){uint32_t delta[8];
   assert(PioneerQ38RecurrentBlockCounterDelta(*p->Layer(layer),*c,w,each[layer],before && layer==2,delta));
   for(unsigned i=0;i<8;i++)expected[i]+=delta[i];
  }
  assert(PioneerQ38RecurrentPrefixCounterDelta(*p,*c,w,total,before,actual));
  for(unsigned i=0;i<8;i++)assert(actual[i]==expected[i]);
  uint32_t base[8],end[8];for(unsigned i=0;i<8;i++)base[i]=0xfffffff0U+i*13;base[6]=base[2];
  for(unsigned i=0;i<8;i++)end[i]=base[i]+expected[i];
  unsigned inferred=0xa5a5a5a5;
  assert(PioneerQ38RecurrentPrefixCounterProof(*p,*c,w,base,end,before,inferred) && inferred==total);cases++;
  for(unsigned i=0;i<8;i++){
   end[i]++;inferred=0xa5a5a5a5;assert(!PioneerQ38RecurrentPrefixCounterProof(*p,*c,w,base,end,before,inferred) && inferred==0xa5a5a5a5);end[i]--;refusals++;
  }
  if(w<=96 && (total==3 || total==768)){
   for(unsigned i=0;i<8;i++)if(i==0 || i==1 || i==2 || i==3 || i==6)end[i]+=total==3?uint32_t(-1):1U;
   inferred=0xa5a5a5a5;assert(!PioneerQ38RecurrentPrefixCounterProof(*p,*c,w,base,end,before,inferred) && inferred==0xa5a5a5a5);refusals++;
  }
 }
 for(unsigned w:{0U,120U,UINT32_MAX}){uint32_t output[8];for(auto& value:output)value=0xa5a5a5a5;
  assert(!PioneerQ38RecurrentPrefixCounterDelta(*p,*c,w,3,false,output));for(auto value:output)assert(value==0xa5a5a5a5);refusals++;
 }
 for(unsigned polls:{0U,1U,2U,769U,UINT32_MAX}){uint32_t output[8];for(auto& value:output)value=0xa5a5a5a5;
  assert(!PioneerQ38RecurrentPrefixCounterDelta(*p,*c,1,polls,false,output));for(auto value:output)assert(value==0xa5a5a5a5);refusals++;
 }
 uint32_t output[8];for(auto& value:output)value=0xa5a5a5a5;
 auto* recipe=const_cast<uint32_t*>(p->Layer(2)->Recipe(7));recipe[0]^=1;
 assert(!PioneerQ38RecurrentPrefixCounterDelta(*p,*c,1,3,false,output));for(auto value:output)assert(value==0xa5a5a5a5);recipe[0]^=1;refusals++;
 assert(p->Matches(*c));
 printf("PASS cumulative prefix counter DATA/cases%u/ALL119/full3+beforeLASTdown/exact sum of per-layer geometry/polls3..768+noGDN0/modulo32wrap/refusals%u+all8mutations/atomic failure;NOT actual instruction/native/STOP proof\n",cases,refusals);
}
