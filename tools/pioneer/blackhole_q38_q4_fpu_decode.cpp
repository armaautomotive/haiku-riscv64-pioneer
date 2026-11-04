/* SPDX-License-Identifier: MIT */
// OFFBOARD code-generation probe. Not a launchable image/driver/Work mode.
#if !defined(__riscv) || __riscv_xlen != 32
#error Build only with the RV32I offboard compiler
#endif
using uint32_t=unsigned;
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQ38Q4FpuDecode.h"
struct Q4FpuIssue {
	__attribute__((always_inline)) inline void Op(unsigned word) {
#ifdef PIONEER_Q4_FPU_DECODE_INLINE
		if(__builtin_constant_p(word))asm volatile(".word %0"::"i"((word<<2)|(word>>30)):"memory");
		else *reinterpret_cast<volatile unsigned*>(0xffe40000)=word;
#else
		*reinterpret_cast<volatile unsigned*>(0xffe40000)=word;
#endif
	}
};
extern "C" void pioneer_q4_fpu_prepare(){Q4FpuIssue issue;PioneerQ38Q4FpuPrepare(issue);}
extern "C" void pioneer_q4_fpu_low(){Q4FpuIssue issue;PioneerQ38Q4FpuDecode<Q4FpuIssue,false>(issue);}
extern "C" void pioneer_q4_fpu_high(){Q4FpuIssue issue;PioneerQ38Q4FpuDecode<Q4FpuIssue,true>(issue);}
