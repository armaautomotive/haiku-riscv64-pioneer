/* SPDX-License-Identifier: MIT */
// Observer DATA tests only. No inference products or cleanup authority.
#include "../PioneerQ38Bf16ProfileData.h"
#include "../PioneerQ38TokenPhaseProfileData.h"
#include <array>
#include <cassert>
#include <cstdio>
namespace Q=PioneerQ38Bf16ProfileData;
using Record=std::array<uint32_t,Q::Words>;
static Record Full(unsigned tokens,uint64_t epoch,uint64_t stride){
 Record p={};Q::Initialize(p.data(),true,epoch);uint64_t now=epoch;
 auto mark=[&](unsigned phase){now+=stride;Q::Advance(p.data(),true,now,phase);};
 for(unsigned token=0;token<tokens;token++){
  for(unsigned layer=0;layer<64;layer++){
   const bool dense=layer%4==3;const unsigned outer=dense?Q::Dense:Q::Recurrent;
   mark(Q::Metadata);mark(outer);
   for(unsigned job=0;job<(dense?7U:8U);job++){
    mark(Q::MatrixPrepare);mark(Q::MatrixDispatch);mark(Q::MatrixGatherWait);mark(Q::MatrixFinish);mark(outer);
   }
  }
  for(unsigned phase:{Q::HeadNormalize,Q::HeadMatrix,Q::Feedback,Q::StateCommit,Q::Publication})mark(phase);
 }
 mark(Q::FinalStop);now+=stride;Q::Advance(p.data(),true,now,Q::Phases,tokens);
 assert(Q::Closed(p.data()));
 assert(!PioneerQ38TokenPhaseProfileData::Closed(p.data())); // old ten-phase grammar is NOT this record
 assert(p[Q::Transitions]==2613*tokens+2);
 assert(Q::Get(p.data(),Q::EndCycle)-epoch==stride*(2613*tokens+2));
 const unsigned counts[]={1,64*tokens,432*tokens,128*tokens,tokens,tokens,tokens,tokens,tokens,1,
  496*tokens,496*tokens,496*tokens,496*tokens};
 for(unsigned phase=0;phase<Q::Phases;phase++){
  assert(p[Q::Buckets+phase*3]==counts[phase]);
  assert(Q::Get(p.data(),Q::Buckets+phase*3+1)==stride*counts[phase]);
 }
 auto unchanged=p;Q::Advance(unchanged.data(),true,0,Q::Metadata);assert(unchanged==p);
 return p;
}
int main(){
 assert(!Q::Closed(nullptr));unsigned cases=0;
 for(unsigned tokens:{1U,2U,34U})for(uint64_t epoch:{UINT64_C(0),UINT64_C(0xfffffffe),UINT64_C(0x10000000000)})
  for(uint64_t stride:{UINT64_C(1),UINT64_C(0x100000001)}){Full(tokens,epoch,stride);cases++;}
 Full(8192,0,1);cases++;
 const auto good=Full(34,UINT64_C(0x10000000000),19);
 for(unsigned word=0;word<Q::Words;word++){
  auto corrupt=good;corrupt[word]^=1;assert(!Q::Closed(corrupt.data()));cases++;
 }
 for(unsigned failure=0;failure<11;failure++){
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
  if(failure==10){p[Q::Current]=Q::FinalStop;Q::Advance(p.data(),true,101,Q::Phases,0);reason=Q::BadCommit;}
  assert(p[Q::State]==Q::Invalid && p[Q::Error]==reason && !Q::Closed(p.data()));
  const auto before=p;Q::Advance(p.data(),true,102,Q::Metadata);assert(before==p);cases++;
 }
 Record p={};Q::Initialize(p.data(),false,0);assert(p[Q::State]==Q::Invalid && p[Q::Error]==Q::ClockUnstable);
 Q::Initialize(p.data(),true,0);assert(!Q::Closed(p.data()));
 printf("PASS BF16 timing DATA:%u cases/14 exclusive buckets/496 matrix scopes per token/8192-token bounds/"
  "all60 word corruptions/64-bit sums/sticky invalid/no old grammar alias;NO hardware or speed proof\n",cases);
}
