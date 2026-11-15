/* SPDX-License-Identifier: MIT */
// OFFLINE local64-column GDN prerequisite. NOT selected by any driver.
// Default: fixed L1 input/next/output and owned Dst/CSR only. NO NoC/DRAM/PCI/DMA.
// Scattered-state selector is an isolated fixture; it adds no runtime owner.
typedef __UINT32_TYPE__ uint32_t;
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQ38DeltaVector.h"
#include "PioneerQ38DeltaVectorLayout.h"
namespace L=PioneerQ38DeltaVectorLayout;
extern "C" void scalar_fault_park() __attribute__((noreturn));
static volatile uint32_t* Words(uint32_t at) { return reinterpret_cast<volatile uint32_t*>(at); }
#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
static bool StatePages(uint32_t (&pages)[L::StatePages])
{
	for (unsigned i=0;i<L::StatePages;i++) {
		pages[i]=Words(L::StatePageTable)[i];
		// Keep this worker fixture's page table and fixed arithmetic buffers
		// outside the candidate state map. The native owner must independently
		// derive its actual free-page map; this is only an image-side bound.
		if ((pages[i]&(L::StatePageBytes-1)) || pages[i]<0x90000
			|| pages[i]>L::L1Bytes-L::StatePageBytes
			|| (pages[i]<uint32_t(L::StatePageTable)+L::StatePages*4
				&& uint32_t(L::StatePageTable)<pages[i]+L::StatePageBytes)
			|| (pages[i]<uint32_t(L::Query)+1280 && uint32_t(L::Query)<pages[i]+L::StatePageBytes)
			|| (pages[i]<uint32_t(L::Key)+512 && uint32_t(L::Key)<pages[i]+L::StatePageBytes)
			|| (pages[i]<uint32_t(L::Value)+256 && uint32_t(L::Value)<pages[i]+L::StatePageBytes)
			|| (pages[i]<uint32_t(L::Output)+256 && uint32_t(L::Output)<pages[i]+L::StatePageBytes)) return false;
		for (unsigned j=0;j<i;j++) if (pages[i]==pages[j]) return false;
	}
	return true;
}
#endif
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
	uint32_t KeyBits(unsigned i) { return Words(L::Key)[i]; }
	uint32_t QueryBits(unsigned i) { return Words(L::Query)[i]; }
};
extern "C" unsigned scalar_main()
{
	const volatile uint32_t* header=Words(L::Descriptor);
	if (header[0]!=0x56383351 || header[1]!=128 || header[2]!=64 || header[3]) return 0;
#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
	uint32_t statePages[L::StatePages];
	if (!StatePages(statePages)) return 0;
#endif
	uint32_t decay=*Words(L::Decay),beta=*Words(L::Beta);
	if (!PioneerQ38DeltaVectorGate(decay) || !PioneerQ38DeltaVectorGate(beta)) return 0;
#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
	for (unsigned page=0;page<L::StatePages;page++)
		for (unsigned i=0;i<L::StatePageBytes/4;i++)
			if (!InputNormalOrZero(Words(statePages[page])[i])) return 0;
#else
	for (unsigned i=0;i<8192;i++) if (!InputNormalOrZero(Words(L::Prior)[i])) return 0;
#endif
	for (unsigned i=0;i<320;i++) if (!InputNormalOrZero(Words(L::Query)[i])) return 0;
	Setup(); Import(L::Value,0,64);
	Emit(0xb2050004); Drain(); Emit(0x8a000002);
	Emit(0x70630000); Emit(0x70730002); Drain(); // preserve ALL64 V values in LRegs
	// SETADCXX's X1 field is TEN bits. Eight1024-datum imports,not a
	// wrapped2048-datum counter,define ALL512 logical FP32 rows.
	for (unsigned chunk=0;chunk<8;chunk++) Import(
#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
		statePages[chunk],
#else
		L::Prior+chunk*4096,
#endif
		chunk*64,1024);
	Emit(0xb2050004); Drain();
	DeltaEmitter emitter;
#ifdef PIONEER_Q38_DELTA_VECTOR_FACTORED_DECAY_IMAGE
	if (!PioneerQ38DeltaVectorHalfFactoredDecay<
#ifdef PIONEER_Q38_DELTA_VECTOR_POST_SCALE
		true
#else
		false
#endif
		>(emitter,decay,beta)) scalar_fault_park();
#else
	if (!PioneerQ38DeltaVectorHalf<
#ifdef PIONEER_Q38_DELTA_VECTOR_POST_SCALE
		true
#else
		false
#endif
		>(emitter,decay,beta)) scalar_fault_park();
#endif
	// Dst is only a shadow. Publish next state into LOCAL scratch only;
	// a future resident owner must not commit it until independent completion.
#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
	for (unsigned page=0;page<L::StatePages;page++) {
		volatile uint32_t* destination=Words(statePages[page]);
		const unsigned first=page*(L::StatePageBytes/4);
		for (unsigned word=0;word<L::StatePageBytes/4;word++) {
			uint32_t bits=Words(0xffbd8000)[first+word]; if (!Finite(bits)) scalar_fault_park();
		destination[word]=bits;
		}
	}
#else
	for (unsigned i=0;i<8192;i++) {
		uint32_t bits=Words(0xffbd8000)[i]; if (!Finite(bits)) scalar_fault_park();
		Words(L::Next)[i]=bits;
	}
#endif
	Emit(0x72430000); Emit(0x72530002); Drain(); // state already exported,rows0..3 now output64
	for (unsigned i=0;i<64;i++) {
		uint32_t bits=Words(0xffbd8000)[i]; if (!Finite(bits)) scalar_fault_park(); Words(L::Output)[i]=bits;
	}
	Fence(); return 64;
}
