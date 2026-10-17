/* SPDX-License-Identifier: MIT */
// OFFLINE local activation image ONLY. No supplied addresses,NoC/DRAM/
// DMA/PCI/reset/driver selection. Not the final fast matrix backend.
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQwen38Activation.h"
extern "C" void scalar_fault_park() __attribute__((noreturn));
extern "C" unsigned scalar_main()
{
	const volatile uint32_t* job=reinterpret_cast<const volatile uint32_t*>(0x74000);
	if (job[0]!=0x4b383351 || job[1]!=256 || job[2]!=292 || job[3]) return 0;
	if (!PioneerQ38QuantizeK(reinterpret_cast<const float*>(0x90000),256,
		reinterpret_cast<PioneerQ38ActivationK*>(0x91000))) scalar_fault_park();
	asm volatile("fence" ::: "memory"); return 1;
}
