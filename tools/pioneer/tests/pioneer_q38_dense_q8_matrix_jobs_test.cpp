/* SPDX-License-Identifier: MIT */
// Full fixed-model Q8 matrix job geometry; no learned bytes or accelerator.
#define PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_BANKS_MOCK_ONLY
#include "../PioneerQ38HybridHeadLayout.h"
#include "pioneer_resident_banks_test.cpp"
#include "../PioneerQ38DenseQ8MatrixJobs.h"
#include <cassert>
#include <cstdio>
#include <memory>
using Canonical=PioneerQ38CanonicalModelPlan;using Plan=PioneerQ38HybridDenseQ8MatrixPlan;using W=PioneerQ38Bfp8WorkerJob;
int main(){
 auto source=std::make_unique<Canonical>();assert(source->Build());Plan::Proposal plan;assert(Plan::Build(*source,plan));
 unsigned jobs=0;uint64_t rows=0;
 for(unsigned layer=3;layer<64;layer+=4)for(unsigned role=5;role<=7;role++){
  auto schedule=std::make_unique<PioneerQ38DenseQ8MatrixJobs>();assert(schedule->Build(*source,layer,role,0));
  assert(schedule->Count()==(role==5?119U:32U) && schedule->Rows()==(role==5?12288U:1024U));
  std::vector<unsigned> coverage(schedule->Rows(),0);
  for(unsigned ordinal=0;ordinal<schedule->Count();ordinal++){
   W::Job job={};unsigned first=UINT32_MAX;assert(schedule->Job(*source,layer,role,0,ordinal,1,job,first) && W::Shape(job));
   assert(job.words[6]==ordinal+1 && job.words[4]>=1 && job.words[4]<=8 && !(first&31));
   unsigned found=0;
   for(unsigned i=0;i<source->Count();i++){
    const auto* p=source->Part(i);if(!p || p->kind!=Canonical::WeightKind || p->layer!=layer || p->role!=role
     || first<p->firstRow || first>=p->firstRow+p->rows)continue;
    const auto& stripe=plan.parts[i];const uint64_t expected=uint64_t(Canonical::Base)+stripe.offset+uint64_t((first-p->firstRow)/32)*174080;
    assert(stripe.encoding==Plan::DenseQ8Bfp8 && job.words[3]==expected && job.words[5]==PioneerQ38ProjectionRoute(p->bank)
     && first+job.words[4]*32<=p->firstRow+p->rows);found++;
   }
   assert(found==1);
   for(unsigned row=first;row<first+job.words[4]*32;row++){assert(row<coverage.size() && !coverage[row]);coverage[row]=1;rows++;}
   jobs++;
  }
  for(unsigned c:coverage)assert(c==1);
  W::Job unchanged={};for(auto& word:unchanged.words)word=0xa5a5a5a5;const auto saved=unchanged;unsigned first=0x12345678;
  assert(!schedule->Job(*source,layer,role,0,schedule->Count(),1,unchanged,first));
  assert(!schedule->Job(*source,layer,role,0,0,0,unchanged,first));
  assert(!schedule->Job(*source,layer,role,0,0,0x7fffffff,unchanged,first));
  assert(!schedule->Job(*source,layer,role,1,0,1,unchanged,first));
  assert(!schedule->Job(*source,layer,role,0,0,1,unchanged,unchanged.words[0]));
  for(unsigned i=0;i<W::Words;i++)assert(unchanged.words[i]==saved.words[i]);assert(first==0x12345678);
 }
 assert(jobs==16*(119+32+32) && rows==16*(12288+1024+1024));
 PioneerQ38DenseQ8MatrixJobs invalid;assert(!invalid.Build(*source,2,5,0) && !invalid.Count());
 assert(!invalid.Build(*source,3,5,0));
 PioneerQ38DenseQ8MatrixJobs outOfContext;assert(!outOfContext.Build(*source,3,5,Canonical::Context));
 PioneerQ38DenseQ8MatrixJobs last;assert(last.Build(*source,63,7,Canonical::Context-1));
 W::Job tail={};unsigned tailRow=0;assert(last.Job(*source,63,7,Canonical::Context-1,31,1,tail,tailRow));
 auto other=std::make_unique<Canonical>();assert(other->Build());const auto savedTail=tail;const auto savedRow=tailRow;
 assert(!last.Job(*other,63,7,Canonical::Context-1,31,1,tail,tailRow) && tailRow==savedRow);
 for(unsigned i=0;i<W::Words;i++)assert(tail.words[i]==savedTail.words[i]);
 std::printf("PASS dense Q8 BFP8 jobs=%u rows=%llu, exact stripe/address/coverage/alias checks; NO DEVICE OR TPS\n",jobs,(unsigned long long)rows);
}
