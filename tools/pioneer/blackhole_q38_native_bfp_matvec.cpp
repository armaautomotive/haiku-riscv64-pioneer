/* SPDX-License-Identifier: MIT */
// OFFBOARD image prototype only;no installed handler launches this program.
#include "PioneerQ38NativeBfpMatvec.h"
#if !defined(__riscv) || __riscv_xlen != 32 || !defined(PIONEER_Q38_NATIVE_BFP_BITS)
#error Build the distinct fixed-format RV32 native BFP prototype
#endif
static_assert(PIONEER_Q38_NATIVE_BFP_BITS==4 || PIONEER_Q38_NATIVE_BFP_BITS==8,"fixed BFP format only");
extern "C" void native_bfp_fault() __attribute__((noreturn));
namespace {
static inline volatile unsigned* Words(unsigned address) { return reinterpret_cast<volatile unsigned*>(address); }
struct Emitter {
	void Op(unsigned instruction) { *Words(0xffe40000)=instruction; }
	void Drain() {
		// Existing bounded Blackhole STALLWAIT/GPR completion protocol. No guessed delay,
		// skipped wait or substitution of a source-ready flag for completion.
		*Words(0xffe000fc)=0;(void)*Words(0xffe000fc);asm volatile("fence":::"memory");
		Op(0xa2101816);Op(0x4500017e);
		for(unsigned attempt=0;attempt<1000000;attempt++)if(*Words(0xffe000fc)) {
			asm volatile("fence":::"memory");return;
		}
		native_bfp_fault(); // uncertain resources:park,no cleanup or retry
	}
	void Config(unsigned index,unsigned value) {
		volatile unsigned* at=Words(0xffef0000+index*4);*at=value;
		if(*at!=value)native_bfp_fault();
	}
};
}
extern "C" unsigned native_bfp_main() {
	Emitter emitter;
	if(!PioneerQ38NativeBfpMatvec::Setup(emitter,PIONEER_Q38_NATIVE_BFP_BITS))native_bfp_fault();
	for(unsigned pass=0;pass<2;pass++) {
		if(!PioneerQ38NativeBfpMatvec::Run(emitter,PIONEER_Q38_NATIVE_BFP_BITS))native_bfp_fault();
		// BOTH32-output copies consume actual FPU Dst after real completion.
		// No scalar multiplication,dequantization or CPU reference exists.
		for(unsigned row=0;row<2;row++)for(unsigned column=0;column<16;column++)
			Words(0x92000+pass*128)[row*16+column]=Words(0xffbd8000+(row?40:32)*64)[column];
		asm volatile("fence":::"memory");
	}
	return 2;
}
