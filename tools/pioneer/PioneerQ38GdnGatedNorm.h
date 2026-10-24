/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_GATED_NORM_H
#define PIONEER_Q38_GDN_GATED_NORM_H
#if !defined(PIONEER_Q38_GDN_GATED_NORM_IMAGE) || PIONEER_Q38_GDN_GATED_NORM_IMAGE != 1 || !defined(PIONEER_Q38_GDN_PREPARE_IMAGE) || PIONEER_Q38_GDN_PREPARE_IMAGE != 1
#error Exact separate OFFLINE gated-normalization role and explicit card-math component required
#endif
#if defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(_KERNEL_MODE)
#error Local card arithmetic is not a host Work/kernel/reference fallback
#endif
// Reuse the qualified card-only sqrt/divide/exp/FTZ primitives. Its separate
// arithmetic-component flag must also be explicit;neither flag grants IO.
#include "PioneerQ38GdnPrepare.h"
// Fused weighted SiLU. Reordering the gate multiplication prevents a tiny
// exp/sigmoid intermediate from flushing a NORMAL final output. Numerical
// qualification is independent,not a bit-exact CPU-expression claim.
static inline float PioneerQ38GdnWeightedSilu(float weighted,float z){
 uint32_t magnitude=PioneerAttentionBits(weighted)&0x7fffffff;
 if(!PioneerQ38PrepareNormal(weighted) || magnitude>0x49800000 || !PioneerQ38PrepareScalar(z))return PioneerAttentionFloat(0x7fc00000);
 float numerator=PioneerQ38PrepareFlush(weighted*z),denominator=1;
 if(z>=0)denominator=1+PioneerQ38PrepareNegativeExp(-z);
 else if(z<=-128){
  // |weighted|<=2^20,|z|<=65504 => even the loose bound on
  // |weighted*z*exp(z)| is subnormal here. Never cut off a NORMAL result.
  return PioneerAttentionFloat(PioneerAttentionBits(numerator)&0x80000000);
 }else{
  float half=PioneerGraphExp(z*0.5f);
  numerator=PioneerQ38PrepareFlush(PioneerQ38PrepareFlush(numerator*half)*half);
  denominator=1+PioneerQ38PrepareFlush(half*half);
 }
 uint32_t bits=PioneerAttentionBits(numerator);
 float absolute=PioneerAttentionFloat(bits&0x7fffffff);
 float result=PioneerGraphDividePositive(absolute,denominator);
 return PioneerQ38PrepareFlush(PioneerAttentionFloat(PioneerAttentionBits(result)|(bits&0x80000000)));
}
// BOTH raw halves are required for the128-wide RMS reduction;the result
// is ONLY the requested64-value half. norm is shared128 learned F32 values,
// z is the SAME head's128 retained role5 outputs. Inputs remain immutable.
// Pinned delta-layer semantics use an FP32 square SUM,not GraphRmsNorm's
// double-like expansion. A future native owner must prove all real sources,
// pair readiness/epoch and private outputs;these pointers grant none of that.
static __attribute__((noinline)) bool PioneerQ38GdnGatedNormHalf(const float* raw,const float* norm,
 const float* z,unsigned half,float* output){
 (void)&PioneerQ38PrepareHead; // math component only;not an alternate preparation/launch path
 if(half>=2 || (reinterpret_cast<__UINTPTR_TYPE__>(output)&3))return false;
 const float* inputs[]={raw,norm,z};
 for(unsigned p=0;p<3;p++)if((reinterpret_cast<__UINTPTR_TYPE__>(inputs[p])&3)
  || !PioneerQ38Disjoint(inputs[p],128,output,64))return false;
 for(unsigned p=0;p<3;p++)for(unsigned i=0;i<128;i++)if(!PioneerQ38PrepareScalar(inputs[p][i]))return false;
 float sum=0;
 for(unsigned i=0;i<128;i++)sum+=PioneerQ38PrepareFlush(raw[i]*raw[i]);
 float inverse=PioneerGraphDividePositive(1,PioneerGraphSqrtPositive(PioneerQ38PrepareFlush(sum*0.0078125f)+1e-6f));
 if(!(inverse>0) || !PioneerQ38PrepareNormal(inverse))return false;
 for(unsigned i=0;i<64;i++){
  unsigned at=half*64+i;
  float weighted=PioneerQ38PrepareFlush(PioneerQ38PrepareFlush(raw[at]*inverse)*norm[at]);
  float value=PioneerQ38GdnWeightedSilu(weighted,z[at]);
  if(!PioneerQ38PrepareNormal(value))return false;
  output[i]=value;
 }
 return true; // LOCAL scratch only;future completion seal/NoC write proof still required
}
#endif
