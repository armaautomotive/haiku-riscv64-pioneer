/* SPDX-License-Identifier: MIT */
#include "PioneerQ38SiluIsolation.h"
#if !defined(PIONEER_Q38_SILU_PRECISION_IMAGE) || PIONEER_Q38_SILU_PRECISION_IMAGE != 1 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(_KERNEL_MODE) || defined(PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE) || defined(__FAST_MATH__)
#error Exact isolated card SiLU precision candidate,never a native or host fallback
#endif
#ifndef PIONEER_Q38_SILU_PRECISION_H
#define PIONEER_Q38_SILU_PRECISION_H
#include "PioneerGraphMath.h"
// Isolated arithmetic candidate ONLY. Preserve the existing two-float exp
// through denominator formation and a compensated division correction.
// No existing math/default/native image is changed by this header.
static inline PioneerAttentionWide PioneerQ38SiluExpReduced(float value,int& power){
 float absolute=PioneerAttentionFloat(PioneerAttentionBits(value)&0x7fffffff);
 uint32_t bits=PioneerAttentionBits(absolute*1.4426950408889634f);
 int exponent=int((bits>>23)&255)-127;unsigned rounded=0;
 if(exponent>=-1){unsigned shift=unsigned(23-exponent);rounded=(((bits&0x7fffff)|0x800000)+(1U<<(shift-1)))>>shift;}
 int n=(PioneerAttentionBits(value)&0x80000000)?-int(rounded):int(rounded);
 auto offset=PioneerAttentionWideMultiply({float(n),0},{0.6931471824645996f,-1.904654323148236e-9f});
 auto r=PioneerAttentionWideAdd({value,0},{-offset.high,-offset.low});
 static const uint32_t coefficient[11][2]={
  {0x3f800000,0},{0x3f800000,0},{0x3f000000,0},{0x3e2aaaab,0xb1aaaaab},
  {0x3d2aaaab,0xb0aaaaab},{0x3c088889,0xafeeeeef},{0x3ab60b61,0xae13e93f},
  {0x39500d01,0xac3fcbfd},{0x37d00d01,0xaabfcbfd},{0x3638ef1d,0x292ad8e6},{0x3493f27e,0xa808760a}
 };
 PioneerAttentionWide result={PioneerAttentionFloat(coefficient[10][0]),PioneerAttentionFloat(coefficient[10][1])};
 for(int i=9;i>=0;i--)result=PioneerAttentionWideAdd(PioneerAttentionWideMultiply(result,r),
  {PioneerAttentionFloat(coefficient[i][0]),PioneerAttentionFloat(coefficient[i][1])});
 power=n;return result;
}
static inline float PioneerQ38SiluFlush(float value){
 uint32_t bits=PioneerAttentionBits(value);return bits&0x7f800000?value:PioneerAttentionFloat(bits&0x80000000);
}
static inline float PioneerQ38SiluScale(float value,int power){
 // For a deep negative tail,keep BOTH scale operands normal. The final
 // rounded value alone may FTZ;never flush an intermediate exp to zero.
 if(power < -126){value*=PioneerAttentionFloat(uint32_t(power+64+127)<<23);return PioneerQ38SiluFlush(value*PioneerAttentionFloat(0x1f800000));}
 return PioneerQ38SiluFlush(value*PioneerAttentionFloat(uint32_t(power+127)<<23));
}
static inline float PioneerQ38SiluDivideWide(PioneerAttentionWide numerator,PioneerAttentionWide denominator){
 float quotient=PioneerGraphDividePositive(numerator.high,denominator.high);
 auto product=PioneerAttentionWideMultiply({quotient,0},denominator);
 auto residual=PioneerAttentionWideAdd(numerator,{-product.high,-product.low});
 float error=residual.high+residual.low;
 uint32_t errorBits=PioneerAttentionBits(error);
 float correction=PioneerGraphDividePositive(PioneerAttentionFloat(errorBits&0x7fffffff),denominator.high);
 correction=PioneerAttentionFloat(PioneerAttentionBits(correction)|(errorBits&0x80000000));
 auto refined=PioneerAttentionWideAdd({quotient,0},{correction,0});
 return refined.high+refined.low;
}
static inline bool PioneerQ38SiluPrecision(float value,float& output){
 uint32_t bits=PioneerAttentionBits(value),absolute=bits&0x7fffffff;
 // Full finite NORMAL-or-zero INPUT domain. Explicit final-result FTZ
 // matches the card;no subnormal input or CPU/legacy fallback is accepted.
 if(absolute>=0x7f800000 || (absolute && absolute<0x00800000))return false;
 if(!absolute){output=value;return true;}
 if(value>=20.f){output=value;return true;} // neglected correction <0.04binary32 ULP
 if(value<=-104.f){output=PioneerAttentionFloat(0x80000000);return true;} // final result is subnormal
 if(absolute<=0x32800000){output=PioneerQ38SiluFlush(value*.5f);return true;} // correction <0.125ULP
 float magnitude=PioneerAttentionFloat(absolute),result;int power;
 if(value < -20.f){
  auto exponential=PioneerQ38SiluExpReduced(value,power);
  float tail=PioneerQ38SiluScale(exponential.high,power);
  auto denominator=PioneerAttentionWideAdd({1,0},{tail,0});
  auto numerator=PioneerAttentionWideMultiply({magnitude,0},exponential);
  result=PioneerQ38SiluScale(PioneerQ38SiluDivideWide(numerator,denominator),power);
 }else{
  auto exponential=PioneerQ38SiluExpReduced(-value,power);
  exponential.high=PioneerQ38SiluScale(exponential.high,power);exponential.low=PioneerQ38SiluScale(exponential.low,power);
  auto denominator=PioneerAttentionWideAdd({1,0},exponential);
  result=PioneerQ38SiluDivideWide({magnitude,0},denominator);
 }
 uint32_t encoded=PioneerAttentionBits(result)|(bits&0x80000000);
 uint32_t normal=encoded&0x7fffffff;
 if(normal>=0x7f800000 || (normal && normal<0x00800000))return false;
 output=PioneerAttentionFloat(encoded);return true;
}
#endif
