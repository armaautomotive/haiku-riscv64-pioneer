/* SPDX-License-Identifier: MIT */
#include "PioneerQ38Qgw8Bfp8ReuseRole.h"
#include "PioneerQ38Qgw8Bfp8BatchRole.h"
#ifndef PIONEER_Q38_HYBRID_MATRIX_RECEIPT_H
#define PIONEER_Q38_HYBRID_MATRIX_RECEIPT_H
#include "PioneerQ38HybridWorkerLayout.h"
#if defined(PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION) || defined(PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION)
#include "PioneerQ38HybridHeadWorkerLayout.h"
#endif
#ifdef PIONEER_Q38_HYBRID_INPUT_REUSE_INSPECTION
#include "PioneerQ38HybridInputReuseProfile.h"
#endif

// Experimental ROOT-side interpretation of the actual QGW5 worker proof.
// The caller must independently authenticate the closed job, physical
// counters, input lifetime, converted weights, and exclusive ownership.
struct PioneerQ38HybridMatrixReceipt {
#ifdef PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP
 static __attribute__((noinline,section(".bf16.receipt")))
#else
 static __attribute__((noinline)) bool Matrix(const uint32_t* p,const uint32_t* j,const uint32_t* b,
#endif
#ifdef PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP
 bool Matrix(const uint32_t* p,const uint32_t* j,const uint32_t* b,
#endif
  const uint32_t* before,const uint32_t* after){
#if defined(PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION) || defined(PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION)
  namespace M=PioneerQ38HybridHeadWorkerLayout;
#else
  namespace M=PioneerQ38HybridWorkerLayout;
#endif
  if(!p || !j || !b || !before || !after || !j[1] || !M::FpuJob(j)
   || !M::JobShape(j,b,j[1]-1,0) || before[2]!=before[6] || after[2]!=after[6])return false;
  const bool eight=j[16]==PioneerQ38Bfp8WorkerJob::Magic;
  const unsigned width=j[18],bands=j[20],tiles=width/32;
#ifdef PIONEER_Q38_HYBRID_INPUT_REUSE_INSPECTION
  unsigned reads=0,flits=0;bool lastWeight=false;
  if(!PioneerQ38HybridInputReuseProfile::Describe(width,bands,eight,reads,flits,lastWeight))return false;
#endif
  if(p[16]!=bands*32 || p[17]!=j[1] || p[18]!=b[3] || p[19]!=j[16]
   || p[30]!=b[16] || p[31]!=b[4] || p[48]!=M::Producer::Format || p[49]!=width
   || p[50]!=bands || p[51]!=j[7] || p[52]!=j[5] || p[53]!=j[6]
   || p[54]!=j[32] || p[55]!=j[33] || p[56]!=j[34])return false;
  for(unsigned i=0;i<16;i++)if(p[32+i]!=b[16+i])return false;
  for(unsigned i=57;i<64;i++)if(p[i])return false;
  // Original QGW5 finishes on input; the separate BFP4 reuse experiment
  // finishes on the final weight when there is more than one band.
  // Derive its complete NIU command from the selected validated leaf.
  unsigned source=0,destination=0,route=0,bytes=0;
  if(eight){
#ifdef PIONEER_Q38_QGW8_BFP8_BATCH_WORKER
   const unsigned offset=((width*2-1)/16384)*16384;
   source=PioneerQ38Bfp8WorkerJob::RootInput+offset;
   destination=PioneerQ38Bfp8WorkerLayout::Input+offset;
   route=PioneerQ38Bfp8WorkerJob::RootRoute;bytes=width*2-offset;
#else
   PioneerQ38Bfp8WorkerJob::Job leaf;for(unsigned i=0;i<16;i++)leaf.words[i]=j[16+i];
   PioneerQ38Bfp8WorkerJob::Read last;
   if(!PioneerQ38Bfp8WorkerJob::Packet(leaf,bands-1,
#ifdef PIONEER_Q38_QGW8_BFP8_REUSE_WORKER
    bands>1?0U:1U,
#else
    1U,
#endif
    tiles-1,last))return false;
   source=last.source;destination=last.destination;route=last.route;bytes=last.bytes;
#endif
  }else{
   PioneerQ38Bfp4WorkerJob::Job leaf;for(unsigned i=0;i<16;i++)leaf.words[i]=j[16+i];
   PioneerQ38Bfp4WorkerJob::Read last;
   if(!PioneerQ38Bfp4WorkerJob::Packet(leaf,bands-1,
#ifdef PIONEER_Q38_HYBRID_INPUT_REUSE_INSPECTION
    lastWeight?0U:1U,
#else
    1U,
#endif
    tiles-1,last))return false;
   source=last.source;destination=last.destination;route=last.route;bytes=last.bytes;
  }
  for(unsigned i=0;i<10;i++){
   const unsigned expected=i==0?source:i==2?route:i==3?destination:i==5?b[4]:i==8?bytes:0U;
   if(p[20+i]!=expected)return false;
  }
  for(unsigned i=0;i<8;i++){
   const unsigned delta=i==1?
#ifdef PIONEER_Q38_HYBRID_INPUT_REUSE_INSPECTION
    flits:i==0 || i==2 || i==3 || i==6?reads:0U;
#elif defined(PIONEER_Q38_QGW8_BFP8_REUSE_WORKER)
    (eight?(bands*17+1)*tiles:bands*tiles*10):i==0 || i==2 || i==3 || i==6?
    (eight?(bands+1)*tiles:bands*tiles*2):0U;
#elif defined(PIONEER_Q38_QGW8_BFP8_BATCH_WORKER)
    bands*tiles*(eight?18U:10U):i==0 || i==2 || i==3 || i==6?
    bands*(eight?tiles+(width*2+16383)/16384:tiles*2):0U;
#else
    bands*tiles*(eight?18U:10U):i==0 || i==2 || i==3 || i==6?bands*tiles*2:0U;
#endif
   if(p[i]!=before[i] || p[8+i]!=after[i] || uint32_t(after[i]-before[i])!=delta)return false;
  }
  return true;
 }
};
#endif
