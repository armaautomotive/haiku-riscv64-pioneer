/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#define PIONEER_Q38_FIXTURE_ONLY
#include "pioneer_qwen38_gguf_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionJobs.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionInputShape.h"
#include "../PioneerQwen38FleetPlan.h"
#include <memory>
#include <type_traits>
using ProjectionJobs=PioneerQ38ProjectionJobs;
using ProjectionPlan=PioneerQ38CanonicalModelPlan;
static_assert(!std::is_copy_constructible<ProjectionJobs>::value && !std::is_move_constructible<ProjectionJobs>::value,"borrowed immutable heap geometry");
static unsigned matrices=0,parts=0,refusals=0,pageChecks=0,multiWave=0,small=0;
static void Compare(ProjectionPlan& canonical,PioneerQwen38Gguf& model,PioneerQwen38PackedPlan& packed,bool faults){
 auto foreign=std::make_unique<ProjectionPlan>();assert(foreign->Build());
 std::vector<uint8_t> learned(PioneerResidentBanks::Pages,0);
 for(unsigned i=0;i<canonical.Count();i++){
  const auto& p=*canonical.Part(i);if(p.kind!=ProjectionPlan::WeightKind)continue;
  unsigned at=p.bank*unsigned(ProjectionPlan::Capacity/4096)+p.offset/4096;
  std::fill(learned.begin()+at,learned.begin()+at+p.allocated/4096,1);
 }
 const auto originalLedger=learned;
 for(unsigned layer=0;layer<=64;layer++)for(unsigned role=0;;role++){
  PioneerQ38Tensor expected;if(!PioneerQ38Expected(layer,role,expected))break;
  auto jobs=std::make_unique<ProjectionJobs>();unsigned type=999,inputs=998,outputs=997;
  assert(!jobs->Count() && !jobs->Waves() && !jobs->WaveWorkers(0) && !jobs->Part(0)
   && !jobs->Type() && !jobs->Inputs() && !jobs->Outputs() && !jobs->LearnedPagesPresent(nullptr,0));
  const uint64_t token=layer&1?UINT64_MAX:123;
  if(!expected.type){
   assert(!PioneerQ38ProjectionGeometry(layer,role,type,inputs,outputs));
   assert(!canonical.ProjectionGeometry(layer,role,type,inputs,outputs) && type==999 && inputs==998 && outputs==997
    && !canonical.ProjectionStripe(layer,role,0) && !canonical.ProjectionPagesPresent(layer,role,learned.data(),unsigned(learned.size()))
    && !jobs->Build(canonical,layer,role,token) && !jobs->Count() && !jobs->Build(canonical,0,2,token));refusals++;continue;
  }
  const auto* tensor=model.Find(expected.name.c_str());assert(tensor);
  assert(canonical.ProjectionGeometry(layer,role,type,inputs,outputs) && type==expected.type && inputs==expected.inputs && outputs==expected.outputs);
  unsigned deviceType=0,deviceInputs=0,deviceOutputs=0;
  assert(PioneerQ38ProjectionGeometry(layer,role,deviceType,deviceInputs,deviceOutputs)
   && deviceType==type && deviceInputs==inputs && deviceOutputs==outputs);
  assert(jobs->Build(canonical,layer,role,token) && jobs->Matches(canonical,layer,role,token)
   && !jobs->Build(canonical,layer,role,token) && !jobs->Matches(*foreign,layer,role,token)
   && !jobs->Matches(canonical,layer,role,token^1) && !jobs->Matches(canonical,layer,role^1,token)
   && !jobs->Matches(canonical,layer==64?63:layer^1,role,token));
  assert(jobs->Type()==type && jobs->Inputs()==inputs && jobs->Outputs()==outputs && !jobs->Part(jobs->Count())
   && !jobs->Part(UINT32_MAX) && !jobs->WaveWorkers(jobs->Waves()) && !jobs->WaveWorkers(UINT32_MAX));
  PioneerQwen38FleetPlan fleet;assert(fleet.Build(model,packed,unsigned(tensor-model.Tensors().data()),119));
  assert(jobs->Count()==fleet.Jobs().size() && jobs->Waves()==(jobs->Count()+118)/119);
  std::vector<uint8_t> cover(outputs,0);
  for(unsigned i=0;i<jobs->Count();i++){
   const auto& actual=*jobs->Part(i);const auto& host=fleet.Jobs()[i];
   assert(host.worker==i%119 && host.wave==i/119 && actual.bank==host.bank && actual.offset==host.proposedOffset
    && actual.firstRow==host.firstRow && actual.groups==host.groups && actual.groups && actual.groups<=16);
   const auto* stripe=canonical.ProjectionStripe(layer,role,actual.stripe);assert(stripe && stripe->bank==actual.bank
    && actual.firstRow>=stripe->firstRow && actual.groups*16<=stripe->rows-(actual.firstRow-stripe->firstRow)
    && actual.offset==uint64_t(stripe->offset)+uint64_t(actual.firstRow-stripe->firstRow)*stripe->rowBytes);
   for(unsigned row=actual.firstRow;row<actual.firstRow+actual.groups*16;row++)assert(row<outputs && !cover[row]++);
   parts++;
  }
  for(auto covered:cover)assert(covered==1);
  for(unsigned wave=0;wave<jobs->Waves();wave++)assert(jobs->WaveWorkers(wave) && jobs->WaveWorkers(wave)<=119);
  if(outputs==248320){assert(jobs->Waves()==9 && jobs->Count()==973 && jobs->WaveWorkers(8)==21);multiWave++;}
  else assert(jobs->Waves()==1);
  if(outputs==48){assert(jobs->Count()==3 && jobs->WaveWorkers(0)==3);small++;}
  assert(jobs->LearnedPagesPresent(learned.data(),unsigned(learned.size()))
   && !jobs->LearnedPagesPresent(nullptr,unsigned(learned.size()))
   && !jobs->LearnedPagesPresent(learned.data(),unsigned(learned.size())-1));pageChecks++;
  if(faults){
   for(unsigned index:{0U,jobs->Count()/2,jobs->Count()-1})for(unsigned failure=0;failure<5;failure++){
    auto* p=const_cast<PioneerQ38ProjectionPart*>(jobs->Part(index));const auto saved=*p;
    if(failure==0)p->bank=7;if(failure==1)p->groups=0;if(failure==2)p->firstRow+=16;if(failure==3)p->offset+=64;if(failure==4)p->stripe^=1;
    assert(!jobs->Matches(canonical,layer,role,token));*p=saved;assert(jobs->Matches(canonical,layer,role,token));refusals++;
   }
   for(unsigned s=0;;s++){
    const auto* p=canonical.ProjectionStripe(layer,role,s);if(!p)break;
    unsigned at=p->bank*unsigned(ProjectionPlan::Capacity/4096)+p->offset/4096,n=p->allocated/4096;
    for(unsigned relative:{0U,n/2,n-1})for(uint8_t invalid:{uint8_t(0),uint8_t(2),uint8_t(255)}){
     learned[at+relative]=invalid;assert(!jobs->LearnedPagesPresent(learned.data(),unsigned(learned.size())));
     learned[at+relative]=1;refusals++;
    }
   }
  }
  assert(jobs->Matches(canonical,layer,role,token));matrices++;
 }
 assert(learned==originalLedger);
}
static void Reverse(Fixture& f){
 unsigned start=f.dimensionPositions[0]-4-8-unsigned(f.expected[0].name.size());
 std::vector<uint8_t> header(f.header.begin(),f.header.begin()+start);
 for(unsigned reverse=851;reverse;reverse--){unsigned i=reverse-1;
  unsigned first=f.dimensionPositions[i]-4-8-unsigned(f.expected[i].name.size()),last=f.offsetPositions[i]+8;
  header.insert(header.end(),f.header.begin()+first,f.header.begin()+last);
 }
 assert(header.size()==f.header.size());f.Write(0,header);
}
int main(int argc,char** argv){
 assert(argc==1 || argc==2);(void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&BankProof;setvbuf(stdout,nullptr,_IOLBF,0);
 auto canonical=std::make_unique<ProjectionPlan>();auto empty=std::make_unique<ProjectionPlan>();assert(canonical->Build());
 for(unsigned fault=0;fault<5;fault++){
  auto job=std::make_unique<ProjectionJobs>();
  assert(!job->Build(fault?*canonical:*empty,fault==1?65:0,fault==2?14:fault==3?0:fault==4?UINT32_MAX:2,0)
   && !job->Count() && !job->Build(*canonical,0,2,0));refusals++;
 }
 Fixture f;PioneerQwen38Gguf model;PioneerQwen38PackedPlan packed;
 assert(model.Open(f.file) && packed.Build(model,8192,true,true));Compare(*canonical,model,packed,true);
 assert(matrices==498 && multiWave==2 && small==96);puts("PASS all498matrix geometries,973-job/9-wave globals,3-worker recurrent scalars;NO_NATIVE_GRANT");
 Reverse(f);assert(model.Open(f.file) && packed.Build(model,8192,true,true));Compare(*canonical,model,packed,false);
 if(argc==2){FILE* real=fopen(argv[1],"rb");assert(real);PioneerQwen38Gguf actual;PioneerQwen38PackedPlan layout;
  assert(actual.Open(real) && layout.Build(actual,8192,true,true));Compare(*canonical,actual,layout,false);fclose(real);
  puts("PASS ACTUAL local GGUF metadata+independent host schedule;NOT full SHA/source authorization");
 }
 printf("PASS canonical SAME119-peer projection geometry matrices%u/parts%u/refusals%u/whole-tensor-page-checks%u/vocabulary-waves%u/small3worker%u;ALL8/12/14 types,complete cover/no overlap,exact plan/layer/role/token,foreign layout+missing/malformed page rejected,ledgers unchanged;NO_DEVICE_AUTHORITY_OR_SPEED\n",matrices,parts,refusals,pageChecks,multiWave,small);
}
