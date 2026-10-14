/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "smp.h"

#include <boot/stage2.h>


static CpuInfo sCpus[SMP_MAX_CPUS];
uint32 sCpuCount = 0;


CpuInfo*
smp_find_cpu(uint32 phandle)
{
	return &sCpus[0];
}


void
smp_init_other_cpus(void)
{
	gKernelArgs.num_cpus = 1;
	for (uint32 i = 0; i < gKernelArgs.num_cpus; i++) {
		gKernelArgs.arch_args.hartIds[i] = sCpus[i].hartId;
		gKernelArgs.arch_args.plicContexts[i] = sCpus[i].plicContext;
	}
}


void
smp_boot_other_cpus(uint64 pageTable, uint64 kernel_entry)
{
}


void
smp_init()
{
	sCpus[0].hartId = 0;
}

#ifdef PIONEER_KEXEC_PROBE
void
smp_init_pioneer(uint32 hart, uint32 phandle, uint32 context)
{
	// First native handoff is deliberately single-hart. Do not report hart 0
	// when Linux kexec entered on another physical hart, or restart stopped APs.
	sCpus[0].hartId = hart;
	sCpus[0].phandle = phandle;
	sCpus[0].plicContext = context;
	sCpuCount = 1;
}
#endif
