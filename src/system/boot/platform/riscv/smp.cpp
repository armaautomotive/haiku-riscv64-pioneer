/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "smp.h"

#include <boot/stage2.h>
#ifdef PIONEER_KEXEC_PROBE
#include <boot/platform.h>
#include <kernel.h>
#include <string.h>
#include <platform/sbi/sbi_syscalls.h>
#include "pioneer_kexec.h"
#include "mmu.h"
static PioneerApEntry sEntries[64];
#endif


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
#ifdef PIONEER_KEXEC_PROBE
	if (PioneerKexecPreloadActive()) gKernelArgs.num_cpus = sCpuCount;
#endif
	for (uint32 i = 0; i < gKernelArgs.num_cpus; i++) {
		gKernelArgs.arch_args.hartIds[i] = sCpus[i].hartId;
		gKernelArgs.arch_args.plicContexts[i] = sCpus[i].plicContext;
	}
#ifdef PIONEER_KEXEC_PROBE
	if (PioneerKexecPreloadActive()) {
		for (uint32 i = 1; i < gKernelArgs.num_cpus; i++) {
			void* stack = NULL;
			size_t size = KERNEL_STACK_SIZE + KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE;
			if (platform_allocate_region(&stack, size, B_READ_AREA | B_WRITE_AREA) != B_OK)
				PioneerKexecStop(false, "AP_stack_allocation");
			memset(stack, 0, size);
			gKernelArgs.cpu_kstack[i].start = fix_address((addr_t)stack);
			gKernelArgs.cpu_kstack[i].size = size;
		}
	}
#endif
}


void
smp_boot_other_cpus(uint64 pageTable, uint64 kernel_entry)
{
#ifdef PIONEER_KEXEC_PROBE
	if (!PioneerKexecPreloadActive()) return;
	if (PioneerKexecMode() != 4)
		PioneerKexecStop(false, "AP_start_requires_execution_mode");
	for (uint32 i = 1; i < gKernelArgs.num_cpus; i++) {
		PioneerApEntry& entry = sEntries[i];
		if (entry.satp != pageTable || entry.kernelEntry != kernel_entry || entry.state != 0)
			PioneerKexecStop(false, "AP_descriptor_changed");
		sbiret before = sbi_hart_get_status(entry.hart);
		if (before.error != SBI_SUCCESS || before.value != SBI_HART_STATE_STOPPED)
			PioneerKexecStop(false, "AP_not_stopped");
		PioneerSmpMemoryBarrier();
		sbiret result = sbi_hart_start(entry.hart, (addr_t)&pioneer_ap_entry, (addr_t)&entry);
		if (result.error != SBI_SUCCESS) {
			dprintf("Pioneer AP start CPU %u hart %" B_PRIu64 " SBI error %ld\n", i, entry.hart, result.error);
			PioneerKexecStop(false, "AP_HSM_start_failed");
		}
		bigtime_t deadline = system_time() + 5000000;
		for (;;) {
			PioneerSmpMemoryBarrier();
			uint64 state = entry.state;
			if (state >= 2) {
				if (state != 2) {
					dprintf("Pioneer AP CPU %u hart %" B_PRIu64 " state %" B_PRIu64
						" trap %" B_PRIx64 "/%" B_PRIx64 "/%" B_PRIx64 "\n",
						i, entry.hart, state, entry.cause, entry.epc, entry.tval);
					PioneerKexecStop(false, "AP_entry_failed");
				}
				break;
			}
			if (system_time() >= deadline) PioneerKexecStop(false, "AP_entry_timeout");
		}
		dprintf("Pioneer AP ready CPU %u hart %" B_PRIu64 " PLIC %u\n", i, entry.hart, sCpus[i].plicContext);
	}
#endif
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
	// Default native handoff stays single-hart. Do not invent hart 0 when
	// Linux entered elsewhere. A separately validated opt-in may add APs.
	sCpus[0].hartId = hart;
	sCpus[0].phandle = phandle;
	sCpus[0].plicContext = context;
	sCpuCount = 1;
}

void
smp_set_pioneer_cpus(const PioneerCpuRecord* cpus, uint32 count)
{
	if (!PioneerCpuCountValid(count) || cpus[0].hart != sCpus[0].hartId)
		PioneerKexecStop(false, "invalid_selected_CPU_set");
	for (uint32 i = 0; i < count; i++)
		sCpus[i] = {cpus[i].phandle, cpus[i].hart, cpus[i].context};
	sCpuCount = count;
}

void
smp_prepare_pioneer(uint64 satp, uint64 kernelEntry)
{
	uint64 sxstatus;
	asm volatile("csrr %0, 0x5c0" : "=r"(sxstatus));
	for (uint32 i = 1; i < gKernelArgs.num_cpus; i++) {
		PioneerApEntry& entry = sEntries[i];
		memset(&entry, 0, sizeof(entry));
		entry.satp = satp;
		entry.stackBase = gKernelArgs.cpu_kstack[i].start;
		entry.stackSize = gKernelArgs.cpu_kstack[i].size;
		entry.kernelEntry = kernelEntry;
		entry.kernelArgs = (addr_t)&gKernelArgs;
		entry.cpu = i;
		entry.hart = sCpus[i].hartId;
		entry.mae = (sxstatus >> 21) & 1;
	}
}

const PioneerApEntry*
smp_pioneer_entry(uint32 cpu)
{
	return cpu < gKernelArgs.num_cpus && cpu > 0 ? &sEntries[cpu] : NULL;
}
#endif
