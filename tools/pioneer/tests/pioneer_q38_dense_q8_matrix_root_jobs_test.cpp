/* SPDX-License-Identifier: MIT */
// Manifest/controller job bridge; synthetic host IO, no P100A.
#define PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_MATRIX_ROOT_INSPECTION 1
#define PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_BANKS_MOCK_ONLY
#include "../PioneerQ38HybridHeadLayout.h"
#include "pioneer_resident_banks_test.cpp"
#include "../PioneerQ38DenseQ8MatrixManifest.h"
#include "../PioneerQ38DenseQ8MatrixJobs.h"
#include "../PioneerQ38DenseQ8MatrixRootJobs.h"
#include <cassert>
#include <cstdio>
#include <memory>
using C=PioneerQ38CanonicalModelPlan;using P=PioneerQ38HybridDenseQ8MatrixPlan;
using M=PioneerQ38DenseQ8MatrixManifest;using W=PioneerQ38Bfp8WorkerJob;
struct IO {
 uint32_t table[7][3]={},out[119][4]={};unsigned writes=0,reads=0,alterAt=0;
 unsigned Table(unsigned bank,unsigned field){assert(bank<7 && field<3);reads++;return table[bank][field]^(reads==alterAt?4096U:0U);}
 void Output(unsigned worker,unsigned field,unsigned value){assert(worker<119 && field<4);out[worker][field]=value;writes++;}
};
int main(){
 auto source=std::make_unique<C>();assert(source->Build());P::Proposal proposal;assert(P::Build(*source,proposal));
 std::array<uint32_t,M::Words> manifest={};assert(M::Build(*source,proposal,manifest.data(),manifest.size()));
 auto modified=proposal;modified.parts[0].offset+=4096;auto snapshot=manifest;
 assert(!M::Build(*source,modified,manifest.data(),manifest.size()) && manifest==snapshot);
 assert(!M::Build(*source,proposal,manifest.data(),manifest.size()-1) && manifest==snapshot);
 assert(!M::Build(*source,proposal,reinterpret_cast<uint32_t*>(proposal.parts.data()),M::Words));
 unsigned covered=0;
 for(unsigned layer=3;layer<64;layer+=4)for(unsigned role=5;role<=7;role++){
  IO io;for(unsigned b=0;b<7;b++)for(unsigned f=0;f<3;f++)io.table[b][f]=manifest[M::Index(layer/4,role-5,b)+f];
  PioneerQ38DenseQ8MatrixJobs host;assert(host.Build(*source,layer,role,0));
  assert(PioneerQ38DenseQ8MatrixRootJobs::Build(io,layer,role) && io.writes==host.Count()*4);
  for(unsigned w=0;w<host.Count();w++){
   W::Job job={};unsigned first=UINT32_MAX;assert(host.Job(*source,layer,role,0,w,1,job,first));
   assert(io.out[w][0]==job.words[3] && PioneerQ38ProjectionRoute(io.out[w][1])==job.words[5]
    && io.out[w][2]==first && io.out[w][3]==job.words[4]);covered++;
  }
 }
 assert(covered==2928);
 IO base;for(unsigned b=0;b<7;b++)for(unsigned f=0;f<3;f++)base.table[b][f]=manifest[M::Index(0,2,b)+f];
 for(unsigned bad=0;bad<6;bad++){
  IO mutated=base;
  if(bad==0)mutated.table[0][0]++;
  if(bad==1)mutated.table[0][1]++;
  if(bad==2)mutated.table[0][2]=0;
  if(bad==3)mutated.table[0][1]=mutated.table[1][1];
  if(bad==4)mutated.table[0][0]=0xc4000000;
  if(bad==5)mutated.alterAt=22;
  assert(!PioneerQ38DenseQ8MatrixRootJobs::Build(mutated,3,7) && !mutated.writes);
 }
 assert(!PioneerQ38DenseQ8MatrixRootJobs::Build(base,2,7) && !base.writes);
 std::printf("PASS manifest and controller matched %u dense Q8 jobs; malformed/unstable data refused before publication; NO DEVICE\n",covered);
}
