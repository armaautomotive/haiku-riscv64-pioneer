/* SPDX-License-Identifier: MIT */
// Immutable package/session/job DATA only; no execution or native authority.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../PioneerQ38TokenPackage.h"
#include "../PioneerQ38GlobalRedistributedTokenPackage.h"
#include "../PioneerQ38TokenLayout.h"
#include "../PioneerQ38GlobalHeadPlan.h"
#include "../PioneerQ38GlobalOutputClosure.h"
#include "../PioneerQ38MixedRecurrentProjectionPlan.h"
#include "../PioneerQ38TokenMetadata.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnTaskPlacement.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>
namespace T=PioneerQ38TokenLayout;namespace C=T::C;namespace S=PioneerQ38TokenStateLayout;
namespace M=PioneerQ38TokenMetadata;namespace RP=PioneerQ38RecurrentPrefixLayout;namespace Route=PioneerQ38GdnTaskPlacement;
int main(){
 if(false){uint32_t data[128]={};(void)M::Outer(data,0,0,0,0,0,data);(void)M::Recipe(data,0,0,0,0);
  (void)M::Conv(data,0,0,0,0);(void)M::Dense(data,3,0,0);}
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 std::array<uint32_t,1920> sources;for(unsigned i=0;i<sources.size();i++)sources[i]=0x76540001+i;
 const PioneerQ38GdnIdentity id={1,2,3,4};unsigned packages=0,words=0,jobs=0,refusals=0,redistributedPackages=0,routedTransforms=0;
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
   if(!bank && !C::Dense(layer)){
    namespace RL=PioneerQ38RedistributedTokenLayout;
    auto routed=std::make_unique<PioneerQ38GlobalRedistributedTokenPackage>();
    const unsigned slot=0;
    if(layer==0){
     auto nonzeroSlot=std::make_unique<PioneerQ38GlobalRedistributedTokenPackage>();
     assert(!nonzeroSlot->Build(*canonical,layer,bank,1,id,sources.data(),bank,workspace+0x600000));
    }
    assert(routed->Build(*canonical,layer,bank,slot,id,sources.data(),bank,workspace+0x600000));
    assert(routed->Bytes()==C::Stride && routed->Matches(*canonical));
    std::array<uint8_t,64> prefixA={},prefixB={};
    assert(package->Read(0,prefixA.data(),prefixA.size()) && routed->Read(0,prefixB.data(),prefixB.size()) && prefixA==prefixB);
    std::array<uint8_t,16384> tableBytes={};assert(routed->Read(RL::Offset,tableBytes.data(),tableBytes.size()));
    for(unsigned i=0;i<tableBytes.size()/4;i++){
     const unsigned word=i;
     uint32_t expectedWord;
     if(word<RL::HeaderWords)expectedWord=routed->GdnHeader()[word];
     else{unsigned recordWord=word-RL::HeaderWords,worker=recordWord/RL::RecordWords+1,index=recordWord%RL::RecordWords;
      expectedWord=routed->GdnWorker(worker)[index];}
     uint32_t observed=uint32_t(tableBytes[i*4])|(uint32_t(tableBytes[i*4+1])<<8)
      |(uint32_t(tableBytes[i*4+2])<<16)|(uint32_t(tableBytes[i*4+3])<<24);
     assert(observed==expectedWord);
    }
    std::array<uint8_t,64> stateA={},stateB={};
    assert(package->Read(S::Offset,stateA.data(),stateA.size()) && routed->Read(S::Offset,stateB.data(),stateB.size())
     && stateA==stateB);
    std::array<uint8_t,64> slackA={},slackB={};
    assert(package->Read(RL::End,slackA.data(),slackA.size()) && routed->Read(RL::End,slackB.data(),slackB.size())
     && slackA==slackB);
    assert(!routed->Read(RL::Offset+1,tableBytes.data(),64));
    uint32_t ordinal=0;assert(Route::LayerOrdinal(layer,ordinal));
    std::vector<uint8_t> originalGdn(RP::GdnBytes);
    for(unsigned at=0;at<RP::GdnBytes;at+=16384){
     unsigned bytes=RP::GdnBytes-at;if(bytes>16384)bytes=16384;
     assert(package->Read(RP::GdnOffset+at,originalGdn.data()+at,bytes));
    }
    for(unsigned logical=0;logical<Route::LogicalWorkers;logical++){
     Route::Route worker={};assert(Route::Map(ordinal,logical,worker));
     std::array<uint32_t,192> original={},candidate={};
     for(unsigned i=0;i<192;i++){
      const unsigned at=logical*768+i*4;
      original[i]=uint32_t(originalGdn[at])|uint32_t(originalGdn[at+1])<<8
       |uint32_t(originalGdn[at+2])<<16|uint32_t(originalGdn[at+3])<<24;
      candidate[i]=routed->GdnWorker(worker.physicalWorker)[i];
     }
     const bool originalReady=M::Gdn(original.data(),layer,logical,0);
     const bool candidateReady=M::Gdn(candidate.data(),layer,logical,0);
     assert(originalReady && (candidateReady==(worker.physicalWorker==logical+1)));
     if(candidateReady)assert(original==candidate);
     const unsigned positions[]={0,1,32,8191};
     for(unsigned position:positions){
      auto routedRecord=candidate;
      assert(M::GdnRedistributed(routedRecord.data(),layer,worker.physicalWorker,position,position));
      if(!position)assert(routedRecord==candidate);
      assert(routedRecord[6]==M::Phase(position,layer)+5 && routedRecord[40]==position
       && routedRecord[64+8]==M::Phase(position,layer)+5 && routedRecord[64+40]==position
       && routedRecord[64+11]==(position&1) && routedRecord[128+6]==M::Phase(position,layer)+5
       && routedRecord[128+40]==position && routedRecord[128+41]==(position&1));
      routedTransforms++;
      Route::Route inverse={};unsigned mapped=~0U;
      assert(Route::LogicalWorker(layer,worker.physicalWorker,mapped) && mapped==logical
       && Route::Map(ordinal,mapped,inverse) && inverse.physicalWorker==worker.physicalWorker);
     }
     auto untouched=candidate;
     assert(!M::GdnRedistributed(untouched.data(),layer,worker.physicalWorker+1,0,0)
      && untouched==candidate);
     assert(!M::GdnRedistributed(untouched.data(),layer,worker.physicalWorker,1,0)
      && untouched==candidate);
    }
    if(layer==0){
     uint32_t* changed=const_cast<uint32_t*>(routed->GdnWorker(1));changed[128+15]^=1;
     assert(!routed->Matches(*canonical));changed[128+15]^=1;assert(routed->Matches(*canonical));
    }
    redistributedPackages++;
   }
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
 assert(routedTransforms==Route::RecurrentLayers*Route::LogicalWorkers*4);
 printf("PASS persistent token DATA %u complete512KiB packages/%u words exact including canonical spans+zero slack/self+canonical-alias refusals;redistributed GDN token packages%u/slot-aware tables in prior slack/preserved state spans/%u physical-indexed transforms validated at four positions;7banks/%u position-aware vocab jobs EXACT vs independently rebuilt output plans/%u refusals;NO instructions/state commit/source/native/physical/Work/TPS grant\n",packages,words,redistributedPackages,routedTransforms,jobs,refusals);
}
