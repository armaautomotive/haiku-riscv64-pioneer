/* SPDX-License-Identifier: MIT */
// Arithmetic candidate only. No source, native owner, image-install or
// Work permission. Recheck on late inclusion, including defined0 roles.
#if !defined(PIONEER_Q38_SIGMOID_PRECISION_IMAGE) || PIONEER_Q38_SIGMOID_PRECISION_IMAGE != 1
#error Exact isolated sigmoid precision candidate required
#endif
#if defined(_KERNEL_MODE) || defined(__FAST_MATH__) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT) || defined(PIONEER_Q38_MIXED_LIVE_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_MIXED_LIVE_VALIDATION_CLIENT) || defined(PIONEER_DRIVER_BUILD_TAG) \
 || defined(PIONEER_Q38_SILU_PRECISION_IMAGE) || defined(PIONEER_Q38_SILU_LEGACY_CONTROL_IMAGE)
#error Isolated sigmoid candidate is not CPU reference, old SiLU, native, kernel or Work
#endif
#ifndef PIONEER_Q38_SIGMOID_PRECISION_H
#define PIONEER_Q38_SIGMOID_PRECISION_H
#include "PioneerGraphMath.h"
namespace PioneerQ38SigmoidDetail {
inline float Flush(float value){unsigned bits=PioneerAttentionBits(value);return bits&0x7f800000?value:PioneerAttentionFloat(bits&0x80000000);}
// Preserve both floats through exp and denominator construction. The old
// gate rounded exp and 1+exp independently before its scalar division.
// Degree10 on [-ln2/2,ln2/2], same split constants as the qualified SiLU
// arithmetic, but a DISTINCT sigmoid role and numerator (not x*sigmoid).
inline PioneerAttentionWide ExpReduced(float value,int& power){
 unsigned bits=PioneerAttentionBits((-value)*1.4426950408889634f);
 int exponent=int((bits>>23)&255)-127;unsigned rounded=0;
 if(exponent>=-1){unsigned shift=unsigned(23-exponent);rounded=(((bits&0x7fffff)|0x800000)+(1U<<(shift-1)))>>shift;}
 int n=-int(rounded);
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
inline float Scale(float value,int power){
 if(power< -126){value*=PioneerAttentionFloat(unsigned(power+64+127)<<23);return Flush(value*PioneerAttentionFloat(0x1f800000));}
 return Flush(value*PioneerAttentionFloat(unsigned(power+127)<<23));
}
inline float Divide(PioneerAttentionWide numerator,PioneerAttentionWide denominator){
 float quotient=PioneerGraphDividePositive(numerator.high,denominator.high);
 auto product=PioneerAttentionWideMultiply({quotient,0},denominator);
 auto residual=PioneerAttentionWideAdd(numerator,{-product.high,-product.low});
 float error=residual.high+residual.low;unsigned bits=PioneerAttentionBits(error);
 float correction=PioneerGraphDividePositive(PioneerAttentionFloat(bits&0x7fffffff),denominator.high);
 correction=PioneerAttentionFloat(PioneerAttentionBits(correction)|(bits&0x80000000));
 auto refined=PioneerAttentionWideAdd({quotient,0},{correction,0});return refined.high+refined.low;
}
}
// Finite NORMAL-or-zero input; final SIGMOID-result FTZ, matching the old
// gate's tail policy. This does not claim fused subnormal-sigmoid*large-PV
// behavior or universal correctly rounded libm. Actual model parity and
// emitted instructions must be qualified separately before any selection.
inline bool PioneerQ38SigmoidPrecision(float value,float& output){
 using namespace PioneerQ38SigmoidDetail;
 unsigned absolute=PioneerAttentionBits(value)&0x7fffffff;
 if(absolute>=0x7f800000 || (absolute && absolute<0x00800000))return false;
 float result;
 if(absolute<=0x32800000)result=.5f;
 else if(value>=20.f)result=1.f;
 else if(value<=-104.f)result=0.f;
 else{
  float negative=-PioneerAttentionFloat(absolute);int power=0;auto exponential=ExpReduced(negative,power);
  if(value< -20.f){
   float tail=Scale(exponential.high,power);
   auto denominator=PioneerAttentionWideAdd({1,0},{tail,0});
   result=Scale(Divide(exponential,denominator),power);
  }else{
   exponential.high=Scale(exponential.high,power);exponential.low=Scale(exponential.low,power);
   auto denominator=PioneerAttentionWideAdd({1,0},exponential);
   result=Divide(value>=0?PioneerAttentionWide{1,0}:exponential,denominator);
  }
 }
 unsigned bits=PioneerAttentionBits(result);
 if(bits>0x3f800000 || (bits && bits<0x00800000))return false;
 output=result;return true;
}
#endif
