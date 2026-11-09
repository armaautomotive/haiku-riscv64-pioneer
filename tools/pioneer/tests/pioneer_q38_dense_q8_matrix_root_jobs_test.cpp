/* SPDX-License-Identifier: MIT */
// Manifest/controller job bridge; synthetic host IO, no P100A.
#define PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_MATRIX_ROOT_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION 1
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
#include "../PioneerQ38DenseQ8MatrixRootFrame.h"
#include <cassert>
#include <cstdio>
#include <memory>
using C=PioneerQ38CanonicalModelPlan;using P=PioneerQ38HybridDenseQ8MatrixPlan;
using M=PioneerQ38DenseQ8MatrixManifest;using W=PioneerQ38Bfp8WorkerJob;
namespace G=PioneerQ38DenseQ8MatrixWorkerLayout;
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
 assert(PioneerQ38DenseQ8MatrixRootJobs::Active(5)==119);
 assert(PioneerQ38DenseQ8MatrixRootJobs::Active(6)==32 && PioneerQ38DenseQ8MatrixRootJobs::Active(7)==32);
 assert(PioneerQ38DenseQ8MatrixRootJobs::Active(4)==0 && PioneerQ38DenseQ8MatrixRootJobs::Active(8)==0);
 for(unsigned layer=3;layer<64;layer+=4)for(unsigned role=5;role<=7;role++){
  IO io;for(unsigned b=0;b<7;b++)for(unsigned f=0;f<3;f++)io.table[b][f]=manifest[M::Index(layer/4,role-5,b)+f];
  PioneerQ38DenseQ8MatrixJobs host;assert(host.Build(*source,layer,role,0));
  assert(PioneerQ38DenseQ8MatrixRootJobs::Build(io,layer,role) && io.writes==host.Count()*4
   && host.Count()==PioneerQ38DenseQ8MatrixRootJobs::Active(role));
  for(unsigned w=0;w<host.Count();w++){
   W::Job job={};unsigned first=UINT32_MAX;assert(host.Job(*source,layer,role,0,w,1,job,first));
   assert(io.out[w][0]==job.words[3] && PioneerQ38ProjectionRoute(io.out[w][1])==job.words[5]
    && io.out[w][2]==first && io.out[w][3]==job.words[4]);covered++;
   std::array<uint32_t,64> boot={},frame={};boot[0]=G::Magic;boot[1]=1;boot[2]=64;boot[3]=w+1;boot[4]=G::Peer(w+1);
   boot[5]=boot[16]=0x14000;boot[6]=8192;boot[7]=64;
   for(unsigned i=0;i<4;i++)boot[8+i*2]=i+1;for(unsigned i=40;i<=50;i++)boot[i]=G::BootWord(i);
   assert(PioneerQ38DenseQ8MatrixRootFrame::Build(io.out[w],boot.data(),layer,role,0,3,0,frame.data()));
   for(unsigned i=0;i<16;i++)assert(frame[16+i]==job.words[i]);
   assert(frame[32]==first && frame[33]==host.Rows() && frame[34]==role && G::JobShape(frame.data(),boot.data(),0,0));
   auto untouched=frame;auto bad=std::array<uint32_t,4>{io.out[w][0],7,io.out[w][2],io.out[w][3]};
   assert(!PioneerQ38DenseQ8MatrixRootFrame::Build(bad.data(),boot.data(),layer,role,0,3,0,frame.data()) && frame==untouched);
   assert(!PioneerQ38DenseQ8MatrixRootFrame::Build(io.out[w],boot.data(),layer,role,0,3,0x7ffffffe,frame.data()) && frame==untouched);
   auto bootSaved=boot;
   assert(!PioneerQ38DenseQ8MatrixRootFrame::Build(io.out[w],boot.data(),layer,role,0,3,0,boot.data()) && boot==bootSaved);
   assert(!PioneerQ38DenseQ8MatrixRootFrame::Build(io.out[w],boot.data(),layer,role,8192,3,0,frame.data()) && frame==untouched);
   assert(!PioneerQ38DenseQ8MatrixRootFrame::Build(io.out[w],boot.data(),layer,role,0,0,0,frame.data()) && frame==untouched);
   assert(!PioneerQ38DenseQ8MatrixRootFrame::Build(io.out[w],boot.data(),layer,role,0,3,0,
    reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(frame.data())+1)) && frame==untouched);
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
