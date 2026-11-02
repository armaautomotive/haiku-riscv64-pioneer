/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_LAYER29_WIDE_REDUCTION_DIAGNOSTIC) || PIONEER_Q38_LAYER29_WIDE_REDUCTION_DIAGNOSTIC != 1 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(__FAST_MATH__) || defined(PIONEER_Q38_LAYER29_EMITTED_DELTA_DIAGNOSTIC)
#error Separate HOST arithmetic experiment only; no CPU oracle, native or Work
#endif
#include "../PioneerBlackholeFma.h"
#include "pioneer_q38_layer29_wide_reduction_bridge.h"
#include <array>
#include <cfenv>

namespace {
struct Pair {float high,low;};
struct Arithmetic {
 uint64_t operations=0;
 float Mad(float a,float b,float c){operations++;return PioneerBlackholeFma(a,b,c);}
 float Add(float a,float b){return Mad(1,a,b);}
 float Mul(float a,float b){return Mad(a,b,-0.f);}
 Pair Product(float a,float b){
  // Split the operands before computing the product residual. The native
  // MAD is only partially fused:plain fma(a,b,-roundedProduct) is INVALID.
  const float ah=PioneerFmaFloat(PioneerFmaBits(a)&~0xfffU);
  const float bh=PioneerFmaFloat(PioneerFmaBits(b)&~0xfffU);
  const float al=Add(a,-ah),bl=Add(b,-bh),p=Mul(a,b);
  float error=Mad(ah,bh,-p);
  error=Mad(al,bh,error);error=Mad(ah,bl,error);error=Mad(al,bl,error);
  return {p,error};
 }
 Pair Sum(Pair a,Pair b){
  const float high=Add(a.high,b.high),v=Add(high,-a.high);
  const float low=Add(Add(Add(a.high,-Add(high,-v)),Add(b.high,-v)),Add(a.low,b.low));
  const float normalized=Add(high,low),w=Add(normalized,-high);
  return {normalized,Add(Add(high,-Add(normalized,-w)),Add(low,-w))};
 }
 float Rounded(Pair a){return Add(a.high,a.low);}
};
bool Domain(float value,uint32_t limit){
 const uint32_t bits=PioneerFmaBits(value)&0x7fffffffU;
 return bits<=limit && (!bits || (bits&0x7f800000U));
}
}
bool Q38Layer29WideReductionCandidate(const float* q,const float* k,const float* value,
 float decay,float beta,float* state,float* output,uint64_t* operations){
 if(!q || !k || !value || !state || !output || !operations || std::fegetround()!=FE_TONEAREST
  || !Domain(decay,0x3f800000) || !Domain(beta,0x3f800000)
  || (PioneerFmaBits(decay)&0x80000000U) || (PioneerFmaBits(beta)&0x80000000U))return false;
 for(unsigned i=0;i<128;i++)if(!Domain(q[i],0x40800000) || !Domain(k[i],0x40800000)
  || !Domain(value[i],0x49800000))return false;
 for(unsigned i=0;i<16384;i++)if(!Domain(state[i],0x49800000))return false;
 // HOST staging only. This is deliberately not a native stack/layout API.
 std::array<float,16384> next={};std::array<float,128> result={};Arithmetic math;
 for(unsigned v=0;v<128;v++){
  Pair prediction={0,0};
  for(unsigned key=0;key<128;key++)prediction=math.Sum(prediction,math.Product(math.Mul(state[key*128+v],decay),k[key]));
  const float delta=math.Mul(math.Add(value[v],-math.Rounded(prediction)),beta);
  Pair sum={0,0};
  for(unsigned key=0;key<128;key++){
   const unsigned at=key*128+v;
   next[at]=math.Add(math.Mul(state[at],decay),math.Mul(k[key],delta));
   if(!Domain(next[at],0x4d800000))return false;
   sum=math.Sum(sum,math.Product(next[at],q[key]));
  }
  result[v]=math.Mul(math.Rounded(sum),PioneerFmaFloat(0x3db504f3));
  if(!Domain(result[v],0x51800000))return false;
 }
 for(unsigned i=0;i<16384;i++)state[i]=next[i];
 for(unsigned i=0;i<128;i++)output[i]=result[i];
 *operations=math.operations;return true;
}
