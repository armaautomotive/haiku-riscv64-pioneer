/* SPDX-License-Identifier: MIT */
// Code-generation functions ONLY,not a bootable/returning service or driver.
#if !defined(__riscv) || __riscv_xlen != 32 || !defined(PIONEER_Q38_Q4_FACTORED_BFP8_BUILD) || PIONEER_Q38_Q4_FACTORED_BFP8_BUILD != 1
#error Exact distinct factored-Q4 RV32 BUILD-only selection required
#endif
using uint32_t=unsigned;
#define PIONEER_ATTENTION_FREESTANDING
#define PIONEER_Q38_Q4_FACTORED_BFP8_OFFBOARD 1
#include "PioneerQ38Q4FactoredBfp8.h"
struct FactoredIssue {
 __attribute__((always_inline)) inline void Op(unsigned word){
#ifdef PIONEER_Q38_Q4_FACTORED_BFP8_INLINE
  if(__builtin_constant_p(word))asm volatile(".word %0"::"i"((word<<2)|(word>>30)):"memory");
  else *reinterpret_cast<volatile unsigned*>(0xffe40000)=word;
#else
  *reinterpret_cast<volatile unsigned*>(0xffe40000)=word;
#endif
 }
};
extern "C" void pioneer_q4_factored_init(){FactoredIssue e;PioneerQ38Q4FactoredBfp8::InitCodes(e);}
extern "C" void pioneer_q4_factored_low(){FactoredIssue e;PioneerQ38Q4FactoredBfp8::Codes<false>(e);}
extern "C" void pioneer_q4_factored_high(){FactoredIssue e;PioneerQ38Q4FactoredBfp8::Codes<true>(e);}
extern "C" void pioneer_q4_factored_affine(){FactoredIssue e;PioneerQ38Q4FactoredBfp8::GroupAffine(e);}
