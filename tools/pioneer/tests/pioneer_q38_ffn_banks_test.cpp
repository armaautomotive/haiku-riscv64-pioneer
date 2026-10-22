/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_Q38_GDN_MUTABLE_STATE
#endif
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#define PIONEER_Q38_FIXTURE_ONLY
#include "pioneer_qwen38_gguf_test.cpp"
#include "../PioneerQ38FfnScratchLayout.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnBanks.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanAuthority.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CohortProtocol.h"
#include <type_traits>
template<class T,class=void> struct PublicFfnCommit:std::false_type {};
template<class T> struct PublicFfnCommit<T,std::void_t<decltype(std::declval<T&>().CommitInitialization(
 std::declval<const PioneerQ38FfnIdentity&>(),0U,0U,1U))>>:std::true_type {};
template<class T,class=void> struct PublicFfnPublish:std::false_type {};
template<class T> struct PublicFfnPublish<T,std::void_t<decltype(std::declval<T&>().PublishInitialization(
 std::declval<const PioneerQ38FfnIdentity&>()))>>:std::true_type {};
static_assert(!PublicFfnCommit<PioneerQ38FfnBanks>::value && !PublicFfnPublish<PioneerQ38FfnBanks>::value,"controller-only ledger/seal");
static_assert(!std::is_copy_constructible<PioneerQ38FfnBanks>::value && !std::is_move_constructible<PioneerQ38FfnBanks>::value,"sticky exact owner");
static const PioneerQ38FfnIdentity FfnIdentity={1,2,3,4};
struct FfnBankFixture {
 BanksMock io;tt_dram_samples_result proof=BankProof(io);PioneerResidentBanks banks;PioneerQ38FfnBanks state;
 std::vector<uint8_t> learned=std::vector<uint8_t>(PioneerResidentBanks::Pages,0);
 FfnBankFixture(){assert(banks.Bind(proof));}
 bool Bind(unsigned bank,uint64_t offset){return state.Bind(banks,proof,FfnIdentity,learned.data(),unsigned(learned.size()),bank,offset);}
 void Inert()const{assert(!io.Mock::writes && !io.requests);}
};
int main() {
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;
 Fixture file;PioneerQwen38Gguf model;assert(model.Open(file.file));unsigned cases=0,refusals=0;
 for(unsigned context:{1U,8192U,32768U})for(bool two:{false,true}) {
  PioneerQwen38PackedPlan plan;assert(plan.Build(model,context,true,two));
  for(unsigned bank=0;bank<7;bank++) {
   PioneerQ38FfnScratchLayout layout;assert(PioneerQ38FfnScratchLayout::Describe(model,plan,bank,layout));
   const auto& workspace=plan.Parts()[layout.workspaceFragment];
   const uint64_t offset=workspace.proposedAddress-PioneerQwen38PackedPlan::Base;
   FfnBankFixture f;f.learned[bank*786432]=1;const auto frozen=f.learned;
   assert(f.Bind(bank,offset) && !f.Bind(bank,offset));cases++;
   assert(f.state.MatchesInitializationOwner(f.banks,f.proof,FfnIdentity));
   unsigned progress=99;assert(f.state.InitializationProgress(FfnIdentity,progress) && !progress && !f.state.InitializationPublished(FfnIdentity));
   for(unsigned first=0;first<PioneerQ38FfnBanks::Pages;first++) {
    PioneerQ38FfnExtent e={};unsigned count=PioneerQ38FfnBanks::Pages-first;
    assert(f.state.FreshInitializationSpan(FfnIdentity,0,first,count,e) && e.bank==bank && e.pages==count && e.bytes==count*4096
     && e.address==PioneerResidentBanks::Base+offset+first*4096 && e.route==PioneerQ38SpanRoute(bank)
     && e.index==bank*786432+offset/4096+first);
    uint32_t a=99,r=99,p=99;
    assert(!f.banks.Page(f.proof,bank,offset+first*4096,a,r,p) && a==99 && r==99 && p==99);
    assert(!f.banks.Range(f.proof,bank,offset+first*4096,1));
    PioneerQ38SpanExtent span;
    assert(!PioneerQ38SpanAuthorize(f.banks,f.proof,f.learned.data(),unsigned(f.learned.size()),bank,offset+first*4096,4096,span));
   }
   const uint64_t end=offset+PioneerQ38FfnBanks::Bytes;
   assert(f.banks.ImmutableRange(f.proof,bank,offset-1,1) && !f.banks.ImmutableRange(f.proof,bank,offset-1,2)
    && !f.banks.ImmutableRange(f.proof,bank,end-1,1) && f.banks.ImmutableRange(f.proof,bank,end,1));
   assert(!f.banks.Range(f.proof,bank,offset-4096,2) && !f.banks.Range(f.proof,bank,end-4096,2)
    && f.banks.Range(f.proof,bank,end,4) && !f.banks.Range(f.proof,bank,end,5));
   assert(f.learned==frozen);
   // Mutable exclusion wins even over an all-one learned bitmap and an
   // immutable matrix whose first page starts outside the reservation.
   std::fill(f.learned.begin(),f.learned.end(),1);
   tt_q38_cohort_part part={};part.bank=bank;part.offset=uint32_t(offset-4096);part.groups=1;
   PioneerQ38CohortExtent cohort;
   assert(!PioneerQ38CohortAuthorizePart(f.banks,f.proof,f.learned.data(),unsigned(f.learned.size()),8,5120,part,1,cohort));
   PioneerResidentMutableRange extra={unsigned((bank+1)%7),16,1};
   assert(!f.banks.ReserveMutable(f.proof,&file,&extra,1,f.learned.data(),unsigned(f.learned.size())));
   PioneerQ38FfnBanks second;assert(!second.Bind(f.banks,f.proof,FfnIdentity,f.learned.data(),unsigned(f.learned.size()),(bank+1)%7,offset));
   auto foreign=FfnIdentity;foreign.session++;auto copied=f.proof;
   assert(!f.state.MatchesInitializationOwner(f.banks,copied,FfnIdentity));
   for(unsigned bad=0;bad<7;bad++) {
    PioneerQ38FfnExtent e={99,99,99,99,99,99};
    assert(!f.state.FreshInitializationSpan(bad==6?foreign:FfnIdentity,bad==0?1:0,
     bad==1?74:bad==2?UINT32_MAX:bad==3?73:0,bad==3?2:bad==4?0:bad==5?75:1,e));
    assert(e.bank==99 && e.address==99 && e.route==99 && e.index==99 && e.pages==99 && e.bytes==99);refusals++;
   }
   f.proof.source[1]^=1;PioneerQ38FfnExtent e={};assert(!f.state.InitializeSpan(FfnIdentity,0,0,1,e));f.proof.source[1]^=1;
   f.state.Fault();assert(f.state.Unsafe() && f.banks.Quarantined() && !f.Bind(bank,offset));
   for(unsigned b=0;b<7;b++){uint32_t a=0,r=0,p=0;assert(!f.banks.Page(f.proof,b,0,a,r,p));}
   f.Inert();
  }
 }
 for(unsigned fault=0;fault<17;fault++) {
  FfnBankFixture f;auto id=FfnIdentity;unsigned bank=0,n=unsigned(f.learned.size());uint64_t offset=0x04000000;
  const uint8_t* learned=f.learned.data();auto copied=f.proof;const tt_dram_samples_result* proof=&f.proof;
  if(fault<4){if(!fault)id.session=0;if(fault==1)id.sequence=0;if(fault==2)id.modelEpoch=0;if(fault==3)id.driverGeneration=0;}
  if(fault==4)bank=7;if(fault==5)offset++;if(fault==6)offset=UINT64_MAX-4095;
  if(fault==7)offset=PioneerResidentBanks::Capacity;if(fault==8)offset=PioneerResidentBanks::Capacity-67108864+4096;
  if(fault==9)learned=nullptr;if(fault==10)n--;if(fault==11)proof=&copied;
  if(fault==12)f.proof.source[1]^=1;
  if(fault>=13 && fault<=15)f.learned[offset/4096+(fault==13?0:fault==14?37:73)]=1;
  if(fault==16)offset=UINT64_C(1)<<32;
  assert(!f.state.Bind(f.banks,*proof,id,learned,n,bank,offset));refusals++;
  if(fault==12)f.proof.source[1]^=1;
  std::fill(f.learned.begin(),f.learned.end(),0);assert(f.Bind(0,0x04000000));f.Inert(); // failure claimed nothing
 }
 printf("PASS OFFLINE FFN workspace owner:canonical plans%u/refusals%u;74pages303104B within EXISTING64MiB/all7banks/3contexts/1or2deltaslots;atomic reservation,full mutable exclusion,private ledger/seal,sticky quarantine;NO_IO_OR_COMPUTE_CAPABILITY\n",cases,refusals);
}
