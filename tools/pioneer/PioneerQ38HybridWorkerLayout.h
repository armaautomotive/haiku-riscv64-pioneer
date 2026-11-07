/* SPDX-License-Identifier: MIT */
#include "PioneerQ38HybridKernelCompileOnly.h"
#ifndef PIONEER_Q38_HYBRID_WORKER_LAYOUT_H
#define PIONEER_Q38_HYBRID_WORKER_LAYOUT_H
#if (defined(_KERNEL_MODE) && !defined(PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY) && !defined(PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY) && !defined(PIONEER_Q38_HYBRID_FULL_NATIVE)) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || (defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) && !defined(PIONEER_Q38_HYBRID_FULL_NATIVE)) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Experimental hybrid worker grammar cannot be used in Work, validation, or native driver roles
#endif
#if defined(PIONEER_Q38_HYBRID_TEST_BFP8_LAYER0) && PIONEER_Q38_HYBRID_TEST_BFP8_LAYER0 != 1
#error Test-only layer-zero mask must be explicitly one
#endif
#if defined(PIONEER_Q38_HYBRID_INPUT_REUSE_INSPECTION) && PIONEER_Q38_HYBRID_INPUT_REUSE_INSPECTION != 1
#error Input-reuse inspection must be explicitly selected
#endif

#include "PioneerQ38Bf16WorkerLayout.h"
#include "PioneerQ38Bfp4WorkerJob.h"
#include "PioneerQ38Bfp8WorkerJob.h"

// Experimental QGW5 frames. Shape alone does not establish ROOT ownership,
// uploaded weights, canonical addresses, or input lifetime.
namespace PioneerQ38HybridWorkerLayout {
namespace Base=PioneerQ38Bf16WorkerLayout;
namespace Producer=Base::Producer;
using namespace PioneerQ38GlobalWorkerLayout;
#ifdef PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP
#define QGW5_GRAMMAR __attribute__((noinline,section(".bf16.root")))
#else
#define QGW5_GRAMMAR __attribute__((noinline,unused))
#endif
enum : unsigned {
#ifdef PIONEER_Q38_HYBRID_INPUT_REUSE_INSPECTION
 // Distinct inspection-only grammar: an old worker or ROOT must not accept
 // the new completion protocol even when a one-band job happens to match.
 Magic=0x36574751, JobWitness=0x364f4751,
#else
 Magic=0x35574751, JobWitness=0x354f4751,
#endif
#ifdef PIONEER_Q38_HYBRID_TEST_BFP8_LAYER0
 // Test-image-only: exercise the BFP8 dispatcher on the first layer without
 // fabricating a 48-layer worker history. Never a production configuration.
 Bfp8MaskLow=1, Bfp8MaskHigh=0};
#else
 Bfp8MaskLow=0, Bfp8MaskHigh=0x1fff0000}; // layers 48 through 60
#endif

static inline unsigned BootWord(unsigned i){
 return i==40?unsigned(JobWitness):i==49?unsigned(Bfp8MaskLow):
  i==50?unsigned(Bfp8MaskHigh):Base::BootWord(i);
}
static QGW5_GRAMMAR bool BootShape(const uint32_t* b){
 if(!b || b[0]!=Magic || b[40]!=JobWitness || b[49]!=Bfp8MaskLow || b[50]!=Bfp8MaskHigh)return false;
 uint32_t old[64];for(unsigned i=0;i<64;i++)old[i]=b[i];
 old[0]=Base::Magic;old[40]=Base::JobWitness;old[49]=old[50]=0;
 return Base::BootShape(old);
}
static inline bool FpuJob(const uint32_t* j){
 return j && j[2]==1 && (j[16]==PioneerQ38Bfp4WorkerJob::Magic || j[16]==PioneerQ38Bfp8WorkerJob::Magic);
}
template<class W> static bool LeafShape(const uint32_t* j,const uint32_t* b){
 typename W::Job leaf;for(unsigned i=0;i<16;i++)leaf.words[i]=j[16+i];
 return W::Shape(leaf) && leaf.words[6]==b[3] && leaf.words[7]==b[4] && leaf.words[11]==j[1]
  && j[33]==(leaf.words[2]==5120?17408U:5120U) && !(j[32]&31) && j[32]<j[33]
  && leaf.words[4]*32<=j[33]-j[32] && (leaf.words[2]==17408?j[34]==4:(j[34]==2 || j[34]==3));
}
static QGW5_GRAMMAR bool JobShape(const uint32_t* j,const uint32_t* b,unsigned previous,unsigned densePhase){
 if(!j || !BootShape(b) || j[0]!=Magic)return false;
 if(FpuJob(j)){
  if(densePhase || previous>=0x7ffffffe || j[1]!=previous+1 || !j[1] || j[1]>=0x7fffffff
   || j[3]!=b[3] || j[4]!=b[4] || j[5]>=64 || j[6]>=8192 || !j[7] || j[7]>=0x7fffffff)return false;
  for(unsigned i=8;i<16;i++)if(j[i]!=b[i])return false;
  const bool eight=((j[5]<32?b[49]:b[50])>>(j[5]&31))&1;
  if(eight ? (j[35]!=8 || j[16]!=PioneerQ38Bfp8WorkerJob::Magic || !LeafShape<PioneerQ38Bfp8WorkerJob>(j,b))
           : (j[35]!=4 || j[16]!=PioneerQ38Bfp4WorkerJob::Magic || !LeafShape<PioneerQ38Bfp4WorkerJob>(j,b)))return false;
  for(unsigned i=36;i<64;i++)if(j[i])return false;
  return true;
 }
 // Reject old FPU and integer Q4 capsules; format is not interchangeable.
 if(j[2]==1 && (j[16]==PioneerQ38Q4FpuCachedLayout::Magic || j[17]==12))return false;
 uint32_t oldBoot[64],oldJob[64];for(unsigned i=0;i<64;i++){oldBoot[i]=b[i];oldJob[i]=j[i];}
 oldBoot[0]=oldJob[0]=Base::Magic;oldBoot[40]=Base::JobWitness;oldBoot[49]=oldBoot[50]=0;
 if(j[2]==GlobalMatrix){if(j[31]!=JobWitness)return false;oldJob[31]=Base::JobWitness;}
 return Base::JobShape(oldJob,oldBoot,previous,densePhase);
}
#undef QGW5_GRAMMAR
}
#endif
