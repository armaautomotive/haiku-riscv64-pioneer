/* SPDX-License-Identifier: MIT */
// Immutable package/session/job DATA only; no execution or native authority.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../PioneerQ38TokenPackage.h"
#include "../PioneerQ38TokenLayout.h"
#include "../PioneerQ38GlobalHeadPlan.h"
#include "../PioneerQ38GlobalOutputClosure.h"
#include "../PioneerQ38MixedRecurrentProjectionPlan.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>
namespace T=PioneerQ38TokenLayout;namespace C=T::C;namespace S=PioneerQ38TokenStateLayout;
int main(){
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 std::array<uint32_t,1920> sources;for(unsigned i=0;i<sources.size();i++)sources[i]=0x76540001+i;
 const PioneerQ38GdnIdentity id={1,2,3,4};unsigned packages=0,words=0,jobs=0,refusals=0;
 for(unsigned bank=0;bank<7;bank++){
  unsigned workspace=canonical->Base+canonical->Workspace(bank)->offset;
  for(unsigned layer=0;layer<64;layer++){
   if(bank && layer!=0 && layer!=3)continue;
   auto package=std::make_unique<PioneerQ38TokenPackage>();
   auto old=std::make_unique<PioneerQ38GlobalChainPackage>();
   auto state=std::make_unique<PioneerQ38TokenStatePlan>();
   assert(package->Build(*canonical,layer,bank,id,sources.data(),bank,workspace+0x600000) && package->Matches(*canonical));
   assert(old->Build(*canonical,layer,bank,id,sources.data(),bank,workspace+0x600000));
   if(!C::Dense(layer))assert(state->Build(*canonical,layer,bank));
   assert(package->Bytes()==C::Stride && !package->Build(*canonical,layer,bank,id,sources.data(),bank,workspace+0x600000));
   std::array<uint8_t,16384> data,expected;
   for(unsigned at=0;at<package->Bytes();at+=data.size()){
    assert(package->Read(at,data.data(),data.size()));
    expected.fill(0);
    if(at<old->Bytes()){
     unsigned n=old->Bytes()-at;if(n>expected.size())n=expected.size();assert(old->Read(at,expected.data(),n));
    }
    if(!C::Dense(layer) && at<=S::Offset && S::Offset-at<=data.size()-S::Bytes)
     for(unsigned i=0;i<256;i++)expected[S::Offset-at+i]=uint8_t(state->Data()[i/4]>>((i&3)*8));
    assert(data==expected);words+=data.size()/4;
   }
   data.fill(0xa5);const auto before=data;
   assert(!package->Read(1,data.data(),64) && data==before);
   assert(!package->Read(C::Stride,data.data(),64) && data==before);
   assert(!package->Read(0,reinterpret_cast<uint8_t*>(package.get()),64) && package->Matches(*canonical));
   assert(!package->Read(0,reinterpret_cast<uint8_t*>(canonical.get()),64) && canonical->Ready());
   refusals+=4;packages++;
  }
  auto root=std::make_unique<PioneerQ38GlobalChainPackage>();assert(root->Build(*canonical,0,bank,id,sources.data(),bank,workspace+0x600000));
  auto head=std::make_unique<PioneerQ38GlobalHeadPlan>();assert(head->Build(*canonical,bank,root->Boot()));
  std::array<uint32_t,128> session={};const unsigned h[]={T::Magic,1,128,bank,workspace,8192,2,248319};
  for(unsigned i=0;i<8;i++){session[i]=h[i];session[24+i]=root->Boot()[8+i];}
  for(unsigned i=0;i<16;i++)session[8+i]=sources[i];session[32]=1;session[33]=248318;session[34]=248317;session[35]=8192;
  assert(T::Session(session.data(),root->Boot(),bank,workspace));
  for(unsigned i=0;i<128;i++){
   if(i==5 || i==6 || i==7 || (i>=32 && i<=34))continue;
   session[i]^=1;assert(!T::Session(session.data(),root->Boot(),bank,workspace));session[i]^=1;refusals++;
  }
  for(unsigned position:{0U,1U,24U,4095U,8191U}){
   auto independent=std::make_unique<PioneerQ38GlobalOutputPlan>();assert(independent->Build(*canonical,position,id,sources.data()));
   for(unsigned ordinal=0;ordinal<973;ordinal++){
    unsigned worker=ordinal%119+1,wave=ordinal/119,previous=1000+worker*17+position*73;
    std::array<uint32_t,64> boot,actual,expected;
    assert(independent->WorkerBoot(worker,boot.data()) && independent->Job(wave,worker,previous,expected.data()));
    assert(T::Job(head->Data(),position,ordinal,previous,boot.data(),actual.data()) && actual==expected);jobs++;
    const auto before=actual;assert(!T::Job(head->Data(),8192,ordinal,previous,boot.data(),actual.data()) && actual==before);refusals++;
    if(position<8191 && wave==(worker<=21?8U:7U) && (worker==1 || worker==21 || worker==22 || worker==119)){
     auto next=std::make_unique<PioneerQ38MixedRecurrentProjectionPlan>();unsigned role=worker<=3?10:5,offset=role==10?0:2;
     assert(next->Build(*canonical,0,role,position+1,17,bank,1001+(position+1)*1024+offset,(position+1)*64+1,id,sources.data()));
     assert(PioneerQ38GlobalOutputClosure::Job(head->Data(),root->Boot(),boot.data(),next->Frame(),expected[1],expected.data()));
     expected[7]^=4096;
     assert(!PioneerQ38GlobalOutputClosure::Job(head->Data(),root->Boot(),boot.data(),next->Frame(),expected[1],expected.data()));refusals++;
    }
   }
  }
 }
 printf("PASS persistent token DATA %u complete512KiB packages/%u words exact including canonical spans+zero slack/self+canonical-alias refusals;7banks/%u position-aware vocab jobs EXACT vs independently rebuilt output plans/%u refusals;NO instructions/state commit/source/native/physical/Work/TPS grant\n",packages,words,jobs,refusals);
}
