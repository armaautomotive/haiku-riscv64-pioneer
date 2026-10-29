/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../PioneerQ38TokenStartupPlan.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>
namespace T=PioneerQ38TokenLayout;namespace C=T::C;
static uint32_t Word(const uint8_t* p){return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
int main(int argc,char** argv){
 assert(argc==1 || (argc==2 && !strcmp(argv[1],"--bindings-only")));const bool bindingsOnly=argc==2;
 setvbuf(stdout,nullptr,_IOLBF,0);
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 auto other=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(other->Build());
 std::array<uint32_t,1920> sources;for(unsigned i=0;i<sources.size();i++)sources[i]=0x76540001+i;
 const PioneerQ38GdnIdentity id={UINT64_C(1)<<40|1,UINT64_C(2)<<40|2,UINT64_C(3)<<40|3,UINT64_C(4)<<40|4};
 const uint32_t prompt[]={248319,17};unsigned packages=0,refusals=0;
 for(unsigned bank=0;bank<7;bank++){
  printf("START startup DATA bank%u/all120 boots/fullPackageCopies%d/no execution\n",bank,int(!bindingsOnly));
  auto plan=std::make_unique<PioneerQ38TokenStartupPlan>();
  assert(!plan->MatchesPublishedBinding(*canonical,bank,id,sources.data()));
  assert(plan->Build(*canonical,bank,id,sources.data(),prompt,2,8192,true,248318,248317));
  assert(plan->Matches(*canonical) && plan->MatchesBinding(*canonical,bank,id,sources.data()) && !plan->Matches(*other)
   && plan->MatchesPublishedBinding(*canonical,bank,id,sources.data()) && !plan->MatchesPublishedBinding(*other,bank,id,sources.data())
   && !plan->MatchesPublishedBinding(*canonical,(bank+1)%7,id,sources.data()) && !plan->MatchesPublishedBinding(*canonical,bank,id,nullptr));
  assert(!plan->Build(*canonical,bank,id,sources.data(),prompt,2,8192,true,248318,248317) && plan->Matches(*canonical));
  assert(plan->Bank()==bank && plan->Workspace()==canonical->Base+canonical->Workspace(bank)->offset);
  auto root=std::make_unique<PioneerQ38GlobalChainPackage>();
  assert(root->Build(*canonical,0,bank,id,sources.data(),bank,plan->Workspace()+0x600000));
  for(unsigned i=0;i<64;i++)assert(plan->Boot()[i]==root->Boot()[i]);
  auto outputPlan=std::make_unique<PioneerQ38GlobalOutputPlan>();assert(outputPlan->Build(*canonical,0,id,sources.data()));
  for(unsigned worker=0;worker<120;worker++){
   std::array<uint32_t,64> actual,expected;assert(plan->ReadBoot(worker,actual.data()));
   if(!worker)for(unsigned i=0;i<64;i++)assert(actual[i]==root->Boot()[i]);
   else{assert(outputPlan->WorkerBoot(worker,expected.data()));assert(actual==expected);}
  }
  if(!bindingsOnly)for(unsigned layer=0;layer<64;layer++){
   auto independent=std::make_unique<PioneerQ38TokenPackage>();
   assert(independent->Build(*canonical,layer,bank,id,sources.data(),bank,plan->Workspace()+0x600000));
   uint32_t address=0;assert(plan->PackageAddress(layer,address) && address==plan->Workspace()+C::Offset+layer*C::Stride);
   std::array<uint8_t,16384> actual,expected,published;
   for(unsigned at=0;at<C::Stride;at+=actual.size()){
    assert(plan->ReadPackage(layer,at,actual.data(),actual.size()) && independent->Read(at,expected.data(),expected.size()));
    assert(actual==expected && plan->ReadPublishedPackage(layer,at,published.data(),published.size()) && actual==published);
   }
   actual.fill(0x5a);const auto before=actual;
   assert(!plan->ReadPackage(layer,1,actual.data(),64) && actual==before);
   assert(!plan->ReadPackage(layer,C::Stride,actual.data(),64) && actual==before);
   assert(!plan->ReadPackage(layer,0,reinterpret_cast<uint8_t*>(plan.get()),64));
   assert(!plan->ReadPackage(layer,0,reinterpret_cast<uint8_t*>(canonical.get()),64));refusals+=4;packages++;
  }
  std::array<uint8_t,512> session,headBytes;
  assert(plan->ReadSession(0,session.data(),session.size()) && plan->ReadHead(0,headBytes.data(),headBytes.size()));
  std::array<uint32_t,128> decoded;for(unsigned i=0;i<128;i++)decoded[i]=Word(session.data()+i*4);
  assert(T::Session(decoded.data(),plan->Boot(),bank,plan->Workspace()) && decoded[5]==8192 && decoded[6]==2 && decoded[7]==prompt[0]
   && decoded[32]==1 && decoded[33]==248318 && decoded[34]==248317);
  auto head=std::make_unique<PioneerQ38GlobalHeadPlan>();assert(head->Build(*canonical,bank,plan->Boot()));
  for(unsigned i=0;i<128;i++)assert(Word(headBytes.data()+i*4)==head->Data()[i]);
  std::array<uint8_t,16384> rawPrompt;
  for(unsigned at=0;at<32768;at+=rawPrompt.size()){
   assert(plan->ReadPrompt(at,rawPrompt.data(),rawPrompt.size()));
   for(unsigned i=0;i<rawPrompt.size()/4;i++){unsigned index=at/4+i;assert(Word(rawPrompt.data()+i*4)==(index<2?prompt[index]:0U));}
  }
  auto before=session;
  assert(!plan->ReadSession(512,session.data(),64) && session==before);
  assert(!plan->ReadSession(0,reinterpret_cast<uint8_t*>(const_cast<uint32_t*>(plan->Boot())),64));
  assert(!plan->ReadPrompt(32768,session.data(),64) && session==before);
  assert(!plan->ReadHead(1,session.data(),64) && session==before);
  assert(!plan->ReadPackage(64,0,session.data(),64) && session==before);refusals+=5;
  std::array<uint32_t,64> rejectedBoot;rejectedBoot.fill(0x5a5a5a5a);const auto bootBefore=rejectedBoot;
  assert(!plan->ReadBoot(120,rejectedBoot.data()) && rejectedBoot==bootBefore);
  assert(!plan->ReadBoot(0,const_cast<uint32_t*>(plan->Boot())) && plan->Matches(*canonical));refusals+=2;
  sources[1919]^=1;assert(!plan->MatchesBinding(*canonical,bank,id,sources.data()));sources[1919]^=1;
  assert(plan->MatchesBinding(*canonical,bank,id,sources.data()));
  auto changed=id;changed.driverGeneration++;assert(!plan->MatchesBinding(*canonical,bank,changed,sources.data()));refusals+=2;
  assert(!plan->MatchesPublishedBinding(*canonical,bank,changed,sources.data()));
  for(unsigned i=0;i<1920;i++){
   sources[i]^=1;assert(!plan->MatchesPublishedBinding(*canonical,bank,id,sources.data()));sources[i]^=1;refusals++;
  }
  assert(plan->MatchesBinding(*canonical,bank,id,sources.data()) && plan->MatchesPublishedBinding(*canonical,bank,id,sources.data()));
  if(!bank){
   std::vector<uint8_t> ledger(PioneerResidentBanks::Pages);unsigned parts=0;
   std::array<unsigned,65> last={};
   for(unsigned i=0;i<canonical->Count();i++){
    const auto* p=canonical->Part(i);if(p->kind!=canonical->WeightKind)continue;
    unsigned first=p->bank*unsigned(canonical->Capacity/4096)+p->offset/4096,count=p->allocated/4096;
    for(unsigned page=0;page<count;page++)ledger[first+page]=1;
    last[p->layer]=first+count-1;parts++;
   }
   assert(parts==3551 && plan->LearnedPagesPresent(ledger.data(),ledger.size()));
   assert(!plan->LearnedPagesPresent(nullptr,ledger.size()) && !plan->LearnedPagesPresent(ledger.data(),ledger.size()-1));
   for(unsigned layer=0;layer<=64;layer++)for(unsigned bad:{0U,2U}){
    ledger[last[layer]]=uint8_t(bad);assert(!plan->LearnedPagesPresent(ledger.data(),ledger.size()));
    ledger[last[layer]]=1;refusals++;
   }
   assert(plan->LearnedPagesPresent(ledger.data(),ledger.size()));
   puts("PASS full851 learned-page PRESENCE/all3551 parts including embedding+vocab+norm/all65 layer/global missing-or-non1 endpoints refuse;NOT upload authentication or state initialization");
  }
  printf("PASS startup DATA bank%u/all120 boots/root+head+session+prompt exact/fullPackageCopies%d/no execution\n",bank,int(!bindingsOnly));
 }
 for(unsigned prompts:{2U,8192U}){
  std::vector<uint32_t> input(prompts);for(unsigned i=0;i<prompts;i++)input[i]=(i*29+17)%248320;
  input[0]=248319;auto plan=std::make_unique<PioneerQ38TokenStartupPlan>();
  assert(plan->Build(*canonical,0,id,sources.data(),input.data(),prompts,prompts==2?3:8192,false,248318,248317));
  std::array<uint8_t,512> raw;assert(plan->ReadSession(0,raw.data(),512));
  assert(Word(raw.data()+5*4)==(prompts==2?3U:8192U) && Word(raw.data()+6*4)==prompts && !Word(raw.data()+32*4));
  assert(plan->ReadPrompt(32768-64,raw.data(),64));
  for(unsigned i=0;i<16;i++)assert(Word(raw.data()+i*4)==(prompts==8192?input[8176+i]:0U));
 }
 puts("PASS startup session EOS-disabled3-position prompt2 and maximum8192-token raw prompt/full last64B serialization;not a qualified context size or model execution");
 for(unsigned bad=0;bad<9;bad++){
  auto rejected=std::make_unique<PioneerQ38TokenStartupPlan>();uint32_t invalid[]={248320,17};
  auto identity=id;if(bad==7)identity.session=0;
  const auto savedSource=sources[119*16];if(bad==8)sources[119*16]=0;
  assert(!rejected->Build(*canonical,bad==0?7:0,identity,sources.data(),bad==1?invalid:prompt,bad==2?0:2,
   bad==3?1:bad==4?8193:3,true,bad==5?248320:248318,bad==6?248320:248317));
  sources[119*16]=savedSource;
  std::array<uint8_t,64> output;output.fill(0x5a);const auto before=output;uint32_t address=0xdeadbeef;
  assert(!rejected->Boot() && !rejected->Matches(*canonical) && !rejected->PackageAddress(0,address) && address==0xdeadbeef
   && !rejected->MatchesPublishedBinding(*canonical,0,id,sources.data())
   && !rejected->ReadSession(0,output.data(),64) && output==before);refusals++;
 }
 puts("PASS private completed-build publication ONLY after full strict64-layer verification/all1920 source mutations/foreign canonical+bank+generation+unbuilt+failed-build refusals;strict API retained,NO native lifetime/corruption/launch grant");
 printf("PASS STARTUP DATA/%u full512KiB packages EXACT vs independent per-layer serialization/7banks/all120 boots/root+head+session+8192 padded prompt/high64bit identity+all1920 source binding/%u refusals+unchanged output/fullPackageCopies%d;NO native owner/RUN/STOP/physical/Work/TPS grant\n",packages,refusals,int(!bindingsOnly));
}
