/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../PioneerQ38MixedRecurrentBlockPlan.h"
#include "../PioneerQ38RecurrentPrefixLayout.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <memory>
int main(){
 namespace P=PioneerQ38RecurrentPrefixLayout;namespace R=PioneerQ38MixedRecurrentBlockRootLayout;
 unsigned geometry=0,frames=0,refusals=0;
 for(uint32_t workspace:{0x04000000U,0xc0000000U})for(unsigned layer=0;layer<P::Layers;layer++){
  uint32_t address=0;assert(P::Address(workspace,layer,address)
   && address==workspace+P::Offset+layer*P::Stride && !(address&4095)
   && uint64_t(address)+P::Bytes<=uint64_t(workspace)+67108864);
  if(layer)assert(uint64_t(workspace)+P::Offset+(layer-1)*P::Stride+P::Bytes<=address);
  geometry++;
 }
 for(uint32_t workspace:{0U,0x03fff000U,0x04000001U,0xc0000001U,0xfffff000U,UINT32_MAX}){
  uint32_t unchanged=0xdeadbeef;assert(!P::Address(workspace,0,unchanged) && unchanged==0xdeadbeef);geometry++;
 }
 for(unsigned layer:{3U,64U,UINT32_MAX}){
  uint32_t unchanged=0xdeadbeef;assert(!P::Address(0x04000000,layer,unchanged) && unchanged==0xdeadbeef);geometry++;
 }
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 std::array<uint32_t,1920> sources;for(unsigned i=0;i<sources.size();i++)sources[i]=0x16005+i*131;
 assert(!P::BootShape(nullptr) && !P::Frame(nullptr,0,0,0,nullptr));
 for(unsigned bank=0;bank<7;bank++)for(unsigned layer=0;layer<P::Layers;layer++){
  auto block=std::make_unique<PioneerQ38MixedRecurrentBlockPlan>();
  const uint32_t workspace=canonical->Base+canonical->Workspace(bank)->offset;
  assert(block->Build(*canonical,layer,0,0,bank,0,1001+layer*16,layer+1,{1,2,3,4},sources.data(),bank,
   workspace+(layer?unsigned(R::FinalOffset):0x600000U)) && block->Matches(*canonical));
  std::array<uint32_t,64> boot;for(unsigned i=0;i<64;i++)boot[i]=block->Boot(0)[i];
  assert(R::BootShape(boot.data()) && !P::BootShape(boot.data())); // no old image alias
  for(unsigned i=40;i<46;i++)boot[i]=P::BootWord(i);
  assert(P::BootShape(boot.data()) && !R::BootShape(boot.data()));
  std::array<uint32_t,128> outer;for(unsigned i=0;i<128;i++)outer[i]=block->Outer()[i];
  assert(P::Frame(outer.data(),layer,bank,workspace,boot.data()));frames++;
  for(unsigned i=0;i<128;i++){
   // Norm bank and the first producer bank remain canonical/native-owner
   // obligations, not rights fabricated by this immutable DATA grammar.
   if(i==64 || i==66 || i==68)continue;
   auto bad=outer;bad[i]^=1;
   assert(!P::Frame(bad.data(),layer,bank,workspace,boot.data()));refusals++;
  }
  for(unsigned byte=0;byte<256;byte++){
   auto bad=boot;bad[byte/4]^=1U<<((byte%4)*8);
   assert(!P::Frame(outer.data(),layer,bank,workspace,bad.data()));refusals++;
  }
  assert(!P::Frame(outer.data(),3,bank,workspace,boot.data())
   && !P::Frame(outer.data(),layer,7,workspace,boot.data())
   && !P::Frame(outer.data(),layer,bank,workspace+4096,boot.data()));refusals+=3;
  if(layer){auto bad=outer;bad[68]=(bank+1)%7;assert(!P::Frame(bad.data(),layer,bank,workspace,boot.data()));refusals++;}
 }
 printf("PASS prefix DATA geometry%u/frames%u/refusals%u/3fixed packages+all7banks/disjoint history+workspace/bounded fields+owner source/old boot refused;NO IO/native/producer/token/Work/TPS\n",geometry,frames,refusals);
}
