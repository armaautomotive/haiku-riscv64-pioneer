/* SPDX-License-Identifier: MIT */
// Distinct OFFBOARD full-K image, not accepted by any installed owner.
#include "PioneerQ38NativeBfpBand.h"
#if !defined(__riscv) || __riscv_xlen != 32 || !defined(PIONEER_NATIVE_BFP_BAND_K) || !defined(PIONEER_NATIVE_BFP_BAND_PHASES)
#error Select a fixed RV32I full-K/fidelity native prototype
#endif
static_assert(PioneerQ38NativeBfpBand::Width(PIONEER_NATIVE_BFP_BAND_K),"exact Qwen width only");
extern "C" void native_bfp_band_fault() __attribute__((noreturn));
namespace {
static inline volatile unsigned* Words(unsigned address){return reinterpret_cast<volatile unsigned*>(address);}
struct Emitter {
	__attribute__((always_inline)) inline void Op(unsigned word) {
#ifdef PIONEER_NATIVE_BFP_BAND_DIRECT
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
		native_bfp_band_fault(); // uncertain resources:park;no next tile/read/reset/cleanup
	}
	void Config(unsigned index,unsigned value) {
		volatile unsigned* at=Words(0xffef0000+index*4);*at=value;
		if(*at!=value)native_bfp_band_fault();
	}
};
}
extern "C" unsigned native_bfp_band_main() {
	Emitter emitter;
	for(unsigned pass=0;pass<2;pass++) {
		if(!PioneerQ38NativeBfpBand::Run<PIONEER_NATIVE_BFP_BAND_PHASES>(emitter,PIONEER_NATIVE_BFP_BAND_K))native_bfp_band_fault();
		for(unsigned row=0;row<2;row++)for(unsigned column=0;column<16;column++)
			Words(PioneerQ38NativeBfpBand::Output+pass*128)[row*16+column]=Words(0xffbd8000+(row?40:32)*64)[column];
		asm volatile("fence":::"memory");
	}
	return (PIONEER_NATIVE_BFP_BAND_PHASES<<24)|PIONEER_NATIVE_BFP_BAND_K;
}
