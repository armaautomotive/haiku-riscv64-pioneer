/* Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk.
 * Distributed under the terms of the MIT License.
 */


#include <arch/platform.h>
#include <boot/kernel_args.h>
#include <Htif.h>
#include <Plic.h>
#include <Clint.h>
#include <platform/sbi/sbi_syscalls.h>

#include <debug.h>


uint32 gPlatform;
bool gRiscvTHeadMae;

void* gFDT = NULL;

HtifRegs  *volatile gHtifRegs  = (HtifRegs *volatile)0;
PlicRegs  *volatile gPlicRegs;
ClintRegs *volatile gClintRegs;


status_t
arch_platform_init(struct kernel_args *args)
{
	gPlatform = args->arch_args.machine_platform;

	debug_early_boot_message("machine_platform: ");
	switch (gPlatform) {
		case kPlatformMNative:
			debug_early_boot_message("Native mmode hooks\n");
			break;
		case kPlatformSbi:
			debug_early_boot_message("SBI\n");
			break;
		default:
			debug_early_boot_message("?\n");
			break;
	}

	gFDT = args->arch_args.fdt;

	gHtifRegs  = (HtifRegs *volatile)args->arch_args.htif.start;
	gPlicRegs  = (PlicRegs *volatile)args->arch_args.plic.start;
	gClintRegs = (ClintRegs *volatile)args->arch_args.clint.start;

	return B_OK;
}


status_t
arch_platform_init_post_vm(struct kernel_args *kernelArgs)
{
	if (gPlatform == kPlatformSbi) {
		// Formatted debug output still depends on atomic/SMP state that is not
		// ready during the Pioneer single-hart bootstrap.  Probe the SBI calls,
		// but report completion through the safe early serial path.
		(void)sbi_get_spec_version();
		(void)sbi_get_impl_id();
		(void)sbi_get_impl_version();
		sbiret vendor = sbi_get_mvendorid();
		(void)sbi_get_marchid();
		if (vendor.error == SBI_SUCCESS && vendor.value == 0x5b7) {
			uint64 sxstatus;
			asm volatile("csrr %0, 0x5c0" : "=r"(sxstatus));
			gRiscvTHeadMae = (sxstatus & (1UL << 21)) != 0;
		}
		debug_early_boot_message("riscv: SBI platform probed\n");
	}
	return B_OK;
}


status_t
arch_platform_init_post_thread(struct kernel_args *kernelArgs)
{
	return B_OK;
}
