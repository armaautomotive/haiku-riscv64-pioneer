/* SPDX-License-Identifier: MIT */
// OFFBOARD ONLY. New image/layout,NO existing native owner/driver accepts it.
#define PIONEER_DRAM_SWEEP_FREESTANDING 1
#include "PioneerNativeDramBandLoader.h"
#include "PioneerNativeDramBandCompute.h"
#if !defined(__riscv) || __riscv_xlen != 32 || !defined(PIONEER_NATIVE_DRAM_BAND_K)
#error Select a fixed RV32I direct-GDDR native band prototype
#endif
static_assert(PioneerNativeDramBandLayout::Width(PIONEER_NATIVE_DRAM_BAND_K),"fixed Qwen width only");
extern "C" void native_dram_band_fault() __attribute__((noreturn));
namespace {
static volatile unsigned* Words(unsigned address){return reinterpret_cast<volatile unsigned*>(address);}
struct IO {
	unsigned Read(unsigned address){return *Words(address);}
	void Write(unsigned address,unsigned value){*Words(address)=value;}
	void Fence(){asm volatile("fence":::"memory");}
};
struct Emitter {
	void Op(unsigned word){*Words(0xffe40000)=word;}
	void Drain() {
		*Words(0xffe000fc)=0;(void)*Words(0xffe000fc);asm volatile("fence":::"memory");
		Op(0xa2101816);Op(0x4500017e);
		for(unsigned attempt=0;attempt<1000000;attempt++)if(*Words(0xffe000fc)){asm volatile("fence":::"memory");return;}
		native_dram_band_fault();
	}
	void Config(unsigned index,unsigned value) {
		volatile unsigned* at=Words(0xffef0000+index*4);*at=value;
		if(*at!=value)native_dram_band_fault();
	}
};
}
extern "C" unsigned native_dram_band_main() {
	// Immutable owned32B diagnostic frame. No arbitrary source addresses.
	unsigned frame[8];for(unsigned i=0;i<8;i++)frame[i]=Words(0x74000)[i];
	if(frame[0]!=0x3142444e || frame[1]!=1 || frame[2]!=PIONEER_NATIVE_DRAM_BAND_K
		|| !PioneerNativeDramBandLoader::Route(frame[3]) || !frame[4] || frame[5] || frame[6] || frame[7])native_dram_band_fault();
	IO io;PioneerNativeDramBandLoader loader;
	if(!loader.Run(io,PIONEER_NATIVE_DRAM_BAND_K,frame[3]))native_dram_band_fault();
	for(unsigned i=0;i<8;i++)if(Words(0x74000)[i]!=frame[i])native_dram_band_fault();
	// One device-side packed load serves BOTH complete FPU jobs. No host
	// round trip,payload scalar copy,reconversion or second GDDR load.
	for(unsigned pass=0;pass<2;pass++) {
		Emitter emitter;
		if(!PioneerNativeDramBandCompute::Run(emitter,PIONEER_NATIVE_DRAM_BAND_K))native_dram_band_fault();
		for(unsigned row=0;row<2;row++)for(unsigned column=0;column<16;column++)
			Words(PioneerNativeDramBandLayout::Output+pass*128)[row*16+column]=Words(0xffbd8000+(row?40:32)*64)[column];
		asm volatile("fence":::"memory");
	}
	for(unsigned i=0;i<8;i++)if(Words(0x74000)[i]!=frame[i])native_dram_band_fault();
	return 0x04000000|PIONEER_NATIVE_DRAM_BAND_K;
}
