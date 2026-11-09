/* SPDX-License-Identifier: MIT */
// Distinct dense Q8 worker grammar, synthetic geometry only.
#define PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION 1
#define PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_BANKS_MOCK_ONLY
#include "../PioneerQ38HybridHeadLayout.h"
#include "pioneer_resident_banks_test.cpp"
#include "../PioneerQ38DenseQ8MatrixJobs.h"
#include "../PioneerQ38DenseQ8MatrixWorkerLayout.h"
#include "../PioneerQ38HybridMatrixReceipt.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <memory>
namespace M=PioneerQ38DenseQ8MatrixWorkerLayout;namespace H=PioneerQ38HybridHeadWorkerLayout;
using C=PioneerQ38CanonicalModelPlan;using W=PioneerQ38Bfp8WorkerJob;
static std::array<uint32_t,64> Boot(unsigned worker){
 std::array<uint32_t,64>b={};b[0]=M::Magic;b[1]=1;b[2]=64;b[3]=worker;b[4]=M::Peer(worker);
 b[5]=b[16]=0x14000;b[6]=8192;b[7]=64;
 for(unsigned i=0;i<4;i++)b[8+i*2]=i+1;for(unsigned i=40;i<=50;i++)b[i]=M::BootWord(i);
 return b;
}
int main(){
 auto source=std::make_unique<C>();assert(source->Build());unsigned accepted=0;
 for(unsigned layer=3;layer<64;layer+=4)for(unsigned role=5;role<=7;role++){
  PioneerQ38DenseQ8MatrixJobs plan;assert(plan.Build(*source,layer,role,0));
  for(unsigned ordinal=0;ordinal<plan.Count();ordinal++){
   W::Job leaf={};unsigned first=0;assert(plan.Job(*source,layer,role,0,ordinal,1,leaf,first));
   auto boot=Boot(ordinal+1);assert(M::BootShape(boot.data()) && !H::BootShape(boot.data()));
   std::array<uint32_t,64> job={};job[0]=M::Magic;job[1]=1;job[2]=1;job[3]=boot[3];job[4]=boot[4];
   job[5]=layer;job[6]=0;job[7]=3;for(unsigned i=8;i<16;i++)job[i]=boot[i];
   for(unsigned i=0;i<16;i++)job[16+i]=leaf.words[i];job[32]=first;job[33]=plan.Rows();job[34]=role;job[35]=8;
   assert(M::DenseJob(job.data()) && M::FpuJob(job.data()) && M::JobShape(job.data(),boot.data(),0,0));
   assert(!H::JobShape(job.data(),boot.data(),0,0));
   if(accepted==0){
    std::array<uint32_t,64> receipt={};std::array<uint32_t,8> before={},after={};
    const unsigned bands=job[20],tiles=job[18]/32;
    for(unsigned i=0;i<8;i++)after[i]=i==1?bands*tiles*18:(i==0 || i==2 || i==3 || i==6?bands*tiles*2:0);
    for(unsigned i=0;i<8;i++)receipt[8+i]=after[i];
    receipt[16]=bands*32;receipt[17]=job[1];receipt[18]=boot[3];receipt[19]=job[16];
    W::Read last;assert(W::Packet(leaf,bands-1,1,tiles-1,last));
    receipt[20]=last.source;receipt[22]=last.route;receipt[23]=last.destination;receipt[25]=boot[4];receipt[28]=last.bytes;
    receipt[30]=boot[16];receipt[31]=boot[4];for(unsigned i=0;i<16;i++)receipt[32+i]=boot[16+i];
    receipt[48]=M::Producer::Format;receipt[49]=job[18];receipt[50]=bands;
    for(unsigned i=51;i<=56;i++)receipt[i]=job[i==51?7:i==52?5:i==53?6:i-22];
    assert(PioneerQ38HybridMatrixReceipt::Matrix(receipt.data(),job.data(),boot.data(),before.data(),after.data()));
    receipt[54]^=32;assert(!PioneerQ38HybridMatrixReceipt::Matrix(receipt.data(),job.data(),boot.data(),before.data(),after.data()));
    auto oldBoot=boot;oldBoot[0]=H::Magic;oldBoot[40]=H::JobWitness;
    auto head=job;head[2]=M::GlobalMatrix;head[5]=64;head[32]=0;head[33]=M::Vocabulary;head[34]=0;head[35]=M::JobWitness;
    auto oldHead=head;oldHead[0]=H::Magic;oldHead[35]=H::JobWitness;
    assert(H::JobShape(oldHead.data(),oldBoot.data(),0,0) && M::JobShape(head.data(),boot.data(),0,0));
    head[35]=H::JobWitness;assert(!M::JobShape(head.data(),boot.data(),0,0));
    auto ffn=job;ffn[5]=50;ffn[32]=0;ffn[33]=17408;ffn[34]=2;
    auto oldFfn=ffn;oldFfn[0]=H::Magic;
    assert(H::JobShape(oldFfn.data(),oldBoot.data(),0,0) && M::JobShape(ffn.data(),boot.data(),0,0));
    auto stop=job;stop[2]=M::Stop;stop[5]=M::GlobalLayer;for(unsigned i=16;i<64;i++)stop[i]=0;
    auto oldStop=stop;oldStop[0]=H::Magic;
    assert(H::JobShape(oldStop.data(),oldBoot.data(),0,0) && M::JobShape(stop.data(),boot.data(),0,0));
   }
   for(unsigned field:{0U,1U,3U,4U,5U,6U,7U,8U,16U,17U,18U,22U,23U,24U,25U,26U,27U,28U,29U,30U,31U,32U,33U,34U,35U,36U}){
    auto bad=job;
    if(field==5)bad[field]=2;else if(field==6)bad[field]=8192;else if(field==34)bad[field]=8;
    else if(field==32)bad[field]=plan.Rows();else if(field==33)bad[field]=0;else bad[field]^=0x80000000U;
    assert(!M::JobShape(bad.data(),boot.data(),0,0));
   }
   assert(!M::JobShape(job.data(),boot.data(),1,0) && !M::JobShape(job.data(),boot.data(),0,1));accepted++;
  }
 }
 assert(accepted==2928);puts("PASS distinct QGWB dense Q8 matrix worker grammar and 2928 jobs; NO DEVICE OR TPS");
}
