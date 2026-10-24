/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_PREPARE_H
#define PIONEER_Q38_GDN_PREPARE_H
#if !defined(PIONEER_Q38_GDN_PREPARE_IMAGE) || PIONEER_Q38_GDN_PREPARE_IMAGE != 1
#error Exact separate OFFLINE GDN preparation image role required
#endif
#if defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(_KERNEL_MODE)
#error Card-image arithmetic is NOT a host Work/kernel/reference fallback
#endif
#include "PioneerGraphNorm.h"
#include "PioneerQwen38Hybrid.h"
// Missing CARD arithmetic between actual Conv4 and recurrent SFPU work.
// No native selector/ownership/source/producer grant. A future combined
// resident worker must source these bytes from its actual QCB1/QPS1 and
// learned-page owners,not host-precomputed q/k/decay/beta products.
static inline bool PioneerQ38PrepareNormal(float v){
 uint32_t a=PioneerAttentionBits(v)&0x7fffffff;
 return !a || (a>=0x00800000 && a<0x7f800000);
}
static inline float PioneerQ38PrepareFlush(float v){
 uint32_t bits=PioneerAttentionBits(v);
 return bits&0x7f800000?v:PioneerAttentionFloat(bits&0x80000000);
}
static inline bool PioneerQ38PrepareScalar(float v){
 return PioneerQ38PrepareNormal(v) && (PioneerAttentionBits(v)&0x7fffffff)<=0x477fe000;
}
static inline float PioneerQ38PrepareNegativeExp(float x){
 // GDN gates may multiply LARGE prior state. Do not use the old activation
 // cutoff at-80:exp(-80..-87) is still NORMAL and must not become zero.
 if(!(x<=0))return PioneerAttentionFloat(0x7fc00000);
 if(x> -80)return PioneerGraphExp(x);
 if(x<=-104)return 0; // below even binary32's subnormal rounding range
 float half=PioneerGraphExp(x*0.5f);
 return PioneerQ38PrepareFlush(half*half); // ONLY actual subnormals -> FTZ
}
static inline float PioneerQ38PrepareSoftplus(float x){
 // log(1+e),e=exp(-abs(x)) in[0,1]. z=e/(2+e)<=1/3;
 // 2*atanh(z)'s ten odd terms have truncation error<1.1e-11.
 // Stable for either sign;NO exp(large positive),log/fdiv/fsqrt/FMA.
 float e=PioneerQ38PrepareNegativeExp(-PioneerAttentionFloat(PioneerAttentionBits(x)&0x7fffffff));
 // Avoid an e/2 subnormal intermediate for tiny NORMAL exp results.
 // log1p(e)=e with relative error<1.2e-7 throughout this branch.
 if(e<2.384185791015625e-7f)return x>0?x+e:e;
 float z=PioneerGraphDividePositive(e,2.0f+e),zz=z*z,p=1.0f/19.0f;
 p=p*zz+1.0f/17.0f;p=p*zz+1.0f/15.0f;p=p*zz+1.0f/13.0f;
 p=p*zz+1.0f/11.0f;p=p*zz+1.0f/9.0f;p=p*zz+1.0f/7.0f;
 p=p*zz+1.0f/5.0f;p=p*zz+1.0f/3.0f;p=p*zz+1.0f;
 float logarithm=(2.0f*z)*p;
 return x>0?x+logarithm:logarithm;
}
static inline bool PioneerQ38PrepareGates(float alpha,float betaLogit,float negativeA,float dtBias,float* gates){
 if(!gates || (reinterpret_cast<__UINTPTR_TYPE__>(gates)&3)
  || reinterpret_cast<__UINTPTR_TYPE__>(gates)>~static_cast<__UINTPTR_TYPE__>(0)-8
  || !PioneerQ38PrepareScalar(alpha) || !PioneerQ38PrepareScalar(betaLogit)
  || !PioneerQ38PrepareScalar(negativeA) || !PioneerQ38PrepareScalar(dtBias) || !(negativeA<0))return false;
 // GGUF ssm_a is ALREADY -exp(A_log). Do not exponentiate it again.
 float softplus=PioneerQ38PrepareSoftplus(alpha+dtBias);
 float decay=PioneerQ38PrepareNegativeExp(PioneerQ38PrepareFlush(negativeA*softplus));
 float e=PioneerQ38PrepareNegativeExp(-PioneerAttentionFloat(PioneerAttentionBits(betaLogit)&0x7fffffff));
 float beta=PioneerGraphDividePositive(betaLogit>=0?1.0f:e,1.0f+e);
 if(!PioneerQ38PrepareNormal(decay) || !PioneerQ38PrepareNormal(beta) || decay<0 || decay>1 || beta<0 || beta>1)return false;
 gates[0]=decay;gates[1]=beta;return true;
}
static __attribute__((noinline)) bool PioneerQ38PrepareHead(const float* query,const float* key,
 float alpha,float betaLogit,float negativeA,float dtBias,float* q,float* k,float* gates){
 const float* inputs[]={query,key};float* outputs[]={q,k,gates};const unsigned sizes[]={128,128,2};
 for(unsigned i=0;i<3;i++){
  if((reinterpret_cast<__UINTPTR_TYPE__>(outputs[i])&3))return false;
  for(unsigned j=i+1;j<3;j++)if(!PioneerQ38Disjoint(outputs[i],sizes[i],outputs[j],sizes[j]))return false;
  for(unsigned j=0;j<2;j++)if((reinterpret_cast<__UINTPTR_TYPE__>(inputs[j])&3)
   || !PioneerQ38Disjoint(outputs[i],sizes[i],inputs[j],128))return false;
 }
 float qs=0,ks=0;
 for(unsigned i=0;i<128;i++){
  if(!PioneerQ38PrepareNormal(query[i]) || !PioneerQ38PrepareNormal(key[i]))return false;
  // Match the pinned hybrid's FP32,not RMS's binary64 reduction. Explicit
  // FTZ mirrors BabyRISCV and makes tiny squared terms inert vs epsilon.
  float a=PioneerQ38PrepareFlush(query[i]*query[i]),b=PioneerQ38PrepareFlush(key[i]*key[i]);
  qs+=a;ks+=b;if(!PioneerQ38PrepareNormal(qs) || !PioneerQ38PrepareNormal(ks))return false;
 }
 float qi=PioneerGraphDividePositive(1.0f,PioneerGraphSqrtPositive(qs+1e-6f))
#ifdef PIONEER_Q38_GDN_INPUT_COMBINED_STAGE
#if PIONEER_Q38_GDN_INPUT_COMBINED_STAGE != 1
#error Exact combined stage required for unscaled query/post-dot recurrence
#endif
 // The combined worker calls the post-dot-scaled SFPU recurrence. NEVER
 // pre-scale Q here as well:that would silently scale every output twice.
 ;
#else
 *0.0883883461356163f;
#endif
 float ki=PioneerGraphDividePositive(1.0f,PioneerGraphSqrtPositive(ks+1e-6f));
 float g[2];if(!(qi>0) || !(ki>0) || !PioneerQ38PrepareNormal(qi) || !PioneerQ38PrepareNormal(ki)
  || !PioneerQ38PrepareGates(alpha,betaLogit,negativeA,dtBias,g))return false;
 for(unsigned i=0;i<128;i++){
  q[i]=PioneerQ38PrepareFlush(query[i]*qi);k[i]=PioneerQ38PrepareFlush(key[i]*ki);
  if(!PioneerQ38PrepareNormal(q[i]) || !PioneerQ38PrepareNormal(k[i]))return false;
 }
 gates[0]=g[0];gates[1]=g[1];return true; // LOCAL scratch only,not publication
}
#endif
