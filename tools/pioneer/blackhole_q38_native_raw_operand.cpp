/* SPDX-License-Identifier: MIT */
// OFFBOARD code-generation probe ONLY: no boot/owner/Dst/unpack/main program.
#if !defined(__riscv) || __riscv_xlen != 32
#error Build only with the RV32I offboard compiler
#endif
using uint32_t=unsigned;
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQ38NativeRawOperand.h"
struct NativeRawIssue {
	__attribute__((always_inline)) inline void Op(unsigned word) {
#ifdef PIONEER_NATIVE_RAW_INLINE
		if(__builtin_constant_p(word))asm volatile(".word %0"::"i"((word<<2)|(word>>30)):"memory");
		else *reinterpret_cast<volatile unsigned*>(0xffe40000)=word;
#else
		*reinterpret_cast<volatile unsigned*>(0xffe40000)=word;
#endif
	}
};
extern "C" void pioneer_native_raw2(){NativeRawIssue issue;PioneerQ38NativeRawRecover(issue,2);}
extern "C" void pioneer_native_raw4(){NativeRawIssue issue;PioneerQ38NativeRawRecover(issue,4);}
extern "C" void pioneer_native_raw8(){NativeRawIssue issue;PioneerQ38NativeRawRecover(issue,8);}
