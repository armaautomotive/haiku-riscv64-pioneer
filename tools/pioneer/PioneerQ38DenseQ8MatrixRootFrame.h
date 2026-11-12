/* SPDX-License-Identifier: MIT */
// Offline frame construction only. The caller must independently bind the
// schedule to sealed canonical weights and prove worker/input ownership.
#if !defined(PIONEER_Q38_DENSE_Q8_MATRIX_ROOT_INSPECTION) || PIONEER_Q38_DENSE_Q8_MATRIX_ROOT_INSPECTION != 1 \
 || !defined(PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION) || PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Dense Q8 ROOT frame is isolated offline inspection only
#endif
#ifndef PIONEER_Q38_DENSE_Q8_MATRIX_ROOT_FRAME_H
#define PIONEER_Q38_DENSE_Q8_MATRIX_ROOT_FRAME_H
#ifdef PIONEER_ATTENTION_FREESTANDING
typedef __UINT32_TYPE__ uint32_t;
#else
#include <stdint.h>
#endif
#include "PioneerQ38DenseQ8MatrixWorkerLayout.h"
struct PioneerQ38DenseQ8MatrixRootFrame {
 using M=PioneerQ38Bfp8WorkerJob;
 // tuple is the scheduler's address, bank, first row and band count.
 // Geometry validation does not authenticate the tuple or boot record.
#ifdef PIONEER_Q38_QGWD_ROOT_IMAGE
 static __attribute__((noinline,section(".bf16.q8frame"))) bool Build(const uint32_t* tuple,const uint32_t* boot,unsigned layer,unsigned role,
#else
 static bool Build(const uint32_t* tuple,const uint32_t* boot,unsigned layer,unsigned role,
#endif
  unsigned position,unsigned invocation,unsigned previous,uint32_t* output){
  namespace W=PioneerQ38DenseQ8MatrixWorkerLayout;
  if(!tuple || !boot || !output || previous>=0x7ffffffe)return false;
  using Address=__UINTPTR_TYPE__;
  const Address t=reinterpret_cast<Address>(tuple),b=reinterpret_cast<Address>(boot),o=reinterpret_cast<Address>(output);
  const Address limit=~Address(0);
  if(((t|b|o)&3) || t>limit-16 || b>limit-256 || o>limit-256
   || (o<t+16 && t<o+256) || (o<b+256 && b<o+256))return false;
  if(tuple[1]>=7 || !W::BootShape(boot))return false;
  uint32_t next[64]={};const unsigned sequence=previous+1,worker=boot[3];
  unsigned width=5120,rows=role==5?12288:1024;
#ifdef PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION
  if(layer<64 && layer%4!=3 && (role==5 || role==6 || role==13)){
   width=role==13?6144:5120;rows=role==5?6144:role==6?10240:5120;
  }
#endif
  const unsigned prefix[]={W::Magic,sequence,1,worker,boot[4],layer,position,invocation};
  for(unsigned i=0;i<8;i++){next[i]=prefix[i];next[8+i]=boot[8+i];}
  const unsigned leaf[]={M::Magic,M::Version,width,tuple[0],tuple[3],PioneerQ38ProjectionRoute(tuple[1]),worker,boot[4],
   PioneerQ38Bf16Wire::Magic,M::RootInput,M::RootRoute,sequence,M::Output,0,0,0};
  for(unsigned i=0;i<16;i++)next[16+i]=leaf[i];
  next[32]=tuple[2];next[33]=rows;next[34]=role;next[35]=8;
  if(!W::JobShape(next,boot,previous,0))return false;
  for(unsigned i=0;i<64;i++)output[i]=next[i];
  return true;
 }
};
#endif
