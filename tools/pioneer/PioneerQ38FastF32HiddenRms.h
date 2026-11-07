/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FAST_F32_HIDDEN_RMS_H
#define PIONEER_Q38_FAST_F32_HIDDEN_RMS_H
#if !defined(PIONEER_Q38_FAST_RMS_DIAGNOSTIC) || PIONEER_Q38_FAST_RMS_DIAGNOSTIC != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || defined(__FAST_MATH__)
#error Approximate RMS is an isolated diagnostic, not a native or Work implementation
#endif
#include "PioneerQ38F32HiddenRms.h"

// Candidate ONLY: replace the serial, software-binary64 sum of rounded
// squares with sixteen independent FP32 sums. The order and result can differ
// from the precise reference. Same domain/alias checks and no CPU reference
// arithmetic in the candidate itself. Full-model quality, emitted instructions,
// device ownership, a runtime selector and speed are NOT established here.
static __attribute__((noinline)) bool
PioneerQ38FastF32HiddenRms(const float* input,const float* weight,float* output){
 if(!input || !weight || !output)return false;
 using Address=__UINTPTR_TYPE__;
 const Address a=reinterpret_cast<Address>(input),b=reinterpret_cast<Address>(weight),c=reinterpret_cast<Address>(output);
 if((a|b|c)&3 || a>~Address(0)-20480 || b>~Address(0)-20480 || c>~Address(0)-20480
  || (a<b+20480 && b<a+20480) || (a<c+20480 && c<a+20480) || (b<c+20480 && c<b+20480))return false;
 float sums[16];volatile float* initial=sums;
 for(unsigned i=0;i<16;i++)initial[i]=0; // freestanding image needs no memset helper
 for(unsigned i=0;i<5120;i++){
  const float x=input[i];
  if(!PioneerQ38F32HiddenDomain(x) || !PioneerQ38RmsDomain(weight[i]))return false;
  sums[i&15]+=x*x;
 }
 float total=0;for(unsigned i=0;i<16;i++)total+=sums[i];
 const float mean=total*(1.0f/5120.0f);
 const float denominator=PioneerGraphSqrtPositive(mean+1e-6f),scale=PioneerGraphDividePositive(1,denominator);
 if(!(scale>0) || !PioneerQ38RmsDomain(scale))return false;
 for(unsigned i=0;i<5120;i++){
  const float value=(input[i]*scale)*weight[i];const uint32_t magnitude=PioneerAttentionBits(value)&0x7fffffff;
  if(magnitude>=0x7f800000 || (magnitude && magnitude<0x00800000))return false;
  output[i]=value;
 }
 return true;
}
static __attribute__((noinline)) bool
PioneerQ38FastF32HiddenResidualRms(const float* projected,const float* raw,const float* weight,float* residual,float* normalized){
 using Address=__UINTPTR_TYPE__;
 const Address spans[]={reinterpret_cast<Address>(projected),reinterpret_cast<Address>(raw),reinterpret_cast<Address>(weight),
  reinterpret_cast<Address>(residual),reinterpret_cast<Address>(normalized)};
 for(unsigned i=0;i<5;i++){
  if(!spans[i] || (spans[i]&3) || spans[i]>~Address(0)-20480)return false;
  for(unsigned j=0;j<i;j++)if(spans[i]<spans[j]+20480 && spans[j]<spans[i]+20480)return false;
 }
 for(unsigned i=0;i<5120;i++){
  if(!PioneerQ38F32HiddenDomain(projected[i]) || !PioneerQ38F32HiddenDomain(raw[i]))return false;
  const float value=raw[i]+projected[i];if(!PioneerQ38F32HiddenDomain(value))return false;
  residual[i]=value;
 }
 return PioneerQ38FastF32HiddenRms(residual,weight,normalized);
}
#endif
