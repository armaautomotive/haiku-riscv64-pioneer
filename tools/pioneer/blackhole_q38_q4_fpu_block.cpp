/* SPDX-License-Identifier: MIT */
// NEW OFFBOARD complete K256 pipeline,not selected by any native owner.
#if !defined(__riscv) || __riscv_xlen != 32
#error Build only with the RV32I compiler
#endif
using uint32_t=unsigned;
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQ38Q4FpuBlock.h"
extern "C" void q4_fpu_block_fault() __attribute__((noreturn));
namespace {
static inline volatile unsigned* Words(unsigned a){return reinterpret_cast<volatile unsigned*>(a);}
struct Emitter {
	__attribute__((always_inline)) inline void Op(unsigned word) {
#ifdef PIONEER_Q4_FPU_BLOCK_DIRECT
		if(__builtin_constant_p(word))asm volatile(".word %0"::"i"((word<<2)|(word>>30)):"memory");
		else *Words(0xffe40000)=word;
#else
		*Words(0xffe40000)=word;
#endif
	}
	void Drain() {
		*Words(0xffe000fc)=0;(void)*Words(0xffe000fc);asm volatile("fence":::"memory");
		Op(0xa2101816);Op(0x4500017e);
		for(unsigned attempt=0;attempt<1000000;attempt++)if(*Words(0xffe000fc)){asm volatile("fence":::"memory");return;}
		q4_fpu_block_fault(); // unknown resources:park,no next group/read/retry/reset
	}
	void Config(unsigned index,unsigned value) {
		volatile unsigned* at=Words(0xffef0000+index*4);*at=value;
		if(*at!=value)q4_fpu_block_fault();
	}
};
}
extern "C" unsigned q4_fpu_block_main()
{
	Emitter emitter;
	for(unsigned pass=0;pass<2;pass++) {
		PioneerQ38Q4FpuBlock::Run(emitter);
		for(unsigned row=0;row<2;row++)for(unsigned column=0;column<16;column++)
			Words(PioneerQ38Q4FpuBlock::Output+pass*128)[row*16+column]=Words(0xffbd8000+(row?40:32)*64)[column];
		asm volatile("fence":::"memory");
	}
	return 0x04000100; // full4-phase,K256;NOT a full-model/Work completion
}
