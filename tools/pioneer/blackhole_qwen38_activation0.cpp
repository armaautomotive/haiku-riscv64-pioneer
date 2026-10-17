/* SPDX-License-Identifier: MIT */
// OFFLINE local Q8_0 serialization ONLY,no driver/GPU launch authority.
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQwen38Activation.h"
extern "C" void scalar_fault_park() __attribute__((noreturn));
extern "C" unsigned scalar_main()
{
	const volatile uint32_t* job=reinterpret_cast<const volatile uint32_t*>(0x74000);
	if (job[0]!=0x30383351 || job[1]!=32 || job[2]!=36 || job[3]) return 0;
	if (!PioneerQ38Quantize0(reinterpret_cast<const float*>(0x90000),32,
		reinterpret_cast<PioneerQ38Activation0*>(0x91000))) scalar_fault_park();
	asm volatile("fence" ::: "memory"); return 1;
}
