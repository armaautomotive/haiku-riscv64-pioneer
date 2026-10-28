/* SPDX-License-Identifier: MIT */
// Transform DATA vs independently rebuilt canonical plans, not execution,
// initialized memory, a completed token, or native/physical qualification.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../PioneerQ38TokenMetadata.h"
#include "../PioneerQ38TokenStatePlan.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>
namespace M=PioneerQ38TokenMetadata;namespace S=PioneerQ38TokenStateLayout;
namespace C=PioneerQ38GlobalChainLayout;
template<unsigned N> static void Equal(const uint32_t* a,const uint32_t* b){
 for(unsigned i=0;i<N;i++)assert(a[i]==b[i]);
}
template<unsigned N,class Fn> static void Refuse(const uint32_t* original,unsigned word,unsigned value,Fn fn){
 std::array<uint32_t,N> data;for(unsigned i=0;i<N;i++)data[i]=original[i];
 data[word]=value;const auto before=data;assert(!fn(data.data()) && data==before);
}
int main(){
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 std::array<uint32_t,1920> sources;for(unsigned i=0;i<sources.size();i++)sources[i]=0x76540001+i;
 const PioneerQ38GdnIdentity id={1,2,3,4};
 const unsigned positions[]={0,1,2,23,24,31,32,4095,8190,8191};
 unsigned outers=0,recipes=0,conv=0,gdn=0,dense=0,spans=0,refusals=0;
 for(unsigned bank=0;bank<7;bank++)for(unsigned layer=0;layer<64;layer++){
  const unsigned workspace=canonical->Base+canonical->Workspace(bank)->offset;
  auto package=std::make_unique<PioneerQ38GlobalChainPackage>();
  assert(package->Build(*canonical,layer,bank,id,sources.data(),bank,workspace+0x600000));
  std::vector<uint32_t> raw(package->Bytes()/4);
  for(unsigned at=0;at<package->Bytes();at+=16384){
   unsigned bytes=package->Bytes()-at;if(bytes>16384)bytes=16384;
   assert(package->Read(at,reinterpret_cast<uint8_t*>(raw.data())+at,bytes));
  }
  if(!C::Dense(layer)){
   auto state=std::make_unique<PioneerQ38TokenStatePlan>();
   assert(state->Build(*canonical,layer,bank) && state->Matches(*canonical));
   assert(!state->Build(*canonical,layer,bank) && state->Matches(*canonical));
   for(unsigned word=0;word<64;word++){
    auto* data=const_cast<uint32_t*>(state->Data());data[word]^=1;
    assert(!state->Matches(*canonical));data[word]^=1;assert(state->Matches(*canonical));refusals++;
   }
   unsigned n=0;
   for(unsigned i=0;i<canonical->Count();i++){
    const auto* p=canonical->Part(i);if(p->kind!=canonical->ConvKind || p->layer!=layer)continue;
    const auto* d=state->Data()+8+n*8;
    const unsigned expected[]={p->bank,canonical->Base+p->offset,p->firstRow,p->rows,12,p->bytes,p->allocated,PioneerQ38ProjectionRoute(p->bank)};
    Equal<8>(d,expected);n++;spans++;
   }
   assert(n==7);
  }else{
   auto state=std::make_unique<PioneerQ38TokenStatePlan>();assert(!state->Build(*canonical,layer,bank) && !state->Data());refusals++;
  }
  for(unsigned position:positions){
   const unsigned token=(position*29+layer*193+bank*7+17)%248320;
   std::array<uint32_t,128> outer;for(unsigned i=0;i<128;i++)outer[i]=raw[i];
   assert(M::Outer(outer.data(),layer,position,token,bank,workspace,package->Boot()));outers++;
   std::unique_ptr<PioneerQ38MixedRecurrentBlockPlan> recurrent;
   std::unique_ptr<PioneerQ38MixedLiveBlockPlan> attention;
   const uint32_t* expectedOuter;
   if(!C::Dense(layer)){
    recurrent=std::make_unique<PioneerQ38MixedRecurrentBlockPlan>();
    assert(recurrent->Build(*canonical,layer,position,token,bank,position&1,M::Phase(position,layer),M::Request(position,layer),
     id,sources.data(),bank,workspace+(layer?unsigned(C::R::FinalOffset):unsigned(PioneerQ38GlobalFeedbackLayout::Offset))));
    expectedOuter=recurrent->Outer();
   }else{
    attention=std::make_unique<PioneerQ38MixedLiveBlockPlan>();
    assert(attention->Build(*canonical,layer,position,token,bank,M::Phase(position,layer),M::Request(position,layer),
     id,sources.data(),bank,workspace+C::R::FinalOffset));
    expectedOuter=attention->Outer();
   }
   Equal<128>(outer.data(),expectedOuter);
   const unsigned recipeCount=C::Dense(layer)?7:8;
   for(unsigned slot=0;slot<recipeCount;slot++){
    std::array<uint32_t,1280> data;for(unsigned i=0;i<1280;i++)data[i]=raw[128+slot*1280+i];
    assert(M::Recipe(data.data(),layer,slot,position,token));
    Equal<1280>(data.data(),C::Dense(layer)?attention->Recipe(slot):recurrent->Recipe(slot));recipes++;
   }
   if(!C::Dense(layer)){
    for(unsigned worker=0;worker<120;worker++)for(unsigned slot=0;slot<6;slot++){
     const unsigned at=C::P::ConvOffset/4+worker*384+slot*64;
     std::array<uint32_t,64> data;for(unsigned i=0;i<64;i++)data[i]=raw[at+i];
     assert(M::Conv(data.data(),layer,worker,slot,position));
     Equal<64>(data.data(),recurrent->Conv()+worker*384+slot*64);conv++;
    }
    for(unsigned worker=0;worker<96;worker++){
     const unsigned at=C::P::GdnOffset/4+worker*192;
     std::array<uint32_t,192> data;for(unsigned i=0;i<192;i++)data[i]=raw[at+i];
     assert(M::Gdn(data.data(),layer,worker,position));
     Equal<192>(data.data(),recurrent->Gdn()+worker*192);gdn++;
    }
   }else for(unsigned worker=0;worker<96;worker++){
    const unsigned at=C::DenseFrames/4+worker*64;
    std::array<uint32_t,64> data;for(unsigned i=0;i<64;i++)data[i]=raw[at+i];
    assert(M::Dense(data.data(),layer,worker,position));
    Equal<64>(data.data(),attention->Dense(worker));dense++;
   }
  }
  // Invalid transform inputs must never partially modify their metadata.
  auto outerFn=[&](uint32_t* d){return M::Outer(d,layer,1,17,bank,workspace,package->Boot());};
  Refuse<128>(raw.data(),3,64,outerFn);Refuse<128>(raw.data(),4,1,outerFn);refusals+=2;
  auto recipeFn=[&](uint32_t* d){return M::Recipe(d,layer,0,1,17);};
  Refuse<1280>(raw.data()+128,40,1,recipeFn);Refuse<1280>(raw.data()+128,48,1,recipeFn);refusals+=2;
  if(!C::Dense(layer)){
   const auto* cd=raw.data()+C::P::ConvOffset/4;
   Refuse<64>(cd,40,1,[&](uint32_t* d){return M::Conv(d,layer,0,0,1);});
   const auto* gd=raw.data()+C::P::GdnOffset/4;
   Refuse<192>(gd,40,1,[&](uint32_t* d){return M::Gdn(d,layer,0,1);});refusals+=2;
  }else{
   const auto* dd=raw.data()+C::DenseFrames/4;
   Refuse<64>(dd,4,1,[&](uint32_t* d){return M::Dense(d,layer,0,1);});refusals++;
  }
  auto bad=std::array<uint32_t,128>{};for(unsigned i=0;i<128;i++)bad[i]=raw[i];const auto before=bad;
  assert(!M::Outer(bad.data(),layer,8192,17,bank,workspace,package->Boot()) && bad==before);
  assert(!M::Outer(bad.data(),layer,1,248320,bank,workspace,package->Boot()) && bad==before);refusals+=2;
 }
 printf("PASS token metadata DATA: %u outer/%u recipes/%u Conv/%u GDN triples/%u dense frames EXACT vs independently rebuilt canonical plans;7 banks/all64 layers/10 positions incl8191/both state slots; %u canonical Conv spans/%u refusal+no-partial-output checks;NO execution/state-commit/source/native/physical/Work/TPS grant\n",outers,recipes,conv,gdn,dense,spans,refusals);
}
