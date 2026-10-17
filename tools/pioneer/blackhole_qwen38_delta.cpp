/* SPDX-License-Identifier: MIT */
// OFFLINE one-head Qwen3.8 GDN image prerequisite. No driver selects it.
// Fixed L1 buffers ONLY: no NoC, DRAM, DMA, PCI, reset or supplied addresses.
// Caller supplies already-normalized/scaled Q/K and scalar decay/beta.
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQwen38Hybrid.h"
extern "C" void scalar_fault_park() __attribute__((noreturn));
extern "C" unsigned scalar_main()
{
	const volatile uint32_t* job=reinterpret_cast<const volatile uint32_t*>(0x74000);
	uint32_t magic=job[0],keys=job[1],values=job[2],reserved=job[3];
	if (magic!=0x44383351 || keys!=128 || values!=128 || reserved) return 0;
	const float* input=reinterpret_cast<const float*>(0xb0000);
	if (!PioneerQ38DeltaHead(input,input+128,input+256,input[384],input[385],
		reinterpret_cast<const float*>(0x90000),reinterpret_cast<float*>(0xa0000),
		reinterpret_cast<float*>(0xb0800))) scalar_fault_park();
	asm volatile("fence" ::: "memory");
	return 128;
}
