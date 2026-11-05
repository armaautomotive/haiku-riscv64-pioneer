/* SPDX-License-Identifier: MIT */
// OFFBOARD image prototype only;no installed handler launches this program.
#include "PioneerQ38NativeBfpMatvec.h"
#ifdef PIONEER_Q38_EXACT_Q4_CORRECTED_IMAGE
#if PIONEER_Q38_EXACT_Q4_CORRECTED_IMAGE != 1 || PIONEER_Q38_NATIVE_BFP_BITS != 4 || defined(PIONEER_Q38_EXACT_Q4_PRIMARY_IMAGE)
#error Distinct corrected integer Q4 image required
#endif
#include "PioneerQ38ExactQ4Corrected.h"
#endif
#ifdef PIONEER_Q38_EXACT_Q4_PRIMARY_IMAGE
#if PIONEER_Q38_EXACT_Q4_PRIMARY_IMAGE != 1 || PIONEER_Q38_NATIVE_BFP_BITS != 4
#error Exact integer-input primary requires its distinct Q4 image
#endif
#include "PioneerQ38ExactQ4Primary.h"
#endif
#if !defined(__riscv) || __riscv_xlen != 32 || !defined(PIONEER_Q38_NATIVE_BFP_BITS)
#error Build the distinct fixed-format RV32 native BFP prototype
#endif
static_assert(PIONEER_Q38_NATIVE_BFP_BITS==4 || PIONEER_Q38_NATIVE_BFP_BITS==8,"fixed BFP format only");
extern "C" void native_bfp_fault() __attribute__((noreturn));
namespace {
static inline volatile unsigned* Words(unsigned address) { return reinterpret_cast<volatile unsigned*>(address); }
struct Emitter {
#ifdef PIONEER_Q38_EXACT_Q4_CORRECTED_IMAGE
	unsigned Mask(unsigned i){return Words(0x90300)[i];}
	void Seven(unsigned i,unsigned v){Words(0x90100)[i]=v;}
	void Escape(unsigned i,unsigned v){Words(0x90200)[i]=v;}
	void SealData(){asm volatile("fence":::"memory");}
#endif
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
#ifdef PIONEER_Q38_EXACT_Q4_CORRECTED_IMAGE
	if(!PioneerQ38ExactQ4Corrected::Setup(emitter))native_bfp_fault();
#elif defined(PIONEER_Q38_EXACT_Q4_PRIMARY_IMAGE)
	if(!PioneerQ38ExactQ4Primary::Setup(emitter))native_bfp_fault();
#else
	if(!PioneerQ38NativeBfpMatvec::Setup(emitter,PIONEER_Q38_NATIVE_BFP_BITS))native_bfp_fault();
#endif
	for(unsigned pass=0;pass<2;pass++) {
#ifdef PIONEER_Q38_EXACT_Q4_CORRECTED_IMAGE
		if(!PioneerQ38ExactQ4Corrected::Run(emitter))native_bfp_fault();
		constexpr unsigned rows=1;
#elif defined(PIONEER_Q38_EXACT_Q4_PRIMARY_IMAGE)
		if(!PioneerQ38ExactQ4Primary::Run(emitter))native_bfp_fault();
		constexpr unsigned rows=1;
#else
		if(!PioneerQ38NativeBfpMatvec::Run(emitter,PIONEER_Q38_NATIVE_BFP_BITS))native_bfp_fault();
		constexpr unsigned rows=2;
#endif
		// Both result copies consume actual FPU Dst after real completion.
		// No scalar multiplication,dequantization or CPU reference exists.
		for(unsigned row=0;row<rows;row++)for(unsigned column=0;column<16;column++)
			Words(0x92000+pass*rows*64)[row*16+column]=Words(0xffbd8000+(row?40:32)*64)[column];
		asm volatile("fence":::"memory");
	}
	return 2;
}
