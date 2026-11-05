/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_Q4_BLOCK_INTEGER_AFFINE_OFFBOARD) \
 || PIONEER_Q38_Q4_BLOCK_INTEGER_AFFINE_OFFBOARD != 1
#error Separate offboard block-integer arithmetic selection required
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_ATTENTION_FREESTANDING) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_QUEUED_Q4_BLOCK_IMAGE) || defined(__FAST_MATH__)
#error Block-integer arithmetic has no native,client,Work or validation integration
#endif
#ifndef PIONEER_Q38_Q4_BLOCK_INTEGER_AFFINE_H
#define PIONEER_Q38_Q4_BLOCK_INTEGER_AFFINE_H
#include <stdint.h>
#include "PioneerQ38VectorAffine.h"

// New rounding contract,NOT the old K16 affine or GGML's accumulation order.
// Every lane starts with +0 in4..7. Keep independent parity accumulators:
// dot4/min5 and dot6/min7. Save BOTH dot halves after8 panels,then clear
// ONLY4/6 before the final8. Each half is an exact integer below2^24.
// Minima accumulate all16 panels exactly. Caller owes all integer domains,
// original metadata,loads,half storage,Dst/register ownership and lifetime.
struct PioneerQ38Q4BlockIntegerAffine {
 enum : unsigned {Panels=16,HalfPanels=8,DotLimit=30720,SumLimit=2048,
  FactorLimit=63,PartialDotLimit=15482880,MinimumLimit=2064384};
 static bool PanelDomain(int dot,int sum,unsigned scale,unsigned minimum){
  return dot>=-int(DotLimit) && dot<=int(DotLimit)
   && sum>=-int(SumLimit) && sum<=int(SumLimit)
   && scale<=FactorLimit && minimum<=FactorLimit;
 }
 // Input0=EXACT FP32 original q dot,1/2=positive FP32 scale/minimum factors,
 // 3=EXACT FP32 input sum. No HALF/BF16 conversion or coefficient products.
 // The two MADs are EXACT integers in the stated split-half domain,despite
 // the general SFPMAD being only partially fused. Preserve0..3/other parity.
 template<unsigned Parity,class E> static void Panel(E& e){
  static_assert(Parity==0 || Parity==2,"two original output parities only");
  constexpr unsigned dot=4+Parity,minimum=5+Parity;
  e.Op(0x84001000|(dot<<8)|(dot<<4));
  e.Op(0x84023000|(minimum<<8)|(minimum<<4));e.Op(0x8f000000);
 }
 // Input0=RN(exact first-half+second-half weighted-dot integers),1=original
 // d HALF converted EXACTLY to FP32,2=NEGATED original dmin FP32,3=activation
 // scale,4=old normal-or-zero accumulator. Min sum remains in5 or7. This
 // non-fused final affine preserves6/7 for finishing the other parity next.
 template<unsigned Parity,class E> static void Finish(E& e){
  static_assert(Parity==0 || Parity==2,"two original output parities only");
  PioneerQ38VectorAffineMul(e,1,3,1);
  PioneerQ38VectorAffineMul(e,2,3,2);
  PioneerQ38VectorAffineMul(e,1,0,0);
  PioneerQ38VectorAffineMul(e,2,5+Parity,5);
  PioneerQ38VectorAffineAdd(e,0,5,0);
  PioneerQ38VectorAffineAdd(e,4,0,0);
 }
};
#endif
