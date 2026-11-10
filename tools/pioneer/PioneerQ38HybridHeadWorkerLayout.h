/* SPDX-License-Identifier: MIT */
#include "PioneerQ38Qgw8NativeInspectionRole.h"
#include "PioneerQ38Qgw8Bfp8ReuseRole.h"
#include "PioneerQ38Qgw8Bfp8BatchRole.h"
#include "PioneerQ38QgwaBatchRootReceiptRole.h"
#include "PioneerQ38QgwaBatchRootImageRole.h"
#include "PioneerQ38QgwcSrcaRootImageRole.h"
#include "PioneerQ38QgwdRootImageRole.h"
#include "PioneerQ38QgwdKernelCompileRole.h"
#ifdef PIONEER_Q38_QGWD_PLAN_SIMULATION
#if PIONEER_Q38_QGWD_PLAN_SIMULATION != 1 || !defined(PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION != 1 || defined(PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION) || defined(PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION) || defined(PIONEER_Q38_QGWC_PLAN_SIMULATION) || defined(PIONEER_Q38_QGWD_ROOT_IMAGE) || (defined(_KERNEL_MODE) && !defined(PIONEER_Q38_QGWD_KERNEL_COMPILE_ONLY) && !defined(PIONEER_Q38_QGWD_NATIVE_INSPECTION)) || (defined(PIONEER_DRIVER_BUILD_TAG) && !defined(PIONEER_Q38_QGWD_NATIVE_INSPECTION)) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE)
#error QGWD plan requires isolated simulation only
#endif
#endif
#ifdef PIONEER_Q38_QGWD_SRCA_DENSE_Q8_WORKER
#if PIONEER_Q38_QGWD_SRCA_DENSE_Q8_WORKER != 1 || !defined(PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION) || PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION != 1 || !defined(PIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE) || PIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE != 1 || !defined(PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION) || PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION != 1 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE)
#error QGWD requires its isolated SrcA-corrected dense-Q8 worker role
#endif
#endif
#ifdef PIONEER_Q38_QGWC_PLAN_SIMULATION
#if PIONEER_Q38_QGWC_PLAN_SIMULATION != 1 || !defined(PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION != 1 || defined(PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION) || defined(PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION) || defined(PIONEER_Q38_QGWC_SRCA_ROOT_IMAGE) || defined(PIONEER_Q38_QGWA_BATCH_ROOT_RECEIPT_INSPECTION) || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE)
#error QGWC plan is isolated simulation only
#endif
#endif
#if defined(PIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE) && (PIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE != 1 \
 || !defined(PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION) || PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION != 1 \
 || (defined(PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION) && !defined(PIONEER_Q38_QGWD_SRCA_DENSE_Q8_WORKER)) || defined(PIONEER_Q38_QGW8_BFP8_BATCH_WORKER) \
 || defined(PIONEER_Q38_QGW8_BFP8_REUSE_WORKER) || defined(PIONEER_Q38_QGWA_BATCH_ROOT_IMAGE) \
 || defined(PIONEER_Q38_QGWA_BATCH_ROOT_RECEIPT_INSPECTION) || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE))
#error QGWC SrcA base candidate is isolated worker inspection only
#endif
#ifndef PIONEER_Q38_HYBRID_HEAD_WORKER_LAYOUT_H
#define PIONEER_Q38_HYBRID_HEAD_WORKER_LAYOUT_H
#if (!defined(PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION) && !defined(PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION) && !defined(PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION)) \
 || (defined(PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION) && (PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION != 1 \
  || !defined(PIONEER_Q38_HYBRID_WORKER_IMAGE) || PIONEER_Q38_HYBRID_WORKER_IMAGE != 1 \
  || !defined(PIONEER_Q38_BF16_WORKER_IMAGE) || PIONEER_Q38_BF16_WORKER_IMAGE != 1 \
  || defined(PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION) || defined(PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION))) \
 || (defined(PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION) && (PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION != 1 \
  || defined(PIONEER_Q38_HYBRID_WORKER_IMAGE) || defined(PIONEER_Q38_BF16_WORKER_IMAGE) || defined(PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION))) \
 || (defined(PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION) && (PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION != 1 \
  || !defined(PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP) || PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP != 1 \
  || !defined(PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU) || PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU != 1 \
  || defined(PIONEER_Q38_HYBRID_WORKER_IMAGE) || defined(PIONEER_Q38_BF16_WORKER_IMAGE))) \
 || !defined(PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION) || PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION != 1 \
 || !defined(PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT) || PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT != 1 \
 || defined(PIONEER_Q38_HYBRID_TEST_BFP8_LAYER0) || defined(PIONEER_Q38_HYBRID_INPUT_REUSE_INSPECTION) \
 || ((defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY)) \
  && !defined(PIONEER_Q38_QGW8_NATIVE_INSPECTION) && !defined(PIONEER_Q38_QGWD_KERNEL_COMPILE_ONLY)) || (defined(PIONEER_Q38_HYBRID_FULL_NATIVE) && !defined(PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128)) \
 || defined(PIONEER_Q38_HYBRID_MATRIX_NATIVE_127) \
 || (defined(PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY) && !defined(PIONEER_Q38_QGWD_KERNEL_COMPILE_ONLY)) || defined(__FAST_MATH__) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error QGW8 output-head matrix grammar is isolated worker inspection only
#endif
#include "PioneerQ38HybridWorkerLayout.h"

// QGW8 inspection frame: all head jobs use the BFP8 leaf; old Q6 head
// frames are rejected. FFN mask uses BFP8 only in layers 50..60. This is
// geometry and format only, never source authentication or device authority.
namespace PioneerQ38HybridHeadWorkerLayout {
namespace Base=PioneerQ38HybridWorkerLayout;
using namespace PioneerQ38HybridWorkerLayout;
enum : unsigned {
#if defined(PIONEER_Q38_QGWD_ROOT_IMAGE) || defined(PIONEER_Q38_QGWD_PLAN_SIMULATION)
 Magic=0x44574751,JobWitness=0x444f4751, // QGWD / QGOD; not a dense-Q8 scheduler
#elif defined(PIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE) || defined(PIONEER_Q38_QGWC_SRCA_ROOT_IMAGE) || defined(PIONEER_Q38_QGWC_PLAN_SIMULATION) || defined(PIONEER_Q38_QGWC_NATIVE_136)
 Magic=0x43574751,JobWitness=0x434f4751, // QGWC / QGOC, inspection only
#elif defined(PIONEER_Q38_QGW8_BFP8_BATCH_WORKER) || defined(PIONEER_Q38_QGWA_BATCH_ROOT_RECEIPT_INSPECTION) || defined(PIONEER_Q38_QGWA_BATCH_ROOT_IMAGE)
 Magic=0x41475751,JobWitness=0x414f4751,
#elif defined(PIONEER_Q38_QGW8_BFP8_REUSE_WORKER)
 Magic=0x39574751,JobWitness=0x394f4751,
#else
 Magic=0x38574751,JobWitness=0x384f4751,
#endif
 Bfp8MaskLow=0,Bfp8MaskHigh=0x1ffc0000};
#ifdef PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION
#define QGW8_HEAD_FN static __attribute__((noinline,unused,section(".bf16.head")))
#else
#define QGW8_HEAD_FN static inline
#endif
static inline unsigned BootWord(unsigned i){
 return i==40?unsigned(JobWitness):i==49?unsigned(Bfp8MaskLow):i==50?unsigned(Bfp8MaskHigh):Base::BootWord(i);
}
QGW8_HEAD_FN bool BootShape(const uint32_t* b){
 if(!b || b[0]!=Magic || b[40]!=JobWitness || b[49]!=Bfp8MaskLow || b[50]!=Bfp8MaskHigh)return false;
 uint32_t old[64];for(unsigned i=0;i<64;i++)old[i]=b[i];
 old[0]=Base::Magic;old[40]=Base::JobWitness;old[49]=Base::Bfp8MaskLow;old[50]=Base::Bfp8MaskHigh;
 return Base::BootShape(old);
}
static inline bool HeadJob(const uint32_t* j){return j && j[2]==GlobalMatrix && j[16]==PioneerQ38Bfp8WorkerJob::Magic;}
static inline bool FpuJob(const uint32_t* j){return HeadJob(j) || Base::FpuJob(j);}
// The head-only coordinator must not link the much larger FFN/general job
// grammar into its small ROM. Equivalent head restrictions, DATA only.
QGW8_HEAD_FN bool HeadShape(const uint32_t* j,const uint32_t* b,unsigned previous,unsigned densePhase){
 if(!j || !BootShape(b) || j[0]!=Magic || !HeadJob(j) || densePhase || previous>=0x7ffffffe
  || j[1]!=previous+1 || !j[1] || j[1]>=0x7fffffff || j[3]!=b[3] || j[4]!=b[4] || j[5]!=64
  || j[6]>=8192 || !j[7] || j[7]>=0x7fffffff)return false;
 for(unsigned i=8;i<16;i++)if(j[i]!=b[i])return false;
 PioneerQ38Bfp8WorkerJob::Job leaf;for(unsigned i=0;i<16;i++)leaf.words[i]=j[16+i];
 if(!PioneerQ38Bfp8WorkerJob::Shape(leaf) || leaf.words[2]!=5120 || leaf.words[6]!=b[3] || leaf.words[7]!=b[4]
  || leaf.words[11]!=j[1] || (j[32]&31) || j[32]>=Vocabulary || leaf.words[4]*32>Vocabulary-j[32]
  || j[33]!=Vocabulary || j[34]>=Waves || (j[34]==8 && b[3]>21) || j[35]!=JobWitness)return false;
 for(unsigned i=36;i<64;i++)if(j[i])return false;
 return true;
}
QGW8_HEAD_FN bool JobShape(const uint32_t* j,const uint32_t* b,unsigned previous,unsigned densePhase){
 if(!j || !BootShape(b) || j[0]!=Magic)return false;
 if(FpuJob(j)){
  if(densePhase || previous>=0x7ffffffe || j[1]!=previous+1 || !j[1] || j[1]>=0x7fffffff
   || j[3]!=b[3] || j[4]!=b[4] || j[6]>=8192 || !j[7] || j[7]>=0x7fffffff)return false;
  for(unsigned i=8;i<16;i++)if(j[i]!=b[i])return false;
  if(HeadJob(j)){
   PioneerQ38Bfp8WorkerJob::Job leaf;for(unsigned i=0;i<16;i++)leaf.words[i]=j[16+i];
   if(j[5]!=64 || !PioneerQ38Bfp8WorkerJob::Shape(leaf) || leaf.words[2]!=5120
    || leaf.words[6]!=b[3] || leaf.words[7]!=b[4] || leaf.words[11]!=j[1]
    || (j[32]&31) || j[32]>=Vocabulary || leaf.words[4]*32>Vocabulary-j[32]
    || j[33]!=Vocabulary || j[34]>=Waves || (j[34]==8 && b[3]>21) || j[35]!=JobWitness)return false;
  }else{
   if(j[5]>=64)return false;
   const bool eight=((j[5]<32?b[49]:b[50])>>(j[5]&31))&1;
   if(eight ? (j[35]!=8 || j[16]!=PioneerQ38Bfp8WorkerJob::Magic || !Base::LeafShape<PioneerQ38Bfp8WorkerJob>(j,b))
            : (j[35]!=4 || j[16]!=PioneerQ38Bfp4WorkerJob::Magic || !Base::LeafShape<PioneerQ38Bfp4WorkerJob>(j,b)))return false;
  }
  for(unsigned i=36;i<64;i++)if(j[i])return false;
  return true;
 }
 if(j[2]==GlobalMatrix)return false; // old Q6 head is never accepted as QGW8
 uint32_t oldBoot[64],oldJob[64];for(unsigned i=0;i<64;i++){oldBoot[i]=b[i];oldJob[i]=j[i];}
 oldBoot[0]=oldJob[0]=Base::Magic;oldBoot[40]=Base::JobWitness;
 oldBoot[49]=Base::Bfp8MaskLow;oldBoot[50]=Base::Bfp8MaskHigh;
 return Base::JobShape(oldJob,oldBoot,previous,densePhase);
}
#undef QGW8_HEAD_FN
}
#endif
#if defined(PIONEER_Q38_QGWD_ROOT_IMAGE) || defined(PIONEER_Q38_QGWD_PLAN_SIMULATION)
static_assert(PioneerQ38HybridHeadWorkerLayout::Magic==0x44574751,"late QGWD coordinator identity selection");
#elif defined(PIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE) || defined(PIONEER_Q38_QGWC_SRCA_ROOT_IMAGE) || defined(PIONEER_Q38_QGWC_PLAN_SIMULATION) || defined(PIONEER_Q38_QGWC_NATIVE_136)
static_assert(PioneerQ38HybridHeadWorkerLayout::Magic==0x43574751,"late QGWC candidate selection");
#else
static_assert(PioneerQ38HybridHeadWorkerLayout::Magic!=0x43574751 && PioneerQ38HybridHeadWorkerLayout::Magic!=0x44574751,"late QGWC/QGWD identity removal");
#endif
