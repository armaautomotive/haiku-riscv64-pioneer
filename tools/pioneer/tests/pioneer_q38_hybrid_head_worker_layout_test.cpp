/* SPDX-License-Identifier: MIT */
// QGW8 inspection grammar only; no ROOT, actual instructions or device.
#define PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION 1
#define PIONEER_Q38_HYBRID_WORKER_IMAGE 1
#define PIONEER_Q38_BF16_WORKER_IMAGE 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#include "../PioneerQ38HybridHeadWorkerLayout.h"
#include <array>
#include <cassert>
#include <cstdio>

namespace H=PioneerQ38HybridHeadWorkerLayout;
namespace Old=PioneerQ38HybridWorkerLayout;
using Leaf=PioneerQ38Bfp8WorkerJob;
static std::array<uint32_t,64> Boot(unsigned worker){
 std::array<uint32_t,64>b={};b[0]=H::Magic;b[1]=1;b[2]=64;b[3]=worker;b[4]=H::Peer(worker);
 b[5]=b[16]=0x14000;b[6]=8192;b[7]=64;for(unsigned i=0;i<4;i++)b[8+i*2]=i+1;
 for(unsigned i=40;i<=50;i++)b[i]=H::BootWord(i);assert(H::BootShape(b.data()));return b;
}
static std::array<uint32_t,64> Job(const std::array<uint32_t,64>& b,unsigned bands,unsigned first,unsigned wave){
 std::array<uint32_t,64>j={};j[0]=H::Magic;j[1]=1;j[2]=H::GlobalMatrix;j[3]=b[3];j[4]=b[4];j[5]=64;j[7]=2025;
 for(unsigned i=0;i<8;i++)j[8+i]=b[8+i];
 const unsigned leaf[]={Leaf::Magic,1,5120,0x04000000,bands,PioneerQ38ProjectionRoute(0),b[3],b[4],
  PioneerQ38Bf16Wire::Magic,Leaf::RootInput,Leaf::RootRoute,1,Leaf::Output,0,0,0};
 for(unsigned i=0;i<16;i++)j[16+i]=leaf[i];
 j[32]=first;j[33]=H::Vocabulary;j[34]=wave;j[35]=H::JobWitness;return j;
}
int main(){
 unsigned valid=0,refused=0;
 for(unsigned worker:{1U,21U,22U,119U}){
  auto b=Boot(worker);assert(!Old::BootShape(b.data()));
  for(unsigned bands:{1U,2U,8U})for(unsigned wave:{0U,7U,8U}){
   auto j=Job(b,bands,H::Vocabulary-bands*32,wave);
   const bool expected=wave!=8 || worker<=21;
   assert(H::JobShape(j.data(),b.data(),0,0)==expected);
   assert(!Old::JobShape(j.data(),b.data(),0,0));valid+=expected;refused+=!expected;
  }
 }
 auto b=Boot(1);auto good=Job(b,8,0,0);assert(H::HeadJob(good.data()) && H::FpuJob(good.data()));
 assert(H::HeadShape(good.data(),b.data(),0,0));
 for(unsigned word=0;word<64;word++)for(unsigned bit=0;bit<32;bit++){
  auto j=good;j[word]^=uint32_t(1U)<<bit;
  assert(H::HeadShape(j.data(),b.data(),0,0)==(H::HeadJob(j.data()) && H::JobShape(j.data(),b.data(),0,0)));
 }
 for(unsigned field:{0U,1U,2U,3U,4U,5U,7U,8U,16U,17U,18U,19U,20U,21U,22U,23U,24U,25U,26U,27U,28U,33U,35U}){
  auto j=good;j[field]=0;assert(!H::JobShape(j.data(),b.data(),0,0));refused++;
 }
 for(unsigned field:{29U,30U,31U,36U,63U}){auto j=good;j[field]=1;assert(!H::JobShape(j.data(),b.data(),0,0));refused++;}
 for(unsigned bad:{1U,248320U,248319U}){auto j=good;j[32]=bad;assert(!H::JobShape(j.data(),b.data(),0,0));refused++;}
 {auto j=good;j[34]=9;assert(!H::JobShape(j.data(),b.data(),0,0));refused++;}
 {auto j=good;j[16]=0x31483851;j[17]=14;j[18]=3392;assert(!H::JobShape(j.data(),b.data(),0,0));refused++;}
 for(unsigned layer:{47U,48U,49U,50U,60U,61U}){
  auto j=good;j[2]=1;j[5]=layer;j[32]=0;j[33]=17408;j[34]=2;
  const bool eight=layer>=50 && layer<=60;j[16]=eight?Leaf::Magic:PioneerQ38Bfp4WorkerJob::Magic;j[35]=eight?8:4;
  assert(H::JobShape(j.data(),b.data(),0,0));j[35]=eight?4:8;assert(!H::JobShape(j.data(),b.data(),0,0));valid++;refused++;
 }
 for(unsigned field:{0U,40U,49U,50U}){auto broken=b;broken[field]^=1;assert(!H::BootShape(broken.data()));refused++;}
 printf("PASS QGW8 BFP8 head and FFN layer mask geometry: %u valid/%u refusals, incompatible QGW7 identity; DATA ONLY no device or TPS\n",valid,refused);
}
