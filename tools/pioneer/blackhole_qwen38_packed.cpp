/* SPDX-License-Identifier: MIT */
// OFFLINE local packed-block arithmetic prerequisite. NOT the fast matrix
// kernel. No driver selection/NoC/DRAM/DMA/PCI/reset or supplied addresses.
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQwen38PackedDot.h"
extern "C" void scalar_fault_park() __attribute__((noreturn));
extern "C" unsigned scalar_main()
{
	const volatile uint32_t* job=reinterpret_cast<const volatile uint32_t*>(0x74000);
	uint32_t magic=job[0],type=job[1],bytes=job[2],count=job[3];
	unsigned expected=type==8?34:type==12?144:type==14?210:0;
	if (magic!=0x50383351 || !expected || bytes!=expected || count!=(type==8?32U:256U)) return 0;
	float scale=*reinterpret_cast<const float*>(0x92000);
	if (!PioneerQ38PackedDot(type,reinterpret_cast<const PioneerQ38Byte*>(0x90000),bytes,
		reinterpret_cast<const PioneerQ38Int8*>(0x91000),count,scale,reinterpret_cast<float*>(0x93000))) scalar_fault_park();
	asm volatile("fence" ::: "memory"); return 1;
}
