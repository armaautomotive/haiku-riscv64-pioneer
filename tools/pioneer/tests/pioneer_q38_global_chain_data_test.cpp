/* SPDX-License-Identifier: MIT */
// Canonical DATA only,not source/initialized pages/all64/launch authority.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../PioneerQ38GlobalHeadPlan.h"
#include "../PioneerQ38GlobalChainPackage.h"
#include "../PioneerQ38GlobalRecurrentDispatch.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>
namespace C=PioneerQ38GlobalChainLayout;namespace H=PioneerQ38GlobalHeadLayout;namespace G=H::G;
int main(){
 static_assert(unsigned(C::HeadMagic)!=unsigned(TT_Q38_GDN_GATED_MARKER) && unsigned(C::CandidateMagic)!=unsigned(TT_Q38_GDN_RENDEZVOUS_MARKER),"new header/result records must not reuse older GDN tags");
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 std::array<uint32_t,1920> sources={};for(unsigned i=0;i<sources.size();i++)sources[i]=0x76540000+i+1;
 PioneerQ38GdnIdentity id={1,2,3,4};unsigned jobs=0,mutations=0,dispatch=0;
 for(unsigned bank=0;bank<7;bank++){
  unsigned workspace=canonical->Base+canonical->Workspace(bank)->offset;
  auto package=std::make_unique<PioneerQ38GlobalChainPackage>();assert(package->Build(*canonical,0,bank,id,sources.data(),bank,workspace+0x600000));
  std::array<uint32_t,64> boot;for(unsigned i=0;i<64;i++)boot[i]=package->Boot()[i];
  assert(C::BootShape(boot.data()) && !C::Old::A::BootShape(boot.data()) && C::LayerFrame(package->Outer(),0,bank,workspace,boot.data()));
  auto plan=std::make_unique<PioneerQ38GlobalHeadPlan>();assert(plan->Build(*canonical,bank,boot.data()) && plan->Matches(*canonical) && !plan->Build(*canonical,bank,boot.data()));
  assert(H::Shape(plan->Data(),boot.data(),bank,workspace));
  auto global=std::make_unique<PioneerQ38GlobalOutputPlan>();assert(global->Build(*canonical,0,id,sources.data()));
  assert(plan->Data()[6]==global->NormBank() && plan->Data()[7]==global->NormAddress());
  std::vector<unsigned> covered(248320,0);
  for(unsigned ordinal=0;ordinal<973;ordinal++){
   unsigned wave=ordinal/119,worker=ordinal%119+1,previous=worker*17+wave;
   std::array<uint32_t,64> b,j,expected;assert(global->WorkerBoot(worker,b.data()) && global->Job(wave,worker,previous,expected.data()));
   assert(H::Job(plan->Data(),ordinal,previous,b.data(),j.data()) && j==expected);
   for(unsigned i=0;i<j[23]*16;i++)assert(covered.at(j[28]+i)++==0);jobs++;
  }
  for(unsigned count:covered)assert(count==1);
  auto* raw=const_cast<uint32_t*>(plan->Data());
  for(unsigned word=0;word<128;word++){
   raw[word]^=1;assert(!plan->Matches(*canonical));raw[word]^=1;assert(plan->Matches(*canonical));mutations++;
  }
  std::array<uint32_t,64> b,out;assert(global->WorkerBoot(1,b.data()));out.fill(0xa55aa55a);const auto saved=out;
  assert(!H::Job(plan->Data(),973,0,b.data(),out.data()) && out==saved);
  assert(!H::Job(plan->Data(),0,0x7ffffffe,b.data(),out.data()) && out==saved);
  assert(!H::Job(plan->Data(),0,0,b.data(),nullptr));
  assert(!H::Job(plan->Data(),0,0,b.data(),raw) && plan->Matches(*canonical));
  assert(!H::Job(plan->Data(),0,0,b.data(),b.data()) && G::BootShape(b.data()));
  // NEW G envelope preserves exact existing numeric Conv/GDN descriptors.
  auto recurrent=std::make_unique<PioneerQ38MixedRecurrentBlockPlan>();
  assert(recurrent->Build(*canonical,0,0,0,bank,0,1001,1,id,sources.data(),bank,workspace+0x600000));
  for(unsigned worker=1;worker<=119;worker++){
   assert(global->WorkerBoot(worker,b.data()));const auto* table=recurrent->Conv()+worker*384;
   assert(PioneerQ38GlobalRecurrentDispatch::Binding(b.data(),table) && !PioneerQ38MixedRecurrentDispatch::Binding(b.data(),table));
   assert(PioneerQ38GlobalRecurrentDispatch::Conv(b.data(),table,7,out.data()) && out[0]==G::Magic && G::JobShape(out.data(),b.data(),7,0));dispatch++;
   if(worker<=96){const auto* gdn=recurrent->Gdn()+(worker-1)*192;
    assert(PioneerQ38GlobalRecurrentDispatch::Gdn(b.data(),gdn,gdn+64,gdn+128,8,out.data()) && out[0]==G::Magic && G::JobShape(out.data(),b.data(),8,0));dispatch++;
   }
  }
 }
 for(unsigned layer=0;layer<64;layer++){
  unsigned workspace=canonical->Base+canonical->Workspace(0)->offset;
  auto package=std::make_unique<PioneerQ38GlobalChainPackage>();assert(package->Build(*canonical,layer,0,id,sources.data(),0,workspace+0x600000) && package->Matches(*canonical));
  assert(C::LayerFrame(package->Outer(),layer,0,workspace,package->Boot()) && !C::Old::A::BootShape(package->Boot()));
  std::array<uint8_t,512> first;assert(package->Read(0,first.data(),512) && !memcmp(first.data(),package->Outer(),512));
  std::array<uint8_t,64> last;assert(package->Read(package->Bytes()-64,last.data(),64));
  assert(!package->Read(0,reinterpret_cast<uint8_t*>(package.get()),64) && package->Matches(*canonical));
 }
 printf("PASS QGC1 DATA/7 banks/%u jobs EXACT vs independent existing canonical scheduler/ALL248320 rows once x7/%u header mutations/%u NEW G Conv+GDN envelopes/oldboot+envelope rejection/noalias+no-partial-output;NO instructions/all64/source/native/physical/token/Work/TPS grant\n",jobs,mutations,dispatch);
}
