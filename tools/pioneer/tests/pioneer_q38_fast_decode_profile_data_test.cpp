/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38FastDecodeProfileData.h"
#include <array>
#include <cassert>
#include <cstdio>
namespace Q=PioneerQ38FastDecodeProfileData;
using Record=std::array<uint32_t,Q::Words>;
static Record Receipt(unsigned prompts,unsigned end){
 Record p{};Q::Initialize(p.data());uint64_t clock=100;
 auto mark=[&](unsigned phase,unsigned committed=0){clock+=10;Q::Advance(p.data(),true,clock,phase,committed);};
 for(unsigned position=0;position<end;position++){
  Q::Position(p.data(),position,prompts,true,clock+=10);
  for(unsigned layer=0;layer<64;layer++){
   mark(Q::Metadata);const unsigned outer=layer%4==3?Q::Dense:Q::Recurrent;mark(outer);
   for(unsigned matrix=0;matrix<(layer%4==3?7U:8U);matrix++){
    mark(Q::MatrixPrepare);mark(Q::MatrixDispatch);mark(Q::MatrixGatherWait);mark(Q::MatrixFinish);mark(outer);
   }
  }
  for(unsigned phase:{Q::HeadNormalize,Q::HeadMatrix,Q::Feedback,Q::StateCommit,Q::Publication})mark(phase);
 }
 mark(Q::FinalStop);mark(Q::Phases,end);return p;
}
int main(){
 unsigned positive=0,refused=0;
 for(auto window:{std::array<unsigned,2>{31,62},{1,2},{1,8192},{8190,8192}}){
  auto p=Receipt(window[0],window[1]);assert(Q::Closed(p.data()));positive++;
  assert(p[Q::WindowStart]==window[0] && p[Q::Committed]==window[1] && p[Q::LastPosition]==window[1]-1);
  assert(p[Q::Buckets+Q::MatrixPrepare*3]==496*(window[1]-window[0]));
 }
 const auto good=Receipt(31,62);
 for(unsigned word=0;word<Q::Words;word++){
  auto p=good;p[word]^=1;assert(!Q::Closed(p.data()));refused++;
 }
 for(unsigned prompts:{0U,8192U}){
  Record p{};Q::Initialize(p.data());Q::Position(p.data(),0,prompts,true,1);assert(p[Q::State]==Q::Invalid);refused++;
 }
 for(unsigned bad:{0U,2U,31U}){
  Record p{};Q::Initialize(p.data());Q::Position(p.data(),0,31,true,1);Q::Position(p.data(),bad,31,true,2);
  assert(p[Q::State]==Q::Invalid);refused++;
 }
 {
  Record p{};Q::Initialize(p.data());
  for(unsigned at=0;at<31;at++)Q::Position(p.data(),at,31,false,0);
  assert(p[Q::State]==Q::Waiting && !Q::Get(p.data(),Q::BeginCycle));
  Q::Advance(p.data(),false,0,Q::Metadata);assert(p[Q::State]==Q::Waiting);
  Q::Position(p.data(),31,31,false,0);assert(p[Q::State]==Q::Invalid);refused++;
 }
 for(unsigned failure=0;failure<7;failure++){
  Record p{};Q::Initialize(p.data());Q::Position(p.data(),0,1,true,1);Q::Position(p.data(),1,1,true,100);
  assert(Q::Running(p.data()));
  if(failure==0)Q::Position(p.data(),2,2,true,110);
  if(failure==1)Q::Position(p.data(),1,1,true,110);
  if(failure==2)Q::Advance(p.data(),true,99,Q::Metadata);
  if(failure==3)Q::Advance(p.data(),false,110,Q::Metadata);
  if(failure==4){p[Q::Buckets]=~uint32_t(0);Q::Advance(p.data(),true,110,Q::Metadata);}
  if(failure==5){Q::Put(p.data(),Q::Buckets+1,~uint64_t(0));Q::Advance(p.data(),true,110,Q::Metadata);}
  if(failure==6)Q::Advance(p.data(),true,110,Q::Phases,2);
  assert(p[Q::State]==Q::Invalid && !Q::Closed(p.data()));refused++;
  const auto invalid=p;Q::Advance(p.data(),true,1000,Q::Metadata);assert(p==invalid);
 }
 std::array<uint32_t,Q::Words+2> guarded{};guarded.front()=0x12345678;guarded.back()=0x87654321;
 Q::Initialize(guarded.data()+1);assert(guarded.front()==0x12345678 && guarded.back()==0x87654321);
 assert(!Q::Closed(nullptr));refused++;
 printf("PASS_FAST_DECODE_PROFILE_DATA positive%u refused%u exact31of62_window_counts_no_prefill_clock_no_model_or_device_authority\n",positive,refused);
}
