/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BF16_INPUT_H
#define PIONEER_Q38_BF16_INPUT_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(__FAST_MATH__)
#error BF16 producer integration is not an installed native/Work/reference capability
#endif
#ifndef PIONEER_ATTENTION_FREESTANDING
#include <stdint.h>
#else
typedef __UINT32_TYPE__ uint32_t;
typedef __UINTPTR_TYPE__ uintptr_t;
#endif
// On-card integer-bit RNE packing of LIVE FP32 activations,not weights or
// learned host arithmetic. Publish a chunk only after every lane validates.
// No integer-Q8 scale,clipping,NaN/Inf/subnormal acceptance or fallback.
struct PioneerQ38Bf16Input {
 enum : unsigned {Magic=0x31494251,Version=1,Values=256,Words=128}; // QBI1
 static bool Lane(uint32_t bits,uint32_t& out){
  const unsigned exponent=bits&0x7f800000;
  if(exponent==0x7f800000 || (!exponent && (bits&0x7fffff)))return false;
  const unsigned rounded=(bits+0x7fff+((bits>>16)&1))>>16;
  const unsigned e=rounded&0x7f80,m=rounded&127;
  if(e==0x7f80 || (!e && m))return false;
  out=rounded;return true;
 }
 static bool Pack(const uint32_t* source,unsigned count,uint32_t* destination,unsigned capacity){
  if(!source || !destination || count!=Values || capacity<Words)return false;
  const uintptr_t a=reinterpret_cast<uintptr_t>(source),b=reinterpret_cast<uintptr_t>(destination),limit=~uintptr_t(0);
  if((a&3) || (b&3) || a>limit-1024 || b>limit-512 || !(a+1024<=b || b+512<=a))return false;
  uint32_t stage[Words];
  for(unsigned i=0;i<Words;i++){
   unsigned low=0,high=0;
   if(!Lane(source[i*2],low) || !Lane(source[i*2+1],high))return false;
   stage[i]=low|(high<<16);
  }
  volatile uint32_t* out=destination;
  for(unsigned i=0;i<Words;i++)out[i]=stage[i];
  return true;
 }
};
#endif
