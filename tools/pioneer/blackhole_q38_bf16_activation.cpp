/* SPDX-License-Identifier: MIT */
// Returning live projection/gated-FFN producer at actual worker buffers.
#if !defined(__riscv) || __riscv_xlen!=32
#error BF16 activation image requires RV32
#endif
#define PIONEER_ATTENTION_FREESTANDING
#define PIONEER_Q38_SIGMOID_PRECISION_IMAGE 1
#include "PioneerQ38Bf16ActivationChunk.h"
extern "C" void bf16_activation_fault() __attribute__((noreturn));
extern "C" unsigned q38_bf16_activation_producer(const unsigned* descriptor){
 unsigned frame[32];for(unsigned i=0;i<32;i++)frame[i]=descriptor[i];
 if(frame[0]!=0x31414251 || frame[1]!=1 || frame[2]!=256 || (frame[3]!=1 && frame[3]!=2)
  || frame[4]!=512 || !frame[5] || frame[5]>=0x7fffffff)return 0;
 for(unsigned i=6;i<32;i++)if(frame[i])return 0;
 if(frame[3]==1){
  if(!PioneerQ38Bf16Input::Pack(reinterpret_cast<const uint32_t*>(0x90000),256,
   reinterpret_cast<uint32_t*>(0x90c00),128))bf16_activation_fault();
 }else if(!PioneerQ38Bf16ActivationChunk(reinterpret_cast<const float*>(0x90000),
  reinterpret_cast<const float*>(0x90400),256,reinterpret_cast<float*>(0x90800),
  reinterpret_cast<uint32_t*>(0x90c00)))bf16_activation_fault();
 for(unsigned i=0;i<32;i++)if(descriptor[i]!=frame[i])bf16_activation_fault();
 asm volatile("fence":::"memory");return 256;
}
#ifndef PIONEER_Q38_BF16_ACTIVATION_RETURN_ONLY
extern "C" unsigned scalar_main(){return q38_bf16_activation_producer(reinterpret_cast<const unsigned*>(0x74600));}
#endif
