/* SPDX-License-Identifier: MIT */
// Local mixed-quant unpack + integer Tensix-dot PROBE,not a model runner.
// Integer-only C++ helper called by the distinct fixed TRISC0 image. No
// code-selected pointers/NoC/DRAM/DMA/PCI/reset or host calls.
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQwen38Panel.h"
extern "C" unsigned q38_local_panel()
{
	const volatile uint32_t* matrix=reinterpret_cast<const volatile uint32_t*>(0x70500);
	const volatile uint32_t* job=reinterpret_cast<const volatile uint32_t*>(0x74000);
	if (matrix[0] || matrix[1]!=1 || matrix[2]!=1 || job[0]!=0x50383355) return 0;
	return PioneerQ38UnpackPanel(job[1],reinterpret_cast<const uint8_t*>(0x90000),job[2],job[3],
		reinterpret_cast<PioneerQ38Panel*>(0x91000))?1:0;
}
#ifdef PIONEER_Q38_PANEL_AFFINE
// Separate OFFLINE candidate. Matrix dots remain Tensix; this bounded
// on-card epilogue applies the exact affine weight scales/minima and
// activation scale. NO host intermediate or extra NoC/PCIe round trip.
// Does NOT accumulate all K panels, run a graph, or authorize a native launch.
static inline float PanelFloat(uint32_t bits)
{
	float f; __builtin_memcpy(&f,&bits,4); return f;
}
static inline uint32_t PanelBits(float f)
{
	uint32_t bits; __builtin_memcpy(&bits,&f,4); return bits;
}
extern "C" unsigned q38_local_affine()
{
	const volatile uint32_t* job=reinterpret_cast<const volatile uint32_t*>(0x74000);
	uint32_t bits=job[4],magnitude=bits&0x7fffffff;
	// Explicit initial probe range prevents scalar denormal flush/overflow;
	// this is NOT silently imposed as a general model activation policy.
	if (magnitude && (magnitude<0x2f800000 || magnitude>0x47800000)) return 0; //2^-32..2^16
	float scale=PanelFloat(bits);
	const volatile uint32_t* input=reinterpret_cast<const volatile uint32_t*>(0x80000);
	int sum=0;
	for (unsigned w=0;w<8;w++) {
		uint32_t operands=input[w];
		for (unsigned lane=0;lane<2;lane++) {
			uint16_t operand=uint16_t(operands>>(lane*16));
			if (!PioneerInt8OperandValid(operand)) return 0;
			sum+=int(operand&1023)*((operand&0x8000)?-1:1);
		}
	}
	volatile uint32_t* dot=reinterpret_cast<volatile uint32_t*>(0x84000);
	const volatile uint32_t* coefficient=reinterpret_cast<const volatile uint32_t*>(0x91200);
	const volatile uint32_t* minimum=reinterpret_cast<const volatile uint32_t*>(0x91240);
	for (unsigned row=0;row<16;row++) {
		static_assert(sizeof(int)==4,"fixed signed32 dot words");
		int value=int(dot[row]);
		if (value<-262144 || value>262144 || (coefficient[row]&0x7f800000)==0x7f800000 || (minimum[row]&0x7f800000)==0x7f800000) return 0;
		float product=(PanelFloat(coefficient[row])*scale)*float(value);
		float correction=(PanelFloat(minimum[row])*scale)*float(sum);
		uint32_t result=PanelBits(product+correction);
		if ((result&0x7f800000)==0x7f800000) return 0;
		dot[row]=result;
	}
	asm volatile("fence" ::: "memory"); return 1;
}
#endif
