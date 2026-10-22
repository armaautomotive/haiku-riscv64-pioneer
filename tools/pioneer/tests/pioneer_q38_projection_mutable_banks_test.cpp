/* SPDX-License-Identifier: MIT */
// Actual canonical atomic ownership,NO device initialization/producer/IO.
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionMutableBanks.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GraphMutableBanks.h"
#include <memory>
#include <type_traits>
template<class T,class=void>struct ProjectionPublicAttach:std::false_type{};
template<class T>struct ProjectionPublicAttach<T,std::void_t<decltype(&T::AttachReservation)>>:std::true_type{};
template<class T,class=void>struct ProjectionPublicCommit:std::false_type{};
template<class T>struct ProjectionPublicCommit<T,std::void_t<decltype(&T::CommitInitialization)>>:std::true_type{};
template<class T,class=void>struct ProjectionPublicBind:std::false_type{};
template<class T>struct ProjectionPublicBind<T,std::void_t<decltype(&T::Bind)>>:std::true_type{};
static_assert(!ProjectionPublicAttach<PioneerQ38ProjectionBanks>::value && !ProjectionPublicCommit<PioneerQ38ProjectionBanks>::value
 && !ProjectionPublicBind<PioneerQ38ProjectionBanks>::value,"no public attach/commit/raw-address bind");
static_assert(!std::is_copy_constructible<PioneerQ38ProjectionMutableBanks>::value
 && !std::is_move_constructible<PioneerQ38ProjectionMutableBanks>::value,"retained HEAP joint owner");
struct ProjectionJointFixture {
 BanksMock io;tt_dram_samples_result proof=BankProof(io);PioneerResidentBanks banks;
 std::vector<uint8_t> learned=std::vector<uint8_t>(PioneerResidentBanks::Pages);
 std::unique_ptr<PioneerQ38CanonicalModelPlan> plan=std::make_unique<PioneerQ38CanonicalModelPlan>();
 std::unique_ptr<PioneerQ38ProjectionMutableBanks> graph=std::make_unique<PioneerQ38ProjectionMutableBanks>();
 PioneerQ38ProjectionIdentity id={1,2,3,4};
 ProjectionJointFixture(){assert(banks.Bind(proof) && plan->Build());}
 bool Bind(unsigned bank=0){return graph->BindCanonical(banks,proof,id,learned.data(),unsigned(learned.size()),*plan,bank);}
 void Inert()const{assert(!io.Mock::writes && !io.requests);}
};
#ifndef PIONEER_Q38_PROJECTION_JOINT_FIXTURE_ONLY
int main(){
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;
 unsigned positive=0,refusals=0,workspacePages=0,gdnRegions=0;
 for(unsigned bank=0;bank<7;bank++){
  ProjectionJointFixture f;auto before=f.learned;assert(f.Bind(bank));positive++;
  auto* p=f.graph->Projection(f.id);auto* g=f.graph->Recurrent(f.id);assert(p && g && f.graph->CanonicalModel(f.id)==f.plan.get());
  assert(f.graph->MatchesInitializationOwner(f.banks,f.proof,f.id));unsigned progress=99;
  assert(p->InitializationProgress(f.id,progress) && !progress && !p->InitializationPublished(f.id));
  assert(g->InitializationProgress(f.id,progress) && !progress && !g->InitializationPublished(f.id));
  const auto& ws=*f.plan->Workspace(bank);
  PioneerQ38ProjectionExtent all={};assert(p->InitializeSpan(f.id,0,0,512,all) && all.bytes==2097152 && all.pages==512);
  for(unsigned page=0;page<512;page++){
   PioneerQ38ProjectionExtent e={};assert(p->InitializeSpan(f.id,0,page,1,e) && e.bank==bank && e.address==PioneerResidentBanks::Base+ws.offset+page*4096
    && e.route==PioneerQ38ProjectionRoute(bank) && e.pages==1 && e.bytes==4096 && e.index==bank*786432+ws.offset/4096+page);
   uint32_t a=99,r=99,i=99;assert(!f.banks.Page(f.proof,bank,ws.offset+page*4096,a,r,i) && a==99 && r==99 && i==99);workspacePages++;refusals++;
  }
  // Future resident schedule storage is claimed NOW,not silently left
  // writable through ordinary upload or incorrectly reported initialized.
  for(unsigned page:{512U,8192U,16383U}){
   uint32_t a=99,r=99,i=99;assert(!f.banks.Page(f.proof,bank,ws.offset+page*4096,a,r,i) && a==99 && r==99 && i==99);refusals++;
  }
  {uint32_t a=0,r=0,i=0;assert(f.banks.Page(f.proof,bank,ws.offset+PioneerQ38ProjectionBanks::ReservedBytes,a,r,i));}
  // Entire canonical recurrent state shares the SAME atomic reservation;
  // neither role can be treated as learned data or a second shared owner.
  for(unsigned i=0;i<f.plan->RecurrentCount();i++){
   const auto& r=f.plan->RecurrentRegions()[i];PioneerQ38GdnBankExtent e={};
   assert(g->InitializeSpan(f.id,i,0,r.heads*16,e) && e.bank==r.bank && e.address==PioneerResidentBanks::Base+r.offset);gdnRegions++;
  }
  for(unsigned first:{0U,1U,63U,64U,73U,74U,255U,256U,511U}){
   PioneerQ38ProjectionExtent e={};assert(p->FreshInitializationSpan(f.id,0,first,512-first,e) && e.bytes==(512-first)*4096);
  }
  PioneerQ38ProjectionExtent sentinel={9,8,7,6,5,4};
  for(unsigned bad=0;bad<6;bad++){
   auto out=sentinel;assert(!p->InitializeSpan(f.id,bad==0?1:0,bad==1?512:bad==2?UINT32_MAX:0,bad==3?0:bad==4?513:bad==5?UINT32_MAX:1,out)
    && !memcmp(&out,&sentinel,sizeof(out)));refusals++;
  }
  auto copy=f.proof;auto foreign=f.id;foreign.sequence++;
  assert(!f.graph->MatchesInitializationOwner(f.banks,copy,f.id) && !f.graph->Projection(foreign) && !f.graph->Recurrent(foreign));
  auto old=std::make_unique<PioneerQ38GraphMutableBanks>();assert(!old->BindCanonical(f.banks,f.proof,f.id,f.learned.data(),unsigned(f.learned.size()),*f.plan,bank));
  auto second=std::make_unique<PioneerQ38ProjectionMutableBanks>();assert(!second->BindCanonical(f.banks,f.proof,f.id,f.learned.data(),unsigned(f.learned.size()),*f.plan,bank));
  assert(f.learned==before && !f.Bind(bank));
  if(bank&1)g->Fault();else p->Fault();
  assert(f.graph->Unsafe() && p->Unsafe() && g->Unsafe() && f.banks.Quarantined() && !f.graph->Projection(f.id) && !f.graph->Recurrent(f.id));f.Inert();
 }
 for(unsigned hit:{0U,73U,74U,255U,256U,511U,512U,8192U,16383U}){
  ProjectionJointFixture f;auto ws=*f.plan->Workspace(0);f.learned[ws.offset/4096+hit]=1;
  assert(!f.Bind() && !f.graph->Projection(f.id) && !f.graph->Recurrent(f.id) && !f.graph->Unsafe());refusals++;
  // Refusal is atomic:the previously valid first74 pages were not claimed.
  uint32_t a=0,r=0,i=0;assert(f.banks.Page(f.proof,0,ws.offset,a,r,i));f.learned[ws.offset/4096+hit]=0;assert(f.Bind());f.Inert();
 }
 {ProjectionJointFixture f;auto old=std::make_unique<PioneerQ38GraphMutableBanks>();
  assert(old->BindCanonical(f.banks,f.proof,f.id,f.learned.data(),unsigned(f.learned.size()),*f.plan,0));
  assert(!f.Bind() && !f.graph->Projection(f.id));uint32_t a=0,r=0,i=0;
  assert(f.banks.Page(f.proof,0,f.plan->Workspace(0)->offset+74*4096,a,r,i));refusals++;
 }
 for(unsigned ordinal:{0U,335U,671U}){
  ProjectionJointFixture f;const auto& region=f.plan->RecurrentRegions()[ordinal];unsigned at=region.bank*786432+region.offset/4096;
  f.learned[at]=1;assert(!f.Bind() && !f.graph->Projection(f.id) && !f.graph->Recurrent(f.id) && !f.graph->Unsafe());
  uint32_t a=0,r=0,i=0;assert(f.banks.Page(f.proof,0,f.plan->Workspace(0)->offset,a,r,i));
  f.learned[at]=0;assert(f.Bind());f.Inert();refusals++;
 }
 for(unsigned bad=0;bad<9;bad++){
  ProjectionJointFixture f;auto foreign=f.proof;auto id=f.id;if(bad<4){if(!bad)id.session=0;if(bad==1)id.sequence=0;if(bad==2)id.modelEpoch=0;if(bad==3)id.driverGeneration=0;}
  assert(!f.graph->BindCanonical(f.banks,bad==4?foreign:f.proof,id,bad==5?nullptr:f.learned.data(),unsigned(f.learned.size())-unsigned(bad==6),*f.plan,bad>=7?bad:0));
  assert(!f.graph->Projection(f.id) && !f.graph->Recurrent(f.id) && !f.graph->Unsafe());assert(f.Bind());f.Inert();refusals++;
 }
 printf("PASS actual canonical projection joint reservation banks%u/runtimePages%u/GDNregions%u/refusals%u;ONE atomic73728+16384page claim,FULL64MiB envelope protected/runtime512pages separately bounded,independent EMPTY ledgers,learned collision through last reserved page,old74page owner NOT promotable,sticky whole-context fault;NO_IO_INITIALIZATION_INPUT_PRODUCER_OR_NATIVE_GRANT\n",positive,workspacePages,gdnRegions,refusals);
}
#endif
