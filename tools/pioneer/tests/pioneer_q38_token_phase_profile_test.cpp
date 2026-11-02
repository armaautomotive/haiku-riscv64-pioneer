/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38TokenPhaseProfileData.h"
#include <array>
#include <cassert>
#include <cstdio>
namespace Q=PioneerQ38TokenPhaseProfileData;
using Record=std::array<uint32_t,Q::Words>;
static Record Full(unsigned tokens,uint64_t epoch,uint64_t stride){
 Record p={};Q::Initialize(p.data(),true,epoch);uint64_t now=epoch;
 auto mark=[&](unsigned phase){now+=stride;Q::Advance(p.data(),true,now,phase);};
 for(unsigned token=0;token<tokens;token++){
  for(unsigned layer=0;layer<64;layer++){mark(Q::Metadata);mark(layer%4==3?Q::Dense:Q::Recurrent);}
  for(unsigned phase:{Q::HeadNormalize,Q::HeadMatrix,Q::Feedback,Q::StateCommit,Q::Publication})mark(phase);
 }
 mark(Q::FinalStop);now+=stride;Q::Advance(p.data(),true,now,Q::Phases,tokens);
 assert(Q::Closed(p.data()));return p;
}
int main(){
 assert(!Q::Closed(nullptr));unsigned cases=0;
 for(unsigned tokens:{1U,2U,34U,8192U})for(uint64_t epoch:{UINT64_C(0),UINT64_C(0xfffffffe),UINT64_C(0x10000000000)})
  for(uint64_t stride:{UINT64_C(1),UINT64_C(0x100000001)}){
   const auto p=Full(tokens,epoch,stride);assert(p[Q::Transitions]==133*tokens+2);
   assert(Q::Get(p.data(),Q::EndCycle)-epoch==stride*(133*tokens+2));
   auto unchanged=p;Q::Advance(unchanged.data(),true,0,Q::Metadata);assert(unchanged==p);cases++;
  }
 const auto good=Full(34,UINT64_C(0x10000000000),19);
 for(unsigned word=0;word<Q::Words;word++){
  auto corrupt=good;corrupt[word]^=1;assert(!Q::Closed(corrupt.data()));cases++;
 }
 for(unsigned failure=0;failure<10;failure++){
  Record p={};Q::Initialize(p.data(),true,100);unsigned reason=0;
  if(failure==0){Q::Advance(p.data(),false,101,Q::Metadata);reason=Q::ClockUnstable;}
  if(failure==1){Q::Advance(p.data(),true,99,Q::Metadata);reason=Q::ClockBackward;}
  if(failure==2){p[0]^=1;Q::Advance(p.data(),true,101,Q::Metadata);reason=Q::BadHeader;}
  if(failure==3){p[Q::Current]=Q::Phases;Q::Advance(p.data(),true,101,Q::Metadata);reason=Q::BadHeader;}
  if(failure==4){Q::Advance(p.data(),true,101,Q::Phases+1);reason=Q::BadPhase;}
  if(failure==5){Q::Advance(p.data(),true,101,Q::Phases,1);reason=Q::BadCommit;}
  if(failure==6){p[Q::Current]=Q::FinalStop;Q::Advance(p.data(),true,101,Q::Phases,8193);reason=Q::BadCommit;}
  if(failure==7){p[Q::Buckets]=UINT32_MAX;Q::Advance(p.data(),true,101,Q::Metadata);reason=Q::Overflow;}
  if(failure==8){p[Q::Transitions]=UINT32_MAX;Q::Advance(p.data(),true,101,Q::Metadata);reason=Q::Overflow;}
  if(failure==9){Q::Put(p.data(),Q::Buckets+1,UINT64_MAX);Q::Advance(p.data(),true,101,Q::Metadata);reason=Q::Overflow;}
  assert(p[Q::State]==Q::Invalid && p[Q::Error]==reason && !Q::Closed(p.data()));
  const auto before=p;Q::Advance(p.data(),true,102,Q::Metadata);assert(before==p);cases++;
 }
 Record p={};Q::Initialize(p.data(),false,0);assert(p[Q::State]==Q::Invalid && p[Q::Error]==Q::ClockUnstable);
 printf("PASS phase timing DATA:%ucases/all60 single-word corruptions/four token counts/64-bit rollover and accumulation/sticky invalid on uncertain,backward,overflow or malformed metric;NO model,native,physical frequency or speed proof\n",cases);
}
