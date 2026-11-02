/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_WIDE_GDN_RETURNING_IMAGE
#include "PioneerQ38WideGdnReturningImageRole.h"
#endif
#if (defined(PIONEER_Q38_WIDE_DELTA_LOCAL_IMAGE) + defined(PIONEER_Q38_WIDE_DELTA_INSTRUCTION_TEST)) != 1 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(__FAST_MATH__)
#error Separate offline wide-delta instruction image or TEST only; no native/CPU oracle/Work
#endif
#if defined(PIONEER_Q38_WIDE_DELTA_LOCAL_IMAGE) && PIONEER_Q38_WIDE_DELTA_LOCAL_IMAGE != 1
#error Exact wide-delta local-image selector required
#endif
#if defined(PIONEER_Q38_WIDE_DELTA_INSTRUCTION_TEST) && PIONEER_Q38_WIDE_DELTA_INSTRUCTION_TEST != 1
#error Exact wide-delta instruction-TEST selector required
#endif
#ifndef PIONEER_Q38_WIDE_DELTA_MATH_H
#define PIONEER_Q38_WIDE_DELTA_MATH_H
#include "PioneerQ38VectorAffine.h"
#include "PioneerSfpuDivide.h"
// OFFLINE instruction candidate. No driver, allocation, source, Dst row,
// import, publication or timing authority. All32 lanes must be enabled.
// LRegs6/7 hold a normalized high/low sum. Product0*1 becomes2/3, using
// ONLY0..7. SFPMAD is partially fused:split BOTH operands before obtaining
// the residual. A plain MAD(a,b,-roundedProduct) is NOT sufficient.
// Every MAD/ADD/MUL has an explicit latency gap, including before SFPAND
// USE_VB (whose automatic dependency detection has a documented bug).
template<class E> static inline void PioneerQ38WideDeltaSub(E& e,unsigned a,unsigned b,unsigned d)
{
 e.Op(0x850a0002|(a<<12)|(b<<8)|(d<<4));e.Op(0x8f000000);
}
template<class E> static inline void PioneerQ38WideDeltaMad(E& e,unsigned a,unsigned b,unsigned c,unsigned d,bool negativeC=false)
{
 e.Op(0x84000000|(a<<16)|(b<<12)|(c<<8)|(d<<4)|(negativeC?2:0));e.Op(0x8f000000);
}
template<class E> static void PioneerQ38WideDeltaProduct(E& e)
{
 PioneerSfpuLoad32(e,3,0xfffff000U);
 PioneerSfpuAnd(e,4,0,3);PioneerSfpuAnd(e,5,1,3);
 PioneerQ38VectorAffineMul(e,0,1,2);
 PioneerQ38WideDeltaSub(e,0,4,0);PioneerQ38WideDeltaSub(e,1,5,1);
 PioneerQ38WideDeltaMad(e,4,5,2,3,true);
 PioneerQ38WideDeltaMad(e,0,5,3,3);
 PioneerQ38WideDeltaMad(e,4,1,3,3);
 PioneerQ38WideDeltaMad(e,0,1,3,3);
}
template<class E> static void PioneerQ38WideDeltaAccumulate(E& e)
{
 // TwoSum((6,7),(2,3)),then normalize.0,1,4,5 are scratch.
 PioneerQ38VectorAffineAdd(e,6,2,0);
 PioneerQ38WideDeltaSub(e,0,6,1);
 PioneerQ38WideDeltaSub(e,0,1,4);
 PioneerQ38WideDeltaSub(e,6,4,4);
 PioneerQ38WideDeltaSub(e,2,1,5);
 PioneerQ38VectorAffineAdd(e,4,5,4);
 PioneerQ38VectorAffineAdd(e,7,3,5);
 PioneerQ38VectorAffineAdd(e,4,5,3);
 PioneerQ38VectorAffineAdd(e,0,3,6);
 PioneerQ38WideDeltaSub(e,6,0,1);
 PioneerQ38WideDeltaSub(e,6,1,4);
 PioneerQ38WideDeltaSub(e,0,4,4);
 PioneerQ38WideDeltaSub(e,3,1,5);
 PioneerQ38VectorAffineAdd(e,4,5,7);
}
#endif
