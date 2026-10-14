/*
 * Copyright 2003-2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2008, François Revol, revol@free.fr. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>
#include <boot/platform.h>
#include <boot/heap.h>
#include <boot/stage2.h>
#include <arch/cpu.h>

#include <string.h>

#include "console.h"
#include "cpu.h"
#include "mmu.h"
#include "smp.h"
#include "traps.h"
#include "fdt.h"
#include "Htif.h"
#include "virtio.h"
#include "graphics.h"
#include "pioneer_kexec.h"

#ifdef PIONEER_KEXEC_PROBE
static bool sPioneerPreloadActive;
static uint64_t sPioneerRamStart;
static size_t sPioneerRamSize;

bool PioneerKexecPreloadActive()
{
	return sPioneerPreloadActive;
}

const void* PioneerKexecRamDisk(size_t& size)
{
	size = sPioneerRamSize;
	return (const void*)(addr_t)sPioneerRamStart;
}
#endif


// GCC defined globals
extern void (*__ctor_list)(void);
extern void (*__ctor_end)(void);
extern uint8 __bss_start;
extern uint8 _end;

extern "C" int main(stage2_args* args);
extern "C" void _start(int hartId, void* fdt);
extern "C" status_t arch_enter_kernel(uint64 satp,
	struct kernel_args* kernelArgs, addr_t kernelEntry, addr_t kernelStackTop);


static uint32 sBootOptions;
static bool sBootOptionsValid = false;


static void
clear_bss(void)
{
	memset(&__bss_start, 0, &_end - &__bss_start);
}


static void
call_ctors(void)
{
	void (**f)(void);

	for (f = &__ctor_list; f < &__ctor_end; f++)
		(**f)();
}


static uint32
check_for_boot_keys()
{
	uint32 options = 0;
	bigtime_t t0 = system_time();
	while (system_time() - t0 < 100000) {
		int key = virtio_input_get_key();
		switch(key) {
			case 94 /* space */:
				options |= BOOT_OPTION_MENU;
				break;
		}
	}
	return options;
}


extern "C" uint32
platform_boot_options(void)
{
#ifdef PIONEER_KEXEC_PROBE
	if (PioneerKexecPreloadActive())
		return BOOT_OPTION_DEBUG_OUTPUT;
#endif
	if (!sBootOptionsValid) {
		sBootOptions = check_for_boot_keys();
		sBootOptionsValid = true;
/*
		if (sBootOptions & BOOT_OPTION_DEBUG_OUTPUT)
			serial_enable();
*/
	}

	return sBootOptions;
}


static void
convert_preloaded_image(preloaded_elf64_image* image)
{
	fix_address(image->next);
	fix_address(image->name);
	fix_address(image->debug_string_table);
	fix_address(image->syms);
	fix_address(image->rel);
	fix_address(image->rela);
	fix_address(image->pltrel);
	fix_address(image->debug_symbols);
}


static void
convert_kernel_args()
{
	if (gKernelArgs.kernel_image->elf_class != ELFCLASS64)
		return;

	fix_address(gKernelArgs.boot_volume);
	fix_address(gKernelArgs.vesa_modes);
	fix_address(gKernelArgs.edid_info);
	fix_address(gKernelArgs.debug_output);
	fix_address(gKernelArgs.boot_splash);
	#if defined(__x86_64__) || defined(__x86__)
	fix_address(gKernelArgs.ucode_data);
	fix_address(gKernelArgs.arch_args.apic);
	fix_address(gKernelArgs.arch_args.hpet);
	#endif
	fix_address(gKernelArgs.arch_args.fdt);

	convert_preloaded_image(static_cast<preloaded_elf64_image*>(
		gKernelArgs.kernel_image.Pointer()));
	fix_address(gKernelArgs.kernel_image);

	// Iterate over the preloaded images. Must save the next address before
	// converting, as the next pointer will be converted.
	preloaded_image* image = gKernelArgs.preloaded_images;
	fix_address(gKernelArgs.preloaded_images);
	while (image != NULL) {
		preloaded_image* next = image->next;
		convert_preloaded_image(static_cast<preloaded_elf64_image*>(image));
		image = next;
	}

	// Fix driver settings files.
	driver_settings_file* file = gKernelArgs.driver_settings;
	fix_address(gKernelArgs.driver_settings);
	while (file != NULL) {
		driver_settings_file* next = file->next;
		fix_address(file->next);
		fix_address(file->buffer);
		file = next;
	}
}


extern "C" void
platform_start_kernel(void)
{
#ifdef PIONEER_KEXEC_PROBE
	if (PioneerKexecPreloadActive()) {
		if (gKernelArgs.kernel_image == NULL
			|| gKernelArgs.kernel_image->elf_class != ELFCLASS64)
			PioneerKexecStop(false, "missing_ELF64_kernel");
		preloaded_elf64_image* kernel = static_cast<preloaded_elf64_image*>(
			gKernelArgs.kernel_image.Pointer());
		if (kernel->elf_header.e_machine != EM_RISCV)
			PioneerKexecStop(false, "wrong_kernel_architecture");
		dprintf("Pioneer preload: kernel entry %#lx\n", kernel->elf_header.e_entry);
		uint32 count = 0;
		for (preloaded_image* module = gKernelArgs.preloaded_images; module != NULL;
			module = module->next) {
			if (++count > 4096)
				PioneerKexecStop(false, "module_list_bounds");
		}
		dprintf("Pioneer preload: %" B_PRIu32 " preloaded boot images\n", count);
		if (count == 0)
			PioneerKexecStop(false, "no_boot_images");
		PioneerKexecStop(true, "kernel_and_boot_images_preloaded");
	}
#endif
	static struct kernel_args* args = &gKernelArgs;
	// platform_bootloader_address_to_kernel_address(&gKernelArgs, (addr_t*)&args);

	preloaded_elf64_image* image = static_cast<preloaded_elf64_image*>(
		gKernelArgs.kernel_image.Pointer());

	smp_init_other_cpus();
	//serial_cleanup();

	// Avoid interrupts from virtio devices before kernel driver takes control.
	virtio_fini();

	// The Pioneer firmware enters the payload in S-mode through OpenSBI.
	gKernelArgs.arch_args.machine_platform = kPlatformSbi;

	fdt_set_kernel_args();

	convert_kernel_args();
	uint64 satp;
	mmu_init_for_kernel(satp);

	smp_boot_other_cpus(satp, image->elf_header.e_entry);

	dprintf("kernel entry: %lx\n", image->elf_header.e_entry);
	dprintf("args: %lx\n", (addr_t)args);

	addr_t stackTop
		= gKernelArgs.cpu_kstack[0].start + gKernelArgs.cpu_kstack[0].size;
	arch_enter_kernel(satp, args, image->elf_header.e_entry, stackTop);

	panic("kernel returned!\n");

}


extern "C" void
platform_exit(void)
{
	HtifShutdown();
}


extern "C" void
_start(int hartId, void* fdt)
{
	HtifOutString("haiku_loader entry point\n");

	clear_bss();
	fdt_init(fdt);
	call_ctors();

	stage2_args args;
	args.heap_size = 0;
	args.arguments = NULL;

	traps_init();
	// console_init();
	// virtio_init();
	cpu_init();
	mmu_init();
	//apm_init();
	smp_init();

	main(&args);
}

#ifdef PIONEER_KEXEC_PROBE
extern "C" void
pioneer_kexec_preload(uint64_t hart, const void* fdt, uint64_t start, uint64_t size)
{
	// Keep the live stack separate from cleared BSS, and initialize platform
	// state only after the early no-allocation metadata checks have passed.
	clear_bss();
	sPioneerPreloadActive = true;
	sPioneerRamStart = start;
	sPioneerRamSize = size;
	gFdt = (void*)fdt; // Already fully validated; do not enumerate hardware here.
	uint64_t poolEnd = PioneerPreloadPoolEnd((addr_t)fdt, start);
	extern uint8 gStackEnd;
	if (poolEnd <= (addr_t)&gStackEnd + 64 * 1024 * 1024)
		PioneerKexecStop(false, "insufficient_nonoverlapping_boot_pool");
	gMemBase = (uint8*)0x2200000;
	gTotalMem = poolEnd - (addr_t)gMemBase;
	call_ctors();
	cpu_init();
	mmu_init();
	dprintf("Pioneer preload: hart %" B_PRIu64 ", RAM disk %p (%" B_PRIu64
		" bytes), allocation ceiling %#" B_PRIx64 "\n", hart,
		(const void*)(addr_t)start, size, poolEnd);
	stage2_args args = {};
	args.heap_size = 16 * 1024 * 1024;
	main(&args);
	PioneerKexecStop(false, "stage2_returned_without_preload");
}
#endif
