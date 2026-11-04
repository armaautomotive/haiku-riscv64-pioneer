/* SPDX-License-Identifier: MIT */
// Explicit LOAD ONLY. This image has NO FPU/unpack/config/output math path.
#define PIONEER_DRAM_SWEEP_FREESTANDING 1
#include "PioneerNativeDramBandLoader.h"
#if !defined(__riscv) || __riscv_xlen != 32 || !defined(PIONEER_NATIVE_DRAM_LOAD_ONLY) || PIONEER_NATIVE_DRAM_LOAD_ONLY != 1
#error Build ONLY the fixed RV32 load-only diagnostic82 image
#endif
extern "C" void native_dram_load_fault() __attribute__((noreturn));
namespace {
static volatile unsigned* Words(unsigned address){return reinterpret_cast<volatile unsigned*>(address);}
struct IO {
	unsigned Read(unsigned address){return *Words(address);}
	void Write(unsigned address,unsigned value){*Words(address)=value;}
	void Fence(){asm volatile("fence":::"memory");}
};
}
extern "C" unsigned native_dram_load_main() {
	unsigned frame[8];for(unsigned i=0;i<8;i++)frame[i]=Words(0x74000)[i];
	if(frame[0]!=PioneerNativeDramLoadVariant::Marker || frame[1]!=1 || frame[2]!=5120 || !PioneerNativeDramBandLoader::Route(frame[3])
		|| frame[4]!=1 || frame[5] || frame[6] || frame[7])native_dram_load_fault();
	IO io;PioneerNativeDramBandLoader loader;
	if(!loader.Run(io,5120,frame[3]))native_dram_load_fault();
	for(unsigned i=0;i<8;i++)if(Words(0x74000)[i]!=frame[i])native_dram_load_fault();
	// Do not read/copy/hash payloads here. Native END must independently
	// compare ALL actually loaded payload/header bytes before restoring them.
	return PioneerNativeDramLoadVariant::Count;
}
