/* SPDX-License-Identifier: MIT */
// Live on-card producer arithmetic,not a native/cohort/Work permission.
#if !defined(PIONEER_Q38_BF16_ACTIVATION_IMAGE) || PIONEER_Q38_BF16_ACTIVATION_IMAGE != 1
#error Exact distinct BF16 activation producer required
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_PRECISE_ACTIVATION_CHUNK_IMAGE) || defined(__FAST_MATH__)
#error BF16 activation cannot alias integer-scaled/CPU/native/Work producers
#endif
#ifndef PIONEER_Q38_BF16_ACTIVATION_CHUNK_H
#define PIONEER_Q38_BF16_ACTIVATION_CHUNK_H
#include "PioneerQ38Bf16Input.h"
#include "PioneerQ38SigmoidPrecision.h"
#include "PioneerQ38LearnedRms.h"

// Same on-card (gate * precise_sigmoid(gate)) * up order/domain as the
// existing precise producer,WITHOUT its integer-Q8 quantization/scales.
// Both local outputs publish only after the entire chunk and BF16 packing
// pass. This does not provide NoC/completion/source-lifetime authority.
static __attribute__((noinline,unused)) bool PioneerQ38Bf16ActivationChunk(
 const float* gate,const float* up,unsigned count,float* activated,uint32_t* packed){
 (void)&PioneerQ38LearnedRms;
 if(count!=256)return false;
 const float* buffers[]={gate,up,activated,reinterpret_cast<const float*>(packed)};
 const unsigned words[]={256,256,256,128};
 using Address=__UINTPTR_TYPE__;Address addresses[4];const Address limit=~Address(0);
 for(unsigned a=0;a<4;a++){
  addresses[a]=reinterpret_cast<__UINTPTR_TYPE__>(buffers[a]);
  if(!buffers[a] || (addresses[a]&3) || addresses[a]>limit-words[a]*4)return false;
 }
 for(unsigned a=0;a<4;a++)for(unsigned b=a+1;b<4;b++)
  if(!(addresses[a]+words[a]*4<=addresses[b] || addresses[b]+words[b]*4<=addresses[a]))return false;
 uint32_t stage[256],bf16[128];
 for(unsigned i=0;i<256;i++){
  const float a=gate[i],b=up[i];float sigmoid;
  if(!PioneerQ38RmsDomain(a) || !PioneerQ38RmsDomain(b) || !PioneerQ38SigmoidPrecision(a,sigmoid))return false;
  const float value=(a*sigmoid)*b;
  if(!PioneerQ38RmsDomain(value))return false;
  stage[i]=PioneerAttentionBits(value);
 }
 if(!PioneerQ38Bf16Input::Pack(stage,256,bf16,128))return false;
 volatile float* out=activated;volatile uint32_t* destination=packed;
 for(unsigned i=0;i<256;i++)out[i]=PioneerAttentionFloat(stage[i]);
 for(unsigned i=0;i<128;i++)destination[i]=bf16[i];
 return true;
}
#endif
