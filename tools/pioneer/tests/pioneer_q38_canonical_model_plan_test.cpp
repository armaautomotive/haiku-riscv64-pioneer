/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#define PIONEER_Q38_FIXTURE_ONLY
#include "pioneer_qwen38_gguf_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GraphMutableBanks.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnCanonicalJobs.h"
#include "../PioneerQwen38FleetPlan.h"
#include <memory>
#include <type_traits>
using Canonical=PioneerQ38CanonicalModelPlan;
static_assert(!std::is_copy_constructible<Canonical>::value && !std::is_move_constructible<Canonical>::value,"one immutable HEAP plan");
static unsigned compared=0,spans=0,refusals=0;
static unsigned jobParts=0;
static void Compare(const Canonical& canonical,const PioneerQwen38Gguf& model,const PioneerQwen38PackedPlan& host) {
 const auto& parts=host.Parts();assert(canonical.Count()==parts.size());
 unsigned recurrent=0,weights=0;const auto* regions=canonical.RecurrentRegions();assert(regions && canonical.RecurrentCount()==672);
 for(unsigned i=0;i<parts.size();i++) {
  const auto& a=*canonical.Part(i);const auto& b=parts[i];
  assert(a.kind==b.kind && a.layer==b.layer && a.role==b.role && a.bank==b.bank && a.firstRow==b.firstRow
   && a.rows==b.rows && a.rowBytes==b.rowBytes && a.offset==b.proposedAddress-Canonical::Base && a.bytes==b.bytes && a.allocated==b.allocated);
  assert(!(a.offset&4095) && a.bytes && a.bytes<=a.allocated && a.allocated-a.bytes<4096
   && uint64_t(a.offset)+a.allocated<=Canonical::Capacity);
  if(a.kind==Canonical::WeightKind){assert(a.type==model.Tensors()[b.tensor].type);weights++;}
  else assert(!a.type);
  if(a.kind==Canonical::RecurrentKind){const auto& r=regions[recurrent++];
   assert(r.layer==a.layer && r.slot==a.role && r.firstHead==a.firstRow && r.heads==a.rows && r.bank==a.bank && r.offset==a.offset);}
  if(a.kind==Canonical::WorkspaceKind)assert(canonical.Workspace(a.bank)==&a && a.allocated==67108864);
  compared++;
 }
 assert(recurrent==672 && weights>3000 && !canonical.Part(unsigned(parts.size())) && !canonical.Part(UINT32_MAX));
 for(unsigned bank=0;bank<7;bank++)assert(canonical.Used(bank)==host.Used(bank));
 for(unsigned layer=0;layer<64;layer++)for(unsigned role=0;role<3;role++)for(unsigned stripe=0;stripe<7;stripe++) {
  const auto& p=*canonical.FfnStripe(layer,role,stripe);assert(p.kind==Canonical::WeightKind && p.layer==layer && p.role==role+2 && p.type==12);
  for(unsigned first=p.firstRow;first<p.firstRow+p.rows;first+=16) {
   unsigned left=p.firstRow+p.rows-first;uint64_t offset=uint64_t(p.offset)+uint64_t(first-p.firstRow)*p.rowBytes;
   assert(canonical.FfnSpan(layer,role,p.bank,offset,first,16) && canonical.FfnSpan(layer,role,p.bank,offset,first,left));spans+=2;
  }
  for(unsigned other=0;other<64;other++)if(other!=layer){assert(!canonical.FfnSpan(other,role,p.bank,p.offset,p.firstRow,p.rows));refusals++;}
  assert(!canonical.FfnSpan(layer,(role+1)%3,p.bank,p.offset,p.firstRow,p.rows));refusals++;
  for(unsigned fault=0;fault<9;fault++) {
   assert(!canonical.FfnSpan(fault==0?64:layer,fault==1?3:role,fault==2?7:p.bank,
    fault==3?UINT64_MAX:fault==4?p.offset+64:p.offset,fault==5?p.firstRow+1:fault==6?UINT32_MAX-15:p.firstRow,
    fault==7?0:fault==8?p.rows+16:p.rows));refusals++;
  }
 }
 for(unsigned layer=0;layer<64;layer++) {
  auto jobs=std::make_unique<PioneerQ38FfnCanonicalJobs>();assert(!jobs->Job(0));
  const uint64_t position=layer&1?UINT64_MAX:layer;
  assert(jobs->Build(canonical,layer,position) && jobs->Matches(canonical,layer,position)
   && !jobs->Build(canonical,layer,position) && !jobs->Matches(canonical,layer^1,position)
   && !jobs->Matches(canonical,layer,position^1) && !jobs->Job(3));
  for(unsigned role=0;role<3;role++) {
   PioneerQ38Tensor expected;assert(PioneerQ38Expected(layer,role+2,expected));const auto* tensor=model.Find(expected.name.c_str());assert(tensor);
   PioneerQwen38FleetPlan fleet;assert(fleet.Build(model,host,unsigned(tensor-model.Tensors().data()),119));
   const auto* j=jobs->Job(role);assert(j && j->peers==119 && fleet.Jobs().size()==119);
   for(unsigned w=0;w<119;w++){const auto& h=fleet.Jobs()[w];
    assert(h.worker==w && !h.wave && j->part[w].bank==h.bank && j->part[w].offset==h.proposedOffset
     && j->part[w].groups==h.groups && j->first[w]==h.firstRow);jobParts++;}
  }
 }
}
static void Reorder(Fixture& f) {
 // Reverse the ACTUAL descriptor records,not a second fake layout API.
 // Lengths/dataStart stay identical. Metadata and data bytes are untouched.
 const unsigned start=f.dimensionPositions[0]-4-8-unsigned(f.expected[0].name.size());
 std::vector<uint8_t> header(f.header.begin(),f.header.begin()+start);
 for(unsigned reverse=851;reverse;reverse--) {
  unsigned i=reverse-1,first=f.dimensionPositions[i]-4-8-unsigned(f.expected[i].name.size()),last=f.offsetPositions[i]+8;
  header.insert(header.end(),f.header.begin()+first,f.header.begin()+last);
 }
 assert(header.size()==f.header.size());f.Write(0,header);
}
int main(int argc,char** argv) {
 assert(argc==1 || argc==2);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;
 auto canonical=std::make_unique<Canonical>();
 assert(!canonical->Ready() && !canonical->Count() && !canonical->Part(0) && !canonical->FfnStripe(0,0,0)
  && !canonical->Workspace(0) && !canonical->RecurrentRegions() && !canonical->RecurrentCount() && !canonical->Used(0)
  && !canonical->FfnSpan(0,0,0,0,0,16) && !canonical->FfnPagesPresent(0,nullptr,0));
 assert(canonical->Build() && canonical->Ready());
 for(unsigned fault=0;fault<2;fault++) {
  auto invalid=std::make_unique<PioneerQ38FfnCanonicalJobs>();auto empty=std::make_unique<Canonical>();
  assert(!invalid->Build(fault?*canonical:*empty,fault?64:0,0) && !invalid->Job(0)
   && !invalid->LearnedPagesPresent(nullptr,0) && !invalid->Build(*canonical,0,0));refusals++;
 }
 const auto* first=canonical->Part(0);assert(!canonical->Build() && canonical->Ready() && canonical->Part(0)==first);
 assert(!canonical->FfnStripe(64,0,0) && !canonical->FfnStripe(0,3,0) && !canonical->FfnStripe(0,0,7)
  && !canonical->Workspace(7) && !canonical->Used(7));
 Fixture file;PioneerQwen38Gguf model;PioneerQwen38PackedPlan host;
 assert(model.Open(file.file) && host.Build(model,8192,true,true));Compare(*canonical,model,host);
 Reorder(file);assert(model.Open(file.file) && host.Build(model,8192,true,true));Compare(*canonical,model,host);
 if(argc==2) {
  FILE* real=fopen(argv[1],"rb");assert(real);PioneerQwen38Gguf actual;PioneerQwen38PackedPlan packed;
  assert(actual.Open(real) && packed.Build(actual,8192,true,true));Compare(*canonical,actual,packed);fclose(real);
  puts("PASS ACTUAL local PUBLIC GGUF descriptor/layout comparison;READ_ONLY_METADATA_NOT_FULL_SHA_AUTHENTICATION_OR_INFERENCE");
 }
 // Fixed two-slot geometry must NOT silently track a different context,
 // a one-slot proposal,row-major layout or a changed model epoch.
 for(unsigned fault=0;fault<3;fault++) {
  assert(host.Build(model,fault==0?1:8192,fault!=2,fault!=1));bool differs=host.Parts().size()!=canonical->Count();
  for(unsigned i=0;!differs && i<host.Parts().size();i++){const auto& p=host.Parts()[i];const auto& c=*canonical->Part(i);
   differs=p.kind!=c.kind || p.bank!=c.bank || p.rows!=c.rows || p.rowBytes!=c.rowBytes || p.proposedAddress-Canonical::Base!=c.offset;}
  assert(differs);refusals++;
 }
 std::vector<uint8_t> learned(PioneerResidentBanks::Pages,0);
 assert(!canonical->FfnPagesPresent(0,nullptr,unsigned(learned.size())) && !canonical->FfnPagesPresent(0,learned.data(),unsigned(learned.size())-1)
  && !canonical->FfnPagesPresent(64,learned.data(),unsigned(learned.size())) && !canonical->FfnPagesPresent(0,learned.data(),unsigned(learned.size())));
 for(unsigned i=0;i<canonical->Count();i++){const auto& p=*canonical->Part(i);if(p.kind!=Canonical::WeightKind)continue;
  unsigned at=p.bank*786432+p.offset/4096;std::fill(learned.begin()+at,learned.begin()+at+p.allocated/4096,1);}
 for(unsigned layer=0;layer<64;layer++) {
  auto jobs=std::make_unique<PioneerQ38FfnCanonicalJobs>();assert(jobs->Build(*canonical,layer,123));
  assert(canonical->FfnPagesPresent(layer,learned.data(),unsigned(learned.size())) && jobs->LearnedPagesPresent(learned.data(),unsigned(learned.size())));
  for(unsigned role=0;role<3;role++)for(unsigned stripe=0;stripe<7;stripe++) {
   const auto& p=*canonical->FfnStripe(layer,role,stripe);unsigned at=p.bank*786432+p.offset/4096,n=p.allocated/4096;
   for(unsigned offset:{0U,n/2,n-1})for(uint8_t invalid:{uint8_t(0),uint8_t(2),uint8_t(255)}) {
    learned[at+offset]=invalid;assert(!canonical->FfnPagesPresent(layer,learned.data(),unsigned(learned.size()))
     && !jobs->LearnedPagesPresent(learned.data(),unsigned(learned.size())));
    learned[at+offset]=1;refusals++;
   }
  }
 }
 const auto saved=learned;const PioneerQ38GdnIdentity identity={1,2,3,4};unsigned bound=0;
 for(unsigned bank=0;bank<7;bank++) {
  BanksMock io;tt_dram_samples_result proof=BankProof(io);PioneerResidentBanks banks;assert(banks.Bind(proof));
  auto graph=std::make_unique<PioneerQ38GraphMutableBanks>();auto absent=std::make_unique<Canonical>();
  assert(!graph->BindCanonical(banks,proof,identity,learned.data(),unsigned(learned.size()),*absent,bank));
  assert(!graph->BindCanonical(banks,proof,identity,learned.data(),unsigned(learned.size()),*canonical,7));
  assert(graph->BindCanonical(banks,proof,identity,learned.data(),unsigned(learned.size()),*canonical,bank));bound++;
  auto foreign=identity;foreign.modelEpoch++;
  assert(graph->CanonicalModel(identity)==canonical.get() && !graph->CanonicalModel(foreign));
  auto* gdn=graph->Recurrent(identity);auto* ffn=graph->Ffn(identity);unsigned pages=99;
  assert(gdn && ffn && gdn->InitializationProgress(identity,pages) && !pages && !gdn->InitializationPublished(identity)
   && ffn->InitializationProgress(identity,pages) && !pages && !ffn->InitializationPublished(identity));
  PioneerQ38FfnExtent e={};assert(ffn->InitializeSpan(identity,0,0,74,e) && e.bank==bank && e.address==Canonical::Base+canonical->Workspace(bank)->offset);
  assert(learned==saved && !io.Mock::writes && !io.requests);
  graph->Fault();assert(graph->Unsafe() && banks.Quarantined() && !graph->Recurrent(identity) && !graph->Ffn(identity) && !graph->CanonicalModel(identity));
 }
 printf("PASS OFFLINE fixed canonical model layout:%u parts/%u independent host-part comparisons/%u exact FFN subspans/%u exact119-worker job-part comparisons/%u refusals/64 learned-page ledgers/%u actual joint reservations with EMPTY initialization ledgers;ALL851 tensors,672 recurrent regions,TWO slots,8192 context,7x3GiB limits unchanged;GEOMETRY_NOT_SOURCE_AUTH_PRODUCER_OR_NATIVE_EXECUTION_SPEED\n",canonical->Count(),compared,spans,jobParts,refusals,bound);
}
