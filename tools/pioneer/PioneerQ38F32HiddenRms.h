/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_F32_HIDDEN_RMS_H
#define PIONEER_Q38_F32_HIDDEN_RMS_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE)
#error Device arithmetic/CPU-only qualification is not a host Work or kernel fallback
#endif
#include "PioneerQ38LearnedRms.h"
// NEW bounded raw FP32 hidden-state arithmetic, NOT a wider FP16 activation
// or learned-weight domain. Existing math/images remain byte-for-byte old.
// |x|<=2^26 => rounded x*x<=2^52,5120-term sum<2^65,mean<=2^52.
// With epsilon1e-6,scale>=2^-26; nonzero x/weight>=2^-50 keeps BOTH
// rounded normalization multiplies normal:2^-50*2^-26*2^-50=2^-126.
// Later FP16/native quantization must STILL enforce its own65504 bound.
static inline bool PioneerQ38F32HiddenDomain(float value){
 uint32_t a=PioneerAttentionBits(value)&0x7fffffff;return a<=0x4c800000 && (!a || a>=0x26800000);
}
static __attribute__((noinline)) bool PioneerQ38F32HiddenRms(const float* input,const float* weight,float* output){
 if(!input || !weight || !output)return false;
 using Address=__UINTPTR_TYPE__;Address a=reinterpret_cast<Address>(input),b=reinterpret_cast<Address>(weight),c=reinterpret_cast<Address>(output);
 if((a|b|c)&3 || a>~Address(0)-20480 || b>~Address(0)-20480 || c>~Address(0)-20480
  || (a<b+20480 && b<a+20480) || (a<c+20480 && c<a+20480) || (b<c+20480 && c<b+20480))return false;
 PioneerQ38RmsSum sum;
 for(unsigned i=0;i<5120;i++){
  if(!PioneerQ38F32HiddenDomain(input[i]) || !PioneerQ38RmsDomain(weight[i]) || !sum.Add(input[i]*input[i]))return false;
 }
 float mean;if(!sum.Mean5120(mean))return false;
 float denominator=PioneerGraphSqrtPositive(mean+1e-6f),scale=PioneerGraphDividePositive(1,denominator);
 if(!(scale>0) || !PioneerQ38RmsDomain(scale))return false;
 for(unsigned i=0;i<5120;i++){
  float normalized=input[i]*scale,value=normalized*weight[i];uint32_t magnitude=PioneerAttentionBits(value)&0x7fffffff;
  if(magnitude>=0x7f800000 || (magnitude && magnitude<0x00800000))return false;
  output[i]=value;
 }
 return true;
}
static __attribute__((noinline)) bool PioneerQ38F32HiddenResidualRms(const float* projected,const float* raw,const float* weight,float* residual,float* normalized){
 using Address=__UINTPTR_TYPE__;
 const Address spans[]={reinterpret_cast<Address>(projected),reinterpret_cast<Address>(raw),reinterpret_cast<Address>(weight),reinterpret_cast<Address>(residual),reinterpret_cast<Address>(normalized)};
 for(unsigned i=0;i<5;i++){
  if(!spans[i] || (spans[i]&3) || spans[i]>~Address(0)-20480)return false;
  for(unsigned j=0;j<i;j++)if(spans[i]<spans[j]+20480 && spans[j]<spans[i]+20480)return false;
 }
 for(unsigned i=0;i<5120;i++){
  if(!PioneerQ38F32HiddenDomain(projected[i]) || !PioneerQ38F32HiddenDomain(raw[i]))return false;
  float value=raw[i]+projected[i];if(!PioneerQ38F32HiddenDomain(value))return false;
  residual[i]=value;
 }
 return PioneerQ38F32HiddenRms(residual,weight,normalized);
}
#endif
