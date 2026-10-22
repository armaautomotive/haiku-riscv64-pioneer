/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#define PIONEER_Q38_FIXTURE_ONLY
#include "pioneer_qwen38_gguf_test.cpp"
#include "../PioneerQ38FfnScratchLayout.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GraphMutableBanks.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanAuthority.h"
#include <memory>
#include <type_traits>
static const PioneerQ38GdnIdentity GraphIdentity={1,2,3,4};
static_assert(!std::is_copy_constructible<PioneerQ38GraphMutableBanks>::value
 && !std::is_move_constructible<PioneerQ38GraphMutableBanks>::value,"exact retained joint owner");
template<class T,class=void>struct PublicAttach:std::false_type{};
template<class T>struct PublicAttach<T,std::void_t<decltype(&T::AttachReservation)>>:std::true_type{};
static_assert(!PublicAttach<PioneerQ38GdnBanks>::value && !PublicAttach<PioneerQ38FfnBanks>::value,"no public shared-owner/address hint");
struct JointFixture {
 BanksMock io;tt_dram_samples_result proof=BankProof(io);PioneerResidentBanks banks;
 std::vector<uint8_t> learned=std::vector<uint8_t>(PioneerResidentBanks::Pages,0);
 std::unique_ptr<PioneerQ38GraphMutableBanks> graph=std::make_unique<PioneerQ38GraphMutableBanks>();
 JointFixture(){assert(banks.Bind(proof));}
 bool Bind(const std::vector<PioneerQ38GdnRegion>& r,unsigned bank,uint64_t offset) {
  return graph->Bind(banks,proof,GraphIdentity,learned.data(),unsigned(learned.size()),r.data(),unsigned(r.size()),bank,offset);
 }
 void Inert(){assert(!io.Mock::writes && !io.requests);}
};
int main() {
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;
 Fixture file;PioneerQwen38Gguf model;assert(model.Open(file.file));PioneerQwen38PackedPlan plan;
 assert(plan.Build(model,8192,true,true));std::vector<PioneerQ38GdnRegion> regions;
 for(const auto& p:plan.Parts())if(p.kind==PIONEER_Q38_DELTA_STATE)
  regions.push_back({p.layer,p.role,p.firstRow,p.rows,p.bank,p.proposedAddress-PioneerQwen38PackedPlan::Base});
 assert(regions.size()==672);unsigned positives=0,refusals=0;
 for(unsigned bank=0;bank<7;bank++) {
  PioneerQ38FfnScratchLayout layout;assert(PioneerQ38FfnScratchLayout::Describe(model,plan,bank,layout));
  uint64_t offset=plan.Parts()[layout.workspaceFragment].proposedAddress-PioneerQwen38PackedPlan::Base;
  JointFixture f;const auto before=f.learned;assert(f.Bind(regions,bank,offset));positives++;
  auto* recurrent=f.graph->Recurrent(GraphIdentity);auto* ffn=f.graph->Ffn(GraphIdentity);assert(recurrent && ffn);
  assert(f.graph->MatchesInitializationOwner(f.banks,f.proof,GraphIdentity));
  assert(recurrent->MatchesInitializationOwner(f.banks,f.proof,GraphIdentity) && ffn->MatchesInitializationOwner(f.banks,f.proof,GraphIdentity));
  unsigned n=99,pages=99;assert(recurrent->InitializationRegions(GraphIdentity,n) && n==672
   && recurrent->InitializationProgress(GraphIdentity,pages) && !pages && !recurrent->InitializationPublished(GraphIdentity));
  assert(ffn->InitializationProgress(GraphIdentity,pages) && !pages && !ffn->InitializationPublished(GraphIdentity));
  // BOTH ledgers remain empty. Reservation does NOT fabricate a producer,
  // zero-CLOSE,initialized learned page or launch/terminal permission.
  for(unsigned i=0;i<regions.size();i++) {
   const auto& r=regions[i];PioneerQ38GdnBankExtent e={};unsigned count=r.heads*16;
   assert(recurrent->InitializeSpan(GraphIdentity,i,0,count,e) && e.bank==r.bank
    && e.address==PioneerResidentBanks::Base+r.offset && e.route==PioneerQ38SpanRoute(r.bank) && e.pages==count);
   for(unsigned p=0;p<count;p++) {
    uint32_t a=99,route=99,index=99;assert(!f.banks.Page(f.proof,r.bank,uint64_t(r.offset)+p*4096,a,route,index)
     && a==99 && route==99 && index==99);refusals++;
   }
  }
  for(unsigned first=0;first<74;first++) {
   PioneerQ38FfnExtent e={};assert(ffn->InitializeSpan(GraphIdentity,0,first,74-first,e) && e.bank==bank
    && e.address==PioneerResidentBanks::Base+offset+first*4096 && e.pages==74-first);
   uint32_t a=99,r=99,p=99;assert(!f.banks.Page(f.proof,bank,offset+first*4096,a,r,p) && a==99 && r==99 && p==99);refusals++;
  }
  auto copy=f.proof;auto foreign=GraphIdentity;foreign.sequence++;
  assert(!f.graph->MatchesInitializationOwner(f.banks,copy,GraphIdentity) && !f.graph->Recurrent(foreign) && !f.graph->Ffn(foreign));
  assert(!recurrent->Bind(f.banks,f.proof,GraphIdentity,f.learned.data(),unsigned(f.learned.size()),regions.data(),672)
   && !ffn->Bind(f.banks,f.proof,GraphIdentity,f.learned.data(),unsigned(f.learned.size()),bank,offset));
  PioneerQ38GraphMutableBanks second;assert(!second.Bind(f.banks,f.proof,GraphIdentity,f.learned.data(),unsigned(f.learned.size()),
   regions.data(),672,bank,offset) && !second.Recurrent(GraphIdentity) && !second.Ffn(GraphIdentity));
  assert(f.learned==before && !f.Bind(regions,bank,offset));
  // Learned all-one claims cannot bypass EITHER mutable exclusion.
  std::fill(f.learned.begin(),f.learned.end(),1);PioneerQ38SpanExtent span;
  assert(!PioneerQ38SpanAuthorize(f.banks,f.proof,f.learned.data(),unsigned(f.learned.size()),bank,offset,4096,span));
  assert(!PioneerQ38SpanAuthorize(f.banks,f.proof,f.learned.data(),unsigned(f.learned.size()),regions[0].bank,regions[0].offset,4096,span));
  // Faulting EITHER child poisons the entire reservation without I/O,
  // release or replacement. Previously borrowed other-role handles fail.
  if(bank&1)recurrent->Fault();else ffn->Fault();
  assert(f.graph->Unsafe() && recurrent->Unsafe() && ffn->Unsafe() && f.banks.Quarantined()
   && !f.graph->Recurrent(GraphIdentity) && !f.graph->Ffn(GraphIdentity) && !f.Bind(regions,bank,offset));f.Inert();
 }
 PioneerQ38FfnScratchLayout layout;assert(PioneerQ38FfnScratchLayout::Describe(model,plan,0,layout));
 const uint64_t goodOffset=plan.Parts()[layout.workspaceFragment].proposedAddress-PioneerQwen38PackedPlan::Base;
 for(unsigned fault=0;fault<25;fault++) {
  JointFixture f;auto r=regions;auto id=GraphIdentity;auto proof=f.proof;const tt_dram_samples_result* selected=&f.proof;
  unsigned bank=0,n=unsigned(f.learned.size()),count=672;uint64_t offset=goodOffset;const uint8_t* learned=f.learned.data();
  if(fault<4){if(!fault)id.session=0;if(fault==1)id.sequence=0;if(fault==2)id.modelEpoch=0;if(fault==3)id.driverGeneration=0;}
  if(fault==4)selected=&proof;if(fault==5)bank=7;if(fault==6)offset++;if(fault==7)offset=UINT64_MAX-4095;
  if(fault==8)offset=PioneerResidentBanks::Capacity-67108864+4096;if(fault==9)learned=nullptr;if(fault==10)n--;
  if(fault>=11 && fault<=13)f.learned[goodOffset/4096+(fault==11?0:fault==12?37:73)]=1;
  if(fault>=14 && fault<=16){const auto& hit=r[fault==14?0:fault==15?335:671];f.learned[hit.bank*786432+hit.offset/4096]=1;}
  if(fault==17)r[671]=r[0];if(fault==18)r[671].layer=3;if(fault==19)count=671;if(fault==20)count=673;
  if(fault==21)r[0].offset++;if(fault==22)r[0].offset=PioneerResidentBanks::Capacity;
  if(fault==23){bank=r[0].bank;assert(r[0].offset>=1048576);offset=r[0].offset-1048576;}
  if(fault==24)f.proof.source[1]^=1;
  assert(!f.graph->Bind(f.banks,*selected,id,learned,n,r.data(),count,bank,offset)
   && !f.graph->Recurrent(GraphIdentity) && !f.graph->Ffn(GraphIdentity) && !f.graph->Unsafe());refusals++;
  if(fault==24)f.proof.source[1]^=1;
  std::fill(f.learned.begin(),f.learned.end(),0);assert(f.Bind(regions,0,goodOffset));f.Inert(); // atomic refusal claimed NOTHING
 }
 printf("PASS OFFLINE joint mutable owner:canonical joint plans%u/refusals%u;673extents/73728GDN+74FFNpages,one atomic reservation,typed exact roles,independent EMPTY ledgers,whole-envelope collision guard,sticky whole-context fault;NO_IO_INITIALIZATION_PRODUCER_LAUNCH_OR_NATIVE_AUTHORITY\n",positives,refusals);
}
