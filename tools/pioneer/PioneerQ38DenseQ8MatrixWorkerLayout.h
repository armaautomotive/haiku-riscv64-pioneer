/* SPDX-License-Identifier: MIT */
#include "PioneerQ38QgwdKernelCompileRole.h"
#include "PioneerQ38QgwdNativeInspectionRole.h"
#include "PioneerQ38Qgw8Bfp8BatchRole.h" // Recheck combined-role prerequisites on every include.
#if !defined(PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION) || PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION != 1 \
 || (!defined(PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION) && !defined(PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION) \
  && !(defined(PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION) && defined(PIONEER_Q38_QGWD_ROOT_IMAGE))) \
 || (defined(PIONEER_Q38_QGW8_BFP8_BATCH_WORKER) && !defined(PIONEER_Q38_QGWE_BATCH_DENSE_WORKER)) || defined(PIONEER_Q38_QGW8_BFP8_REUSE_WORKER) \
 || defined(PIONEER_Q38_QGWA_BATCH_ROOT_IMAGE) || defined(PIONEER_Q38_QGWA_BATCH_ROOT_RECEIPT_INSPECTION) \
 || (defined(_KERNEL_MODE) && !defined(PIONEER_Q38_QGWD_KERNEL_COMPILE_ONLY) && !defined(PIONEER_Q38_QGWD_NATIVE_INSPECTION)) \
 || ((defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT)) && !defined(PIONEER_Q38_QGWD_NATIVE_INSPECTION)) \
 || (defined(PIONEER_Q38_HYBRID_FULL_NATIVE) && !defined(PIONEER_Q38_QGWD_NATIVE_141)) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Dense Q8 matrix worker grammar is isolated offline inspection only
#endif
#ifndef PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_LAYOUT_H
#define PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_LAYOUT_H
#include "PioneerQ38HybridHeadWorkerLayout.h"
// New identity and dense-Q8-BFP8 shape, not authentication, dispatch or IO.
namespace PioneerQ38DenseQ8MatrixWorkerLayout {
namespace Base=PioneerQ38HybridHeadWorkerLayout;
using namespace PioneerQ38HybridHeadWorkerLayout;
#if defined(PIONEER_Q38_QGWF_BATCH4_WORKER) || defined(PIONEER_Q38_QGWF_DUAL_BATCH_ROOT) || defined(PIONEER_Q38_QGWF_DUAL_BATCH_PLAN)
enum : unsigned {Magic=0x46574751,JobWitness=0x464f4751}; // QGWF/QGOF, worker only
#elif defined(PIONEER_Q38_QGWE_BATCH_DENSE_WORKER) || defined(PIONEER_Q38_QGWE_BATCH_DENSE_ROOT) || defined(PIONEER_Q38_QGWE_BATCH_DENSE_PLAN)
enum : unsigned {Magic=0x45574751,JobWitness=0x454f4751}; // QGWE/QGOE; worker only
#elif defined(PIONEER_Q38_QGWD_SRCA_DENSE_Q8_WORKER) || defined(PIONEER_Q38_QGWD_ROOT_IMAGE) || defined(PIONEER_Q38_QGWD_PLAN_SIMULATION)
enum : unsigned {Magic=0x44574751,JobWitness=0x444f4751}; // QGWD/QGOD
#else
enum : unsigned {Magic=0x42574751,JobWitness=0x424f4751}; // QGWB/QGOB
#endif
static inline unsigned BootWord(unsigned i){return i==40?unsigned(JobWitness):Base::BootWord(i);}
static inline bool BootShape(const uint32_t* b){
 if(!b || b[0]!=Magic || b[40]!=JobWitness)return false;
 uint32_t old[64];for(unsigned i=0;i<64;i++)old[i]=b[i];old[0]=Base::Magic;old[40]=Base::JobWitness;
 return Base::BootShape(old);
}
static inline bool DenseJob(const uint32_t* j){
 return j && j[2]==1 && j[16]==PioneerQ38Bfp8WorkerJob::Magic && ((j[34]>=5 && j[34]<=7)
#ifdef PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION
  || j[34]==13
#endif
  );
}
static inline bool DenseProducerJob(const uint32_t* j){
 return j && j[2]==5 && ((j[28]>=5 && j[28]<=7)
#ifdef PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION
  || j[28]==13
#endif
  );
}
#ifdef PIONEER_Q38_QGWD_ROOT_IMAGE
static __attribute__((noinline,unused,section(".bf16.root"))) bool JobShape(const uint32_t* j,const uint32_t* b,unsigned previous,unsigned densePhase){
#else
static __attribute__((noinline,unused)) bool JobShape(const uint32_t* j,const uint32_t* b,unsigned previous,unsigned densePhase){
#endif
 if(!j || !BootShape(b) || j[0]!=Magic)return false;
 if(DenseProducerJob(j)){
  unsigned width=5120;
#ifdef PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION
  const bool recurrent=j[5]<64 && (j[5]&3)!=3 && (j[28]==5 || j[28]==6 || j[28]==13);
  if(recurrent && j[28]==13)width=6144;
#else
  const bool recurrent=false;
#endif
  if(densePhase || previous>=0x7ffffffe || !j[1] || j[1]!=previous+1 || j[1]>=0x7fffffff
   || j[3]!=b[3] || j[4]!=b[4] || j[5]>=64 || (!recurrent && ((j[5]&3)!=3 || j[28]>7)) || j[6]>=8192 || !j[7] || j[7]>=0x7fffffff
   || b[3]>width/256 || j[16]!=Producer::Magic || j[17]!=256 || j[18]!=512 || j[19]!=j[7] || j[20]!=12
   || j[27]>=248320 || j[29]!=width || j[30]!=Producer::Format || j[31]!=Producer::Witness)return false;
  for(unsigned i=8;i<16;i++)if(j[i]!=b[i])return false;
  for(unsigned i=21;i<27;i++)if(j[i])return false;
  for(unsigned i=32;i<64;i++)if(j[i])return false;
  return true;
 }
 if(DenseJob(j)){
  unsigned rows=j[34]==5?12288:1024,width=5120;
#ifdef PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION
  const bool recurrent=j[5]<64 && (j[5]&3)!=3 && (j[34]==5 || j[34]==6 || j[34]==13);
  if(recurrent){rows=j[34]==5?6144:j[34]==6?10240:5120;width=j[34]==13?6144:5120;}
#else
  const bool recurrent=false;
#endif
  if(densePhase || previous>=0x7ffffffe || !j[1] || j[1]!=previous+1 || j[1]>=0x7fffffff
   || j[3]!=b[3] || j[4]!=b[4] || j[5]>=64 || (!recurrent && (j[5]%4!=3 || j[34]>7)) || j[6]>=8192 || !j[7] || j[7]>=0x7fffffff)return false;
  for(unsigned i=8;i<16;i++)if(j[i]!=b[i])return false;
  PioneerQ38Bfp8WorkerJob::Job leaf;for(unsigned i=0;i<16;i++)leaf.words[i]=j[16+i];
  if(!PioneerQ38Bfp8WorkerJob::Shape(leaf) || leaf.words[2]!=width || leaf.words[6]!=b[3] || leaf.words[7]!=b[4]
   || leaf.words[11]!=j[1] || (j[32]&31) || j[32]>=rows || leaf.words[4]*32>rows-j[32] || j[33]!=rows || j[35]!=8)return false;
  for(unsigned i=36;i<64;i++)if(j[i])return false;
  return true;
 }
 uint32_t oldBoot[64],oldJob[64];for(unsigned i=0;i<64;i++){oldBoot[i]=b[i];oldJob[i]=j[i];}
 oldBoot[0]=oldJob[0]=Base::Magic;oldBoot[40]=Base::JobWitness;
 if(Base::HeadJob(oldJob)){
  if(oldJob[35]!=JobWitness)return false;
  oldJob[35]=Base::JobWitness;
 }
 return Base::JobShape(oldJob,oldBoot,previous,densePhase);
}
}
#endif
#if defined(PIONEER_Q38_QGWF_BATCH4_WORKER) || defined(PIONEER_Q38_QGWF_DUAL_BATCH_ROOT) || defined(PIONEER_Q38_QGWF_DUAL_BATCH_PLAN)
static_assert(PioneerQ38DenseQ8MatrixWorkerLayout::Magic==0x46574751,"late QGWF worker selection");
#elif defined(PIONEER_Q38_QGWE_BATCH_DENSE_WORKER) || defined(PIONEER_Q38_QGWE_BATCH_DENSE_ROOT) || defined(PIONEER_Q38_QGWE_BATCH_DENSE_PLAN)
static_assert(PioneerQ38DenseQ8MatrixWorkerLayout::Magic==0x45574751,"late QGWE combined worker selection");
#elif defined(PIONEER_Q38_QGWD_SRCA_DENSE_Q8_WORKER) || defined(PIONEER_Q38_QGWD_ROOT_IMAGE) || defined(PIONEER_Q38_QGWD_PLAN_SIMULATION)
static_assert(PioneerQ38DenseQ8MatrixWorkerLayout::Magic==0x44574751,"late QGWD combined worker selection");
#else
static_assert(PioneerQ38DenseQ8MatrixWorkerLayout::Magic!=0x44574751,"late QGWD combined worker removal");
#endif
