/* SPDX-License-Identifier: MIT */
// OFFLINE local64-column GDN prerequisite. NOT selected by any driver.
// Fixed L1 input/next/output and owned Dst/CSR only. NO NoC/DRAM/PCI/DMA.
typedef __UINT32_TYPE__ uint32_t;
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQ38DeltaVector.h"
extern "C" void scalar_fault_park() __attribute__((noreturn));
static volatile uint32_t* Words(uint32_t at) { return reinterpret_cast<volatile uint32_t*>(at); }
static inline void Fence() { asm volatile("fence" ::: "memory"); }
static inline void Emit(uint32_t op) { *Words(0xffe40000)=op; }
static void Drain()
{
	volatile uint32_t* ack=Words(0xffe000fc); *ack=0; (void)*ack; Fence();
	Emit(0xa2101816); Emit(0x4500017e);
	for (unsigned attempt=0;attempt<1000000;attempt++) if (*ack) { Fence(); return; }
	scalar_fault_park();
}
static void Config(unsigned index,uint32_t value)
{
	volatile uint32_t* at=Words(0xffef0000+index*4); *at=value;
	if (*at!=value) scalar_fault_park();
}
static bool Finite(uint32_t bits) { return (bits&0x7f800000)!=0x7f800000; }
static bool InputNormalOrZero(uint32_t bits) { return Finite(bits) && (!(bits&0x7fffffff) || (bits&0x7f800000)); }
static void Setup()
{
	Emit(0xb2000000); Drain();
	for (unsigned i=1;i<68;i++) Emit(0xb2000000|(i<<16));
	Emit(0xb2020001); Emit(0xb2030001); Emit(0xb2050000); Drain();
	static const unsigned index[]={0,1,2,3,6,7,220,20,24,28,49,50,56,59,64,65,72,73,76,84,86,112,113,120,121,124};
	static const uint32_t value[]={0x4231,0xa0200000,0,0,0,0,0,0,0xffff,0x100,0,0x100,
		0x100000,0x100,0x100010,16,0,0x13,0x8fff,0x400040,0x100010,0x100011,8,1,3,0x7091};
	for (unsigned i=0;i<sizeof(index)/sizeof(index[0]);i++) Config(index[i],value[i]);
}
static void Import(uint32_t address,unsigned row,unsigned words)
{
	Emit(0xb2050000); Drain();
	Config(76,(address-16)/16); Config(84,0x400000+(row+4)*16);
	Emit(0x5e200000|((words-1)<<10)); Emit(0x5120000a); Emit(0x5420000f);
	Emit(0x42000081); Drain(); // FULL FP32 rows,NO SrcA publication/flip
}
struct DeltaEmitter {
	void Op(uint32_t op) { Emit(op); }
	void Drain() { ::Drain(); }
	uint32_t KeyBits(unsigned i) { return Words(0xb0200)[i]; }
	uint32_t QueryBits(unsigned i) { return Words(0xb0000)[i]; }
};
extern "C" unsigned scalar_main()
{
	const volatile uint32_t* header=Words(0x74000);
	if (header[0]!=0x56383351 || header[1]!=128 || header[2]!=64 || header[3]) return 0;
	uint32_t decay=*Words(0xb0500),beta=*Words(0xb0504);
	if (!PioneerQ38DeltaVectorGate(decay) || !PioneerQ38DeltaVectorGate(beta)) return 0;
	for (unsigned i=0;i<8192;i++) if (!InputNormalOrZero(Words(0x90000)[i])) return 0;
	for (unsigned i=0;i<320;i++) if (!InputNormalOrZero(Words(0xb0000)[i])) return 0;
	Setup(); Import(0xb0400,0,64);
	Emit(0xb2050004); Drain(); Emit(0x8a000002);
	Emit(0x70630000); Emit(0x70730002); Drain(); // preserve ALL64 V values in LRegs
	// SETADCXX's X1 field is TEN bits. Eight1024-datum imports,not a
	// wrapped2048-datum counter,define ALL512 logical FP32 rows.
	for (unsigned chunk=0;chunk<8;chunk++) Import(0x90000+chunk*4096,chunk*64,1024);
	Emit(0xb2050004); Drain();
	DeltaEmitter emitter;
	if (!PioneerQ38DeltaVectorHalf(emitter,decay,beta)) scalar_fault_park();
	// Dst is only a shadow. Publish next state into LOCAL scratch only;
	// a future resident owner must not commit it until independent completion.
	for (unsigned i=0;i<8192;i++) {
		uint32_t bits=Words(0xffbd8000)[i]; if (!Finite(bits)) scalar_fault_park();
		Words(0xa0000)[i]=bits;
	}
	Emit(0x72430000); Emit(0x72530002); Drain(); // state already exported,rows0..3 now output64
	for (unsigned i=0;i<64;i++) {
		uint32_t bits=Words(0xffbd8000)[i]; if (!Finite(bits)) scalar_fault_park(); Words(0xb0800)[i]=bits;
	}
	Fence(); return 64;
}
