/*
 * Copyright 2019-2022, Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
*/


#include "arch_smp.h"

#include <algorithm>
#include <string.h>

#include <KernelExport.h>

#include <kernel.h>
#include <safemode.h>
#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/menu.h>
#include <platform/sbi/sbi_syscalls.h>

#include "mmu.h"


//#define TRACE_SMP
#ifdef TRACE_SMP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


typedef status_t (*KernelEntry) (kernel_args *bootKernelArgs, int currentCPU);


struct CpuEntryInfo {
	uint64 satp;		//  0
	uint64 stackBase;	//  8
	uint64 stackSize;	// 16
	KernelEntry kernelEntry;// 24
	kernel_args* kernelArgs;// 32
	uint64 cpu;		// 40
};


static platform_cpu_info sCpus[SMP_MAX_CPUS];
uint32 sCpuCount = 0;


static void __attribute__((naked))
arch_cpu_entry(int hartId, CpuEntryInfo* info)
{
	// Load everything from the boot loader's address space before switching to
	// the kernel page tables. The CpuEntryInfo allocation is not guaranteed to
	// remain mapped after satp changes.
	asm("ld t0, 0(a1)");   // CpuEntryInfo::satp
	asm("ld t1, 8(a1)");   // CpuEntryInfo::stackBase
	asm("ld t2, 16(a1)");  // CpuEntryInfo::stackSize
	asm("ld t3, 24(a1)");  // CpuEntryInfo::kernelEntry
	asm("ld t4, 32(a1)");  // CpuEntryInfo::kernelArgs
	asm("ld t5, 40(a1)");  // CpuEntryInfo::cpu

	// enable MMU
	asm("csrw satp, t0");
	asm("sfence.vma");

	// setup stack
	asm("mv sp, t1");
	asm("add sp, sp, t2");
	asm("li fp, 0");

	// No boot-loader address is valid after loading the kernel page table.
	// Enter the kernel directly with its normal _start(kernelArgs, cpu) ABI.
	asm("mv a0, t4");
	asm("mv a1, t5");
	asm("jr t3");
}


void
arch_smp_register_cpu(platform_cpu_info** cpu)
{
	uint32 newCount = sCpuCount + 1;
	if (newCount > SMP_MAX_CPUS) {
		*cpu = NULL;
		return;
	}
	*cpu = &sCpus[sCpuCount];
	sCpuCount = newCount;
}


platform_cpu_info*
arch_smp_find_cpu(uint32 phandle)
{
	for (uint32 i = 0; i < sCpuCount; i++) {
		if (sCpus[i].phandle == phandle)
			return &sCpus[i];
	}
	return NULL;
}


int
arch_smp_get_current_cpu(void)
{
	return Mhartid();
}


void
arch_smp_init_other_cpus(void)
{
	// Some RISC-V firmware supplies a stale boot-hartid in both the device tree
	// and EFI boot protocol.  HSM still knows which single hart is executing the
	// loader, so use it when it yields an unambiguous answer.
	uint32 startedHart = 0;
	uint32 startedHartCount = 0;
	for (uint32 i = 0; i < sCpuCount; i++) {
		sbiret result = sbi_hart_get_status(sCpus[i].id);
		if (result.error == SBI_SUCCESS
			&& result.value == SBI_HART_STATE_STARTED) {
			startedHart = sCpus[i].id;
			startedHartCount++;
		}
	}
	if (startedHartCount == 1)
		gBootHart = startedHart;

	// make boot CPU first as expected by kernel
	for (uint32 i = 1; i < sCpuCount; i++) {
		if (sCpus[i].id == gBootHart)
			std::swap(sCpus[i], sCpus[0]);
	}

	for (uint32 i = 0; i < sCpuCount; i++) {
		gKernelArgs.arch_args.hartIds[i] = sCpus[i].id;
		gKernelArgs.arch_args.plicContexts[i] = sCpus[i].plicContext;
	}

	// Bring up three secondary harts for the next bounded SG2042 SMP scaling
	// step. This exercises the AP trampoline, per-CPU PLIC contexts, inter-CPU
	// interrupts, and TLB shootdowns without jumping directly to all 64 harts.
	gKernelArgs.num_cpus = std::min<uint32>(sCpuCount, 4);
	dprintf("Pioneer: enabling %" B_PRIu32 " CPU(s)\n", gKernelArgs.num_cpus);

	if (get_safemode_boolean(B_SAFEMODE_DISABLE_SMP, false)) {
		// SMP has been disabled!
		TRACE(("smp disabled per safemode setting\n"));
		gKernelArgs.num_cpus = 1;
	}

	if (gKernelArgs.num_cpus < 2)
		return;

	for (uint32 i = 1; i < gKernelArgs.num_cpus; i++) {
		// create a final stack the trampoline code will put the ap processor on
		void * stack = NULL;
		const size_t size = KERNEL_STACK_SIZE
			+ KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE;
		if (platform_allocate_region(&stack, size, 0) != B_OK)
			panic("Unable to allocate AP stack");

		memset(stack, 0, size);
		gKernelArgs.cpu_kstack[i].start = fix_address((uint64_t)stack);
		gKernelArgs.cpu_kstack[i].size = size;
	}
}


void
arch_smp_boot_other_cpus(addr_t satp, uint64 kernel_entry, addr_t virtKernelArgs)
{
	dprintf("arch_smp_boot_other_cpus(%p, %p)\n", (void*)satp, (void*)kernel_entry);

	// Only start CPUs that arch_smp_init_other_cpus() enabled.
	for (uint32 i = 0; i < gKernelArgs.num_cpus; i++) {
		if (sCpus[i].id != gBootHart) {
			sbiret res;
			dprintf("  starting CPU %" B_PRIu32 "\n", sCpus[i].id);

			dprintf("  stack: %#" B_PRIx64 " - %#" B_PRIx64 "\n",
				gKernelArgs.cpu_kstack[i].start, gKernelArgs.cpu_kstack[i].start
				+ gKernelArgs.cpu_kstack[i].size - 1);

			CpuEntryInfo* info = new(std::nothrow) CpuEntryInfo{
				.satp = satp,
				.stackBase = gKernelArgs.cpu_kstack[i].start,
				.stackSize = gKernelArgs.cpu_kstack[i].size,
				.kernelEntry = (KernelEntry)kernel_entry,
				.kernelArgs = (kernel_args*)virtKernelArgs,
				.cpu = i
			};
			if (info == NULL)
				panic("Unable to allocate CPU entry information");

			res = sbi_hart_start(sCpus[i].id, (addr_t)&arch_cpu_entry, (addr_t)info);
			if (res.error != SBI_SUCCESS) {
				panic("Unable to start CPU %" B_PRIu32 ": SBI error %" B_PRId64,
					sCpus[i].id, res.error);
			}

			bigtime_t deadline = system_time() + 5000000;
			for (;;) {
				res = sbi_hart_get_status(sCpus[i].id);
				if (res.error < 0 || res.value == SBI_HART_STATE_STARTED)
					break;
				if (system_time() >= deadline)
					panic("Timed out starting CPU %" B_PRIu32, sCpus[i].id);
			}
			if (res.error < 0) {
				panic("Unable to query CPU %" B_PRIu32 ": SBI error %" B_PRId64,
					sCpus[i].id, res.error);
			}
		}
	}
}


void
arch_smp_add_safemode_menus(Menu *menu)
{
	MenuItem *item;

	if (gKernelArgs.num_cpus < 2)
		return;

	item = new(nothrow) MenuItem("Disable SMP");
	menu->AddItem(item);
	item->SetData(B_SAFEMODE_DISABLE_SMP);
	item->SetType(MENU_ITEM_MARKABLE);
	item->SetHelpText("Disables all but one CPU core.");
}


void
arch_smp_init(void)
{
}
