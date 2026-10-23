/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_CONV_MATH_H
#define PIONEER_Q38_CONV_MATH_H
#if defined(PIONEER_Q38_WORK_INFERENCE) || defined(_KERNEL_MODE)
#error QCV1 candidate has NO host Work/kernel numerical fallback
#endif
#include "PioneerGraphMath.h"
#include "PioneerQwen38Hybrid.h"
// Card-side16channel Conv4+SiLU. ALL local math completes BEFORE GDDR
// writes. Previous input/state/ORIGINAL weights immutable;no CPU policy,
// address capability,whole-model registration or implicit fallback.
static inline bool PioneerQ38ConvDomain(float value){
 uint32_t a=PioneerAttentionBits(value)&0x7fffffff;return !a || (a>=0x26800000 && a<=0x477fe000);
}
static inline bool PioneerQ38ConvNormal(float value){
 uint32_t a=PioneerAttentionBits(value)&0x7fffffff;return !a || (a>=0x00800000 && a<0x7f800000);
}
static __attribute__((noinline)) bool PioneerQ38Conv16(const float* input,const float* weights,const float* previous,float* next,float* output){
 const float* read[]={input,weights,previous};const unsigned size[]={16,64,48};
 if(!PioneerQ38Disjoint(next,48,output,16))return false;
 for(unsigned p=0;p<3;p++){
  if(!read[p] || (reinterpret_cast<__UINTPTR_TYPE__>(read[p])&3)
   || !PioneerQ38Disjoint(next,48,read[p],size[p]) || !PioneerQ38Disjoint(output,16,read[p],size[p]))return false;
  for(unsigned i=0;i<size[p];i++)if(!PioneerQ38ConvDomain(read[p][i]))return false;
 }
 if((reinterpret_cast<__UINTPTR_TYPE__>(next)|reinterpret_cast<__UINTPTR_TYPE__>(output))&3)return false;
 for(unsigned c=0;c<16;c++){
  float sum=0;
  for(unsigned t=0;t<4;t++){
   float value=(t==3?input[c]:previous[c*3+t])*weights[c*4+t];
   if(!PioneerQ38ConvNormal(value))return false;
   sum+=value;if(!PioneerQ38ConvNormal(sum))return false;
  }
  float value=PioneerGraphSilu(sum);if(!PioneerQ38ConvNormal(value))return false;output[c]=value;
  next[c*3]=previous[c*3+1];next[c*3+1]=previous[c*3+2];next[c*3+2]=input[c];
 }
 return true;
}
#endif
