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
