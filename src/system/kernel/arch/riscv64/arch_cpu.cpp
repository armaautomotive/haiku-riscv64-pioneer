/*
 * Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk.
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>

#include <arch/cpu.h>
#include <boot/kernel_args.h>
#include <vm/VMAddressSpace.h>
#include <commpage.h>
#include <elf.h>
#include <Htif.h>
#include <platform/sbi/sbi_syscalls.h>

#include <algorithm>


extern "C" void SVec();

extern uint32 gPlatform;


status_t
arch_cpu_preboot_init_percpu(kernel_args *args, int curr_cpu)
{
	// dprintf("arch_cpu_preboot_init_percpu(%" B_PRId32 ")\n", curr_cpu);
	return B_OK;
}


status_t
arch_cpu_init_percpu(kernel_args *args, int curr_cpu)
{
	uint64 sxstatus;
	asm volatile("csrr %0, 0x5c0" : "=r"(sxstatus));
	dprintf("P288:CPU %" B_PRId32 " hart %" B_PRIu32
		" sxstatus %#" B_PRIx64 "\n", curr_cpu,
		args->arch_args.hartIds[curr_cpu], sxstatus);

	SetStvec((uint64)SVec);
	SstatusReg sstatus{.val = Sstatus()};
	sstatus.ie = 0;
	sstatus.fs = extStatusInitial; // enable FPU
	sstatus.xs = extStatusOff;
	SetSstatus(sstatus.val);
	SetBitsSie((1 << sTimerInt) | (1 << sSoftInt) | (1 << sExternInt));

	return B_OK;
}


status_t
arch_cpu_init(kernel_args *args)
{
	for (uint32 curCpu = 0; curCpu < args->num_cpus; curCpu++) {
		cpu_ent* cpu = &gCPU[curCpu];

		cpu->arch.hartId = args->arch_args.hartIds[curCpu];

		cpu->topology_id[CPU_TOPOLOGY_PACKAGE] = 0;
		cpu->topology_id[CPU_TOPOLOGY_CORE] = curCpu;
		cpu->topology_id[CPU_TOPOLOGY_SMT] = 0;

		for (unsigned int i = 0; i < CPU_MAX_CACHE_LEVEL; i++)
			cpu->cache_id[i] = -1;
	}

	uint64 conversionFactor
		= (1LL << 32) * 1000000LL / args->arch_args.timerFrequency;

	__riscv64_setup_system_time(conversionFactor);

	return B_OK;
}


status_t
arch_cpu_init_post_vm(kernel_args *args)
{
	// Set address space ownership to currently running threads
	debug_early_boot_message("riscv: cpu address space refs init\n");
	for (uint32 i = 0; i < args->num_cpus; i++) {
		if (gKernelStartup)
			VMAddressSpace::Kernel()->GetBootstrap();
		else
			VMAddressSpace::Kernel()->Get();
	}
	debug_early_boot_message("riscv: cpu address space refs ready\n");

	return B_OK;
}


status_t
arch_cpu_init_post_modules(kernel_args *args)
{
	return B_OK;
}


void
arch_cpu_sync_icache(void *address, size_t len)
{
	FenceI();

	if (smp_get_num_cpus() > 1) {
		memory_full_barrier();
		sbi_remote_fence_i(0, -1);
	}
}


void
arch_cpu_invalidate_tlb_range(intptr_t, addr_t start, addr_t end)
{
	addr_t kernelStart = std::max<addr_t>(start, KERNEL_BASE);
	addr_t kernelEnd   = std::min<addr_t>(end,   KERNEL_TOP);

	addr_t userStart = std::max<addr_t>(start, USER_BASE);
	addr_t userEnd   = std::min<addr_t>(end,   USER_TOP);

	if (kernelStart <= kernelEnd) {
		addr_t page = kernelStart;
		int64 numPages = kernelEnd / B_PAGE_SIZE - kernelStart / B_PAGE_SIZE;
		while (numPages-- >= 0) {
			FlushTlbPage(page);
			page += B_PAGE_SIZE;
		}
	}

	if (userStart <= userEnd) {
		addr_t page = userStart;
		int64 numPages = userEnd / B_PAGE_SIZE - userStart / B_PAGE_SIZE;
		while (numPages-- >= 0) {
			FlushTlbPageAsid(page, 0);
			page += B_PAGE_SIZE;
		}
	}
}


void
arch_cpu_invalidate_tlb_list(intptr_t, addr_t pages[], int num_pages)
{
	for (int i = 0; i < num_pages; i++) {
		addr_t page = pages[i];
		if (IS_KERNEL_ADDRESS(page))
			FlushTlbPage(page);
		else
			FlushTlbPageAsid(page, 0);
	}
}


void
arch_cpu_global_tlb_invalidate(void)
{
	FlushTlbAll();
}


void
arch_cpu_user_tlb_invalidate(intptr_t)
{
	FlushTlbAllAsid(0);
}


status_t
arch_cpu_shutdown(bool reboot)
{
	if (gPlatform == kPlatformSbi) {
		sbi_system_reset(
			reboot ? SBI_RESET_TYPE_COLD_REBOOT : SBI_RESET_TYPE_SHUTDOWN,
			SBI_RESET_REASON_NONE);
	}

	HtifShutdown();
	return B_ERROR;
}
