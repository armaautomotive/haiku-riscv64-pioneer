/* SPDX-License-Identifier: MIT */
// Selection checks deliberately outside the include guard.
#if !defined(PIONEER_Q38_Q4_FACTORED_BFP8_OFFBOARD) || PIONEER_Q38_Q4_FACTORED_BFP8_OFFBOARD != 1
#error Separate factored-Q4 offboard arithmetic selection required
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(TT_PIONEER_DRIVER_RELOAD) || defined(__FAST_MATH__)
#error Factored-Q4 arithmetic has NO native,Work,client,reference or driver integration
#endif
#if defined(PIONEER_Q38_Q4_FACTORED_BFP8_BUILD) && (PIONEER_Q38_Q4_FACTORED_BFP8_BUILD != 1 || !defined(__riscv) || __riscv_xlen != 32)
#error Factored-Q4 BUILD selection requires exact RV32 role
#endif
#if defined(PIONEER_ATTENTION_FREESTANDING) && (!defined(PIONEER_Q38_Q4_FACTORED_BFP8_BUILD) || PIONEER_Q38_Q4_FACTORED_BFP8_BUILD != 1 || !defined(__riscv) || __riscv_xlen != 32)
#error Freestanding factored-Q4 requires its distinct RV32 BUILD-only role
#endif
#ifndef PIONEER_Q38_Q4_FACTORED_BFP8_H
#define PIONEER_Q38_Q4_FACTORED_BFP8_H
#include "PioneerSfpuDivide.h"

// NEW arithmetic order,NOT the old per-weight BF16-RNE recipe or GGML's
// accumulation order. Keep the ORIGINAL unsigned q in0..15. A BFP8_B tile
// with shared exponent130 and positive magnitude(q<<3) represents q EXACTLY.
// Matrix(q,x) followed by coefficient*qdot + negativeMinimum*sum(x) avoids
// converting each weight to float,scaling,subtracting and rounding it first.
// It requires genuine BF16-input sums and a genuine full-fidelity matrix dot.
// This file provides ONLY code-byte extraction and output affine arithmetic.
// No PACK/L1 native tile publication,source banks,NoC,owner,layout,journal,
// root integration or model-quality/speed authority is established here.
// In particular it does NOT allocate an expanded global weight model.
struct PioneerQ38Q4FactoredBfp8 {
 enum : unsigned {SharedExponent=130,CodeMask=0x78,GroupWidth=32};
 template<class E> static void InitCodes(E& e){PioneerSfpuLoad32(e,7,CodeMask);}
 // Entry:0=original unsigned packed BYTE,7=0x78,ALL32 lanes enabled.
 // Exit:0=positive BFP8 magnitude byte q*8;preserves1..7 and lane flags.
 // No FP32 cast,MAD or BF16 rounding. Still owes a REAL pack into the
 // native byte plane and exponent section before any FPU consumes it.
 template<bool Upper,class E> static void Codes(E& e){
  PioneerSfpuShift(e,0,Upper?-1:3);
  PioneerSfpuAnd(e,0,0,7);
 }
 // Entry:0=genuine qdot,1=genuine sum(x),2=exact d*scale,
 // 3=NEGATED exact dmin*minimum (including its signed zero),4=old FP32 acc.
 // Exit:4=old + ((2*0)+(3*1)),with SEPARATE FP32 rounding points.
 // Preserve0..3 and7;clobber5/6. The independent products cover one
 // another's first latency slot. Explicit NOPs also protect later integer
 // consumers from Blackhole's incomplete automatic dependency detection.
 // Native FP32 FTZ/zero behavior remains;caller owes finite/range checks.
 template<class E> static void GroupAffine(E& e){
  e.Op(0x86020952); // 5=2*0,negative-zero addend preserves product zeros
  e.Op(0x86031962); // 6=3*1
  e.Op(0x8f000000);
  e.Op(0x850a5650); // 5=5+6
  e.Op(0x8f000000);
  e.Op(0x850a4540); // 4=4+5
  e.Op(0x8f000000);
 }
};
#endif
