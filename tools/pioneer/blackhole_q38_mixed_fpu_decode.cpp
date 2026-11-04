/* SPDX-License-Identifier: MIT */
// OFFBOARD code-generation probe,not a launchable image/driver/Work mode.
#if !defined(__riscv) || __riscv_xlen!=32
#error Build only with the RV32I offboard compiler
#endif
using uint32_t=unsigned;
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQ38MixedFpuDecode.h"
struct MixedFpuIssue {
	__attribute__((always_inline)) inline void Op(unsigned word) {
#ifdef PIONEER_MIXED_FPU_DECODE_INLINE
		if(__builtin_constant_p(word))asm volatile(".word %0"::"i"((word<<2)|(word>>30)):"memory");
		else *reinterpret_cast<volatile unsigned*>(0xffe40000)=word;
#else
		*reinterpret_cast<volatile unsigned*>(0xffe40000)=word;
#endif
	}
};
extern "C" void pioneer_q6_fpu_prepare(){MixedFpuIssue e;PioneerQ38Q6FpuPrepare(e);}
extern "C" void pioneer_q8_fpu_prepare(){MixedFpuIssue e;PioneerQ38Q8FpuPrepare(e);}
extern "C" void pioneer_q6_fpu_group0(){MixedFpuIssue e;PioneerQ38Q6FpuDecode<0>(e);}
extern "C" void pioneer_q6_fpu_group1(){MixedFpuIssue e;PioneerQ38Q6FpuDecode<1>(e);}
extern "C" void pioneer_q6_fpu_group2(){MixedFpuIssue e;PioneerQ38Q6FpuDecode<2>(e);}
extern "C" void pioneer_q6_fpu_group3(){MixedFpuIssue e;PioneerQ38Q6FpuDecode<3>(e);}
extern "C" void pioneer_q8_fpu_decode(){MixedFpuIssue e;PioneerQ38Q8FpuDecode(e);}
