/* SPDX-License-Identifier: MIT */
// CPU-only integration of actual startup serializers with the proposed
// relocated model layout. No model payloads, firmware or device access.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_Q38_HYBRID_LAYOUT_DIAGNOSTIC 1
#define PIONEER_Q38_HYBRID_JOB_DIAGNOSTIC 1
#include "../PioneerQ38TokenStartupPlan.h"
#include "../PioneerQ38HybridModelPlan.h"
#include "../PioneerQ38HybridJobBinding.h"
#include "../PioneerQ38Bf16RootPairs.h"
#include "../PioneerQ38HybridRootPairs.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionJobs.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>

using Canonical=PioneerQ38CanonicalModelPlan;
static uint32_t Word(const uint8_t* p){
 return uint32_t(p[0]) | uint32_t(p[1])<<8 | uint32_t(p[2])<<16 | uint32_t(p[3])<<24;
}
struct RootProbe {
 const PioneerQ38ProjectionJobs& input;
 unsigned parts[119][8]={},rows[119]={},table[7][4]={};
 unsigned Part(unsigned w,unsigned i)const{
  const auto* p=input.Part(w);assert(p);
  return i==0?Canonical::Base+unsigned(p->offset):i==1?p->bank:i==2?p->groups:i==4?Route(p->bank):i==5?Source(w):0;
 }
 unsigned Row(unsigned w)const{return input.Part(w)->firstRow;}
 unsigned Route(unsigned bank)const{return PioneerQ38ProjectionRoute(bank);}
 unsigned Source(unsigned w)const{return w+1;}
 unsigned Result(unsigned w,unsigned i)const{return parts[w][i];}
 unsigned ResultRow(unsigned w)const{return rows[w];}
 unsigned Table(unsigned bank,unsigned field)const{return table[bank][field];}
 void Output(unsigned w,unsigned i,unsigned value){parts[w][i]=value;}
 void OutputRow(unsigned w,unsigned value){rows[w]=value;}
};
int main(){
 auto original=std::make_unique<Canonical>();assert(original->Build());
 PioneerQ38HybridModelPlan::Proposal proposal;
 assert(PioneerQ38HybridModelPlan::Build(*original,UINT64_C(0x1fff000000000000),proposal));
 auto hybrid=std::make_unique<Canonical>();assert(hybrid->BuildHybridDiagnostic());
 assert(original->OriginalFormatLayout() && !original->HybridFormatLayout()
  && hybrid->HybridFormatLayout() && !hybrid->OriginalFormatLayout());
 assert(PioneerQ38TokenStartupPlan::WorkerLayoutCompatible(*hybrid)==bool(PioneerQ38SelectedWorkerHybrid));
 assert(PioneerQ38TokenStartupPlan::WorkerLayoutCompatible(*original)!=bool(PioneerQ38SelectedWorkerHybrid));
 assert(hybrid->Count()==original->Count() && hybrid->Count()==proposal.parts.size());
 unsigned moved=0,eight=0,regions=0,weights=0;
 for(unsigned i=0;i<hybrid->Count();i++){
  const auto& a=*original->Part(i);const auto& b=*hybrid->Part(i);const auto& p=proposal.parts[i];
  assert(p.original==i && p.bank==a.bank && b.kind==a.kind && b.layer==a.layer && b.role==a.role
   && b.type==a.type && b.bank==a.bank && b.firstRow==a.firstRow && b.rows==a.rows && b.rowBytes==a.rowBytes
   && b.offset==p.offset && b.bytes==p.bytes && b.allocated==p.allocated);
  if(a.offset!=b.offset)moved++;
  if(p.encoding==8)eight++;
  if(b.kind==Canonical::WeightKind)weights++;
  if(b.kind==Canonical::RecurrentKind){
   const auto& r=hybrid->RecurrentRegions()[regions++];
   assert(r.layer==b.layer && r.slot==b.role && r.firstHead==b.firstRow && r.heads==b.rows
    && r.bank==b.bank && r.offset==b.offset);
  }
 }
 assert(eight==273 && weights==3551 && regions==672 && moved>0);
 for(unsigned b=0;b<7;b++)assert(hybrid->Used(b)==proposal.used[b]
  && hybrid->Workspace(b)->offset==proposal.parts[hybrid->Count()-7+b].offset);
 assert(!hybrid->BuildHybridDiagnostic() && hybrid->Ready() && !hybrid->Build());

 // Real existing projection jobs start from the relocated canonical plan.
 // Run the same ROOT pair/relocation routines, using a manifest independently
 // derived from the ORIGINAL canonical plan, and compare every resulting
 // worker address with the original-plan leaf binding for both formats.
 PioneerQ38HybridJobBinding binding;
 assert(binding.Bind(*original,UINT64_C(0x1fff000000000000)));
 std::array<uint32_t,PioneerQ38HybridJobBinding::RootManifestWords> manifest={};
 assert(binding.RootManifest(manifest.data(),manifest.size()));
 unsigned workerJobs=0;
 for(unsigned layer=0;layer<64;layer++)for(unsigned role=2;role<=4;role++){
  auto generated=std::make_unique<PioneerQ38ProjectionJobs>();
  assert(generated->Build(*hybrid,layer,role,8191) && generated->Count()==119);
  PioneerQ38PairedProjectionJobs expected;
  assert(expected.Build(*original,layer,role,8191) && expected.Count()==119);
  RootProbe root{*generated};
  for(unsigned stripe=0;stripe<7;stripe++){
   const auto* p=hybrid->ProjectionStripe(layer,role,stripe);assert(p);
   root.table[p->bank][0]=Canonical::Base+p->offset;
   root.table[p->bank][1]=manifest[(layer*3+role-2)*7+p->bank];
   root.table[p->bank][2]=p->firstRow;root.table[p->bank][3]=p->rows;
  }
  assert(PioneerQ38Bf16RootPairs::Build(root,expected.Inputs(),expected.Outputs()));
  assert(PioneerQ38HybridRootPairs::Relocate(root,layer,expected.Inputs(),expected.Outputs()));
  for(unsigned w=0;w<119;w++){
   const auto* part=expected.Part(w);assert(part);
   uint32_t address=0;
   if(layer>=48 && layer<=60){
    PioneerQ38Bfp8WorkerJob::Job job={};assert(binding.Build8(expected,layer,role,8191,w,7,job));address=job.words[3];
   }else{
    PioneerQ38Bfp4WorkerJob::Job job={};assert(binding.Build4(expected,layer,role,8191,w,7,job));address=job.words[3];
   }
   assert(root.parts[w][0]==address && root.parts[w][1]==part->bank
    && root.parts[w][2]==part->groups && root.rows[w]==part->firstRow);workerJobs++;
  }
 }
 assert(workerJobs==64*3*119);

 // The actual startup page-presence checker must reject an old-layout
 // ledger, accept all relocated learned pages, and reject a missing page
 // from every layer/global group. This is a synthetic ledger, not upload.
 std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,0);
 for(unsigned i=0;i<original->Count();i++){
  const auto& p=*original->Part(i);if(p.kind!=Canonical::WeightKind)continue;
  const unsigned first=p.bank*unsigned(Canonical::Capacity/4096)+p.offset/4096;
  for(unsigned n=0;n<p.allocated/4096;n++)ledger[first+n]=1;
 }
 assert(PioneerQ38TokenStartupPlan::CanonicalLearnedPagesPresent(*original,ledger.data(),ledger.size()));
 assert(!PioneerQ38TokenStartupPlan::CanonicalLearnedPagesPresent(*hybrid,ledger.data(),ledger.size()));
 std::fill(ledger.begin(),ledger.end(),0);std::array<unsigned,65> last={};
 for(unsigned i=0;i<hybrid->Count();i++){
  const auto& p=*hybrid->Part(i);if(p.kind!=Canonical::WeightKind)continue;
  const unsigned first=p.bank*unsigned(Canonical::Capacity/4096)+p.offset/4096,pages=p.allocated/4096;
  for(unsigned n=0;n<pages;n++)ledger[first+n]=1;
  last[p.layer]=first+pages-1;
 }
 assert(PioneerQ38TokenStartupPlan::CanonicalLearnedPagesPresent(*hybrid,ledger.data(),ledger.size()));
 for(unsigned layer=0;layer<=64;layer++){
  ledger[last[layer]]=0;
  assert(!PioneerQ38TokenStartupPlan::CanonicalLearnedPagesPresent(*hybrid,ledger.data(),ledger.size()));
  ledger[last[layer]]=1;
 }
 assert(PioneerQ38TokenStartupPlan::CanonicalLearnedPagesPresent(*hybrid,ledger.data(),ledger.size()));

 std::array<uint32_t,1920> source={};for(unsigned i=0;i<source.size();i++)source[i]=0x76540001+i;
 const PioneerQ38GdnIdentity identity={1,2,3,4};const uint32_t prompt[]={248319,17};
 unsigned checkedState=0;
 for(unsigned bank=0;bank<7;bank++){
  auto startup=std::make_unique<PioneerQ38TokenStartupPlan>();
  assert(startup->Build(*hybrid,bank,identity,source.data(),prompt,2,8192,true,248318,248317));
  assert(startup->Matches(*hybrid) && !startup->Matches(*original)
   && startup->Workspace()==Canonical::Base+hybrid->Workspace(bank)->offset);
  for(unsigned worker=1;worker<120;worker++){
   std::array<uint32_t,64> boot={};
   assert(startup->ReadBoot(worker,boot.data()) && PioneerQ38SelectedGlobalWorker::BootShape(boot.data()));
  }
  for(unsigned layer=0;layer<64;layer++){
   uint32_t address=0;assert(startup->PackageAddress(layer,address));
   assert(address==startup->Workspace()+PioneerQ38GlobalChainLayout::Offset
    +layer*PioneerQ38GlobalChainLayout::Stride);
   if(PioneerQ38GlobalChainLayout::Dense(layer))continue;
   std::array<uint8_t,256> serialized={};
   assert(startup->ReadPackage(layer,PioneerQ38TokenStateLayout::Offset,serialized.data(),serialized.size()));
   unsigned n=0;
   for(unsigned i=0;i<hybrid->Count();i++){
    const auto& p=*hybrid->Part(i);if(p.kind!=Canonical::ConvKind || p.layer!=layer)continue;
    assert(n<7 && Word(serialized.data()+(8+n*8)*4)==p.bank
     && Word(serialized.data()+(9+n*8)*4)==Canonical::Base+p.offset
     && Word(serialized.data()+(10+n*8)*4)==p.firstRow
     && Word(serialized.data()+(11+n*8)*4)==p.rows);n++;checkedState++;
   }
   assert(n==7);
  }
 }
 assert(checkedState==7*48*7);
 printf("PASS offline hybrid layout %u parts/%u changed offsets/%u BFP8 stripes/%u learned/%u recurrent; "
  "actual startup all 7 banks and 64 layers/119 worker boot shapes, %u serialized relocated conv spans, "
  "%u ROOT projection job addresses, relocated ledger/missing-page checks, selected hybrid worker %u; "
  "NO device access, upload, worker dispatch, model result or speed\n",
  hybrid->Count(),moved,eight,weights,regions,checkedState,workerJobs,unsigned(PioneerQ38SelectedWorkerHybrid));
}
