/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#ifndef _FDT_H_
#define _FDT_H_

#include <stdint.h>

extern void* gFdt;


void fdt_init(void* fdt);
void fdt_set_kernel_args();
#ifdef PIONEER_KEXEC_PROBE
void fdt_init_pioneer(unsigned int hart);
struct PioneerMemoryPlan;
bool fdt_pioneer_memory_plan(const void* fdt, uint64_t initrd,
	uint64_t initrdSize, PioneerMemoryPlan& plan);
#endif


#endif	// _FDT_H_
