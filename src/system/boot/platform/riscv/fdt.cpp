/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "fdt.h"
#include <SupportDefs.h>
#include <ByteOrder.h>
#include <KernelExport.h>
#include <boot/stage2.h>
#include <arch/generic/debug_uart_8250.h>
#include <arch/riscv64/arch_uart_sifive.h>
#include <arch_cpu_defs.h>

extern "C" {
#include <libfdt.h>
}

#include "mmu.h"
#include "smp.h"
#include "graphics.h"
#include "virtio.h"
#include "Htif.h"
#include "Clint.h"
#include "FwCfg.h"
#include "pioneer_kexec.h"


void* gFdt = NULL;
ClintRegs *volatile gClintRegs = NULL;

static uint64 sTimerFrequrency = 10000000;

static addr_range sPlic = {0};
static addr_range sClint = {0};
static uart_info sUart{};


static bool
HasFdtString(const char* prop, int size, const char* pattern)
{
	int patternLen = strlen(pattern);
	const char* propEnd = prop + size;
	while (propEnd - prop > 0) {
		int curLen = strlen(prop);
		if (curLen == patternLen && memcmp(prop, pattern, curLen + 1) == 0)
			return true;
		prop += curLen + 1;
	}
	return false;
}


static bool
GetReg(const void* fdt, int node, uint32 addressCells, uint32 sizeCells, size_t idx,
	addr_range& range)
{
	int propSize;
	const uint8* prop = (const uint8*)fdt_getprop(fdt, node, "reg", &propSize);
	if (prop == NULL)
		return false;

	size_t entrySize = 4*(addressCells + sizeCells);
	if ((idx + 1)*entrySize > (size_t)propSize)
		return false;

	prop += idx*entrySize;

	switch (addressCells) {
		case 1: range.start = fdt32_to_cpu(*(uint32*)prop); prop += 4; break;
		case 2: range.start = fdt64_to_cpu(*(uint64*)prop); prop += 8; break;
		default: panic("unsupported addressCells");
	}
	switch (sizeCells) {
		case 1: range.size = fdt32_to_cpu(*(uint32*)prop); prop += 4; break;
		case 2: range.size = fdt64_to_cpu(*(uint64*)prop); prop += 8; break;
		default: panic("unsupported sizeCells");
	}
	return true;
}


static uint32
GetInterrupt(const void* fdt, int node)
{
	if (uint32* prop = (uint32*)fdt_getprop(fdt, node, "interrupts-extended", NULL)) {
		return fdt32_to_cpu(*(prop + 1));
	}
	if (uint32* prop = (uint32*)fdt_getprop(fdt, node, "interrupts", NULL)) {
		return fdt32_to_cpu(*prop);
	}
	dprintf("[!] no interrupt field\n");
	return 0;
}


static void
HandleFdt(const void* fdt, int node, uint32 addressCells, uint32 sizeCells,
	uint32 interruptCells /* from parent node */)
{
	// TODO: handle different field sizes

	const char* name = fdt_get_name(fdt, node, NULL);
	if (strcmp(name, "cpus") == 0) {
		if (uint32* prop = (uint32*)fdt_getprop(fdt, node, "timebase-frequency", NULL))
			sTimerFrequrency = fdt32_to_cpu(*prop);
	}

	const char* device_type = (const char*)fdt_getprop(fdt, node,
		"device_type", NULL);
	if (device_type != NULL && strcmp(device_type, "memory") == 0) {
		gMemBase = (uint8*)fdt64_to_cpu(*((uint64*)fdt_getprop(fdt, node,
			"reg", NULL) + 0));
		gTotalMem = fdt64_to_cpu(*((uint64*)fdt_getprop(fdt, node,
			"reg", NULL) + 1));
		return;
	}
	int compatibleLen;
	const char* compatible = (const char*)fdt_getprop(fdt, node,
		"compatible", &compatibleLen);
	if (compatible == NULL) return;
	if (HasFdtString(compatible, compatibleLen, "riscv,clint0")) {
		uint64* reg = (uint64*)fdt_getprop(fdt, node, "reg", NULL);
		sClint.start = fdt64_to_cpu(*(reg + 0));
		sClint.size  = fdt64_to_cpu(*(reg + 1));
		gClintRegs = (ClintRegs*)sClint.start;
	} else if (HasFdtString(compatible, compatibleLen, "riscv,plic0")
		|| HasFdtString(compatible, compatibleLen, "thead,c900-plic")) {
		GetReg(fdt, node, addressCells, sizeCells, 0, sPlic);
		int propSize;
		if (uint32* prop = (uint32*)fdt_getprop(fdt, node, "interrupts-extended", &propSize)) {
			dprintf("PLIC contexts\n");
			uint32 contextId = 0;
			for (uint32 *it = prop; (uint8_t*)it - (uint8_t*)prop < propSize; it += 2) {
				uint32 phandle = fdt32_to_cpu(*it);
				uint32 interrupt = fdt32_to_cpu(*(it + 1));
				if (interrupt == sExternInt) {
					CpuInfo* cpuInfo = smp_find_cpu(phandle);
					dprintf("  context %" B_PRIu32 ": %" B_PRIu32 "\n", contextId, phandle);
					if (cpuInfo != NULL) {
						cpuInfo->plicContext = contextId;
						dprintf("    hartId: %" B_PRIu32 "\n", cpuInfo->hartId);
					}
				}
				contextId++;
			}
		}
	} else if (HasFdtString(compatible, compatibleLen, "virtio,mmio")) {
		uint64* reg = (uint64*)fdt_getprop(fdt, node, "reg", NULL);
		virtio_register(
			fdt64_to_cpu(*(reg + 0)), fdt64_to_cpu(*(reg + 1)),
			GetInterrupt(fdt, node));
	} else if (
		strcmp(sUart.kind, "") == 0 && (
			HasFdtString(compatible, compatibleLen, "ns16550a") ||
			HasFdtString(compatible, compatibleLen, "snps,dw-apb-uart") ||
			HasFdtString(compatible, compatibleLen, "sifive,uart0"))
	) {
		if (HasFdtString(compatible, compatibleLen, "ns16550a")
			|| HasFdtString(compatible, compatibleLen, "snps,dw-apb-uart"))
			strcpy(sUart.kind, UART_KIND_8250);
		else if (HasFdtString(compatible, compatibleLen, "sifive,uart0"))
			strcpy(sUart.kind, UART_KIND_SIFIVE);

		uint64* reg = (uint64*)fdt_getprop(fdt, node, "reg", NULL);
		sUart.regs.start = fdt64_to_cpu(*(reg + 0));
		sUart.regs.size  = fdt64_to_cpu(*(reg + 1));
		sUart.irq = GetInterrupt(fdt, node);
		const void* prop = fdt_getprop(fdt, node, "clock-frequency", NULL);
		sUart.clock = (prop == NULL) ? 0 : fdt32_to_cpu(*(uint32*)prop);
		prop = fdt_getprop(fdt, node, "reg-shift", NULL);
		sUart.reg_shift = (prop == NULL) ? 0 : fdt32_to_cpu(*(uint32*)prop);
		prop = fdt_getprop(fdt, node, "reg-io-width", NULL);
		sUart.reg_io_width = (prop == NULL) ? 1 : fdt32_to_cpu(*(uint32*)prop);
	} else if (HasFdtString(compatible, compatibleLen, "qemu,fw-cfg-mmio")) {
		gFwCfgRegs = (FwCfgRegs *volatile)
			fdt64_to_cpu(*(uint64*)fdt_getprop(fdt, node, "reg", NULL));
	} else if (HasFdtString(compatible, compatibleLen, "simple-framebuffer")) {
		gFramebuf.colors = (uint32*)fdt64_to_cpu(
			*(uint64*)fdt_getprop(fdt, node, "reg", NULL));
		gFramebuf.stride = fdt32_to_cpu(
			*(uint32*)fdt_getprop(fdt, node, "stride", NULL)) / 4;
		gFramebuf.width = fdt32_to_cpu(
			*(uint32*)fdt_getprop(fdt, node, "width", NULL));
		gFramebuf.height = fdt32_to_cpu(
			*(uint32*)fdt_getprop(fdt, node, "height", NULL));
	}
}


void
fdt_init(void* fdt)
{
	dprintf("FDT: %p\n", fdt);
	gFdt = fdt;

	int res = fdt_check_header(gFdt);
	if (res != 0) {
		panic("Invalid FDT: %s\n", fdt_strerror(res));
	}

	dprintf("FDT valid, size: %" B_PRIu32 "\n", fdt_totalsize(gFdt));

	int node = -1;
	int depth = -1;
	while ((node = fdt_next_node(gFdt, node, &depth)) >= 0 && depth >= 0) {
		HandleFdt(gFdt, node, 2, 2, 1);
	}
}


void
fdt_set_kernel_args()
{
	uint32_t fdtSize = fdt_totalsize(gFdt);

	// libfdt requires 8-byte alignment
	gKernelArgs.arch_args.fdt = (void*)(addr_t)kernel_args_malloc(fdtSize, 8);

	if (gKernelArgs.arch_args.fdt != NULL)
		memcpy(gKernelArgs.arch_args.fdt, gFdt, fdt_totalsize(gFdt));
	else
		panic("unable to malloc for FDT!\n");

	gKernelArgs.arch_args.timerFrequency = sTimerFrequrency;

#ifdef PIONEER_KEXEC_PROBE
	if (PioneerKexecPreloadActive()) {
		// The Pioneer has SBI, not the emulator's HTIF MMIO device.
		gKernelArgs.arch_args.htif = {};
	} else
#endif
	{
		gKernelArgs.arch_args.htif.start = (addr_t)gHtifRegs;
		gKernelArgs.arch_args.htif.size = sizeof(HtifRegs);
	}

	gKernelArgs.arch_args.plic  = sPlic;
	gKernelArgs.arch_args.clint = sClint;
	gKernelArgs.arch_args.uart  = sUart;
}

#ifdef PIONEER_KEXEC_PROBE
void
fdt_init_pioneer(unsigned int hart)
{
	// Read only the resources used by the initial single-hart handoff. Avoid
	// generic emulator enumeration and its last-memory-node allocator override.
	int cpus = fdt_path_offset(gFdt, "/cpus");
	int length;
	const unsigned char* timebase = (const unsigned char*)fdt_getprop(gFdt,
		cpus, "timebase-frequency", &length);
	if (!timebase || length != 4 || PioneerReadBeCell(timebase, 4) == 0)
		PioneerKexecStop(false, "invalid_timebase");
	sTimerFrequrency = PioneerReadBeCell(timebase, 4);
	uint32 phandle = 0;
	int cpu;
	fdt_for_each_subnode(cpu, gFdt, cpus) {
		const unsigned char* reg = (const unsigned char*)fdt_getprop(gFdt, cpu,
			"reg", &length);
		if (!reg || length != 4 || PioneerReadBeCell(reg, 4) != hart)
			continue;
		int intc = fdt_subnode_offset(gFdt, cpu, "interrupt-controller");
		if (fdt_node_check_compatible(gFdt, intc, "riscv,cpu-intc") == 0)
			phandle = fdt_get_phandle(gFdt, intc);
	}
	if (phandle == 0 || phandle == UINT32_MAX)
		PioneerKexecStop(false, "boot_hart_not_in_fdt");
	int plic = fdt_node_offset_by_compatible(gFdt, -1, "thead,c900-plic");
	if (plic < 0 || !GetReg(gFdt, plic, 2, 2, 0, sPlic)
		|| sPlic.start != 0x7090000000ULL || sPlic.size != 0x4000000)
		PioneerKexecStop(false, "invalid_pioneer_plic");
	const unsigned char* contexts = (const unsigned char*)fdt_getprop(gFdt,
		plic, "interrupts-extended", &length);
	if (!contexts || length <= 0 || length > 64 * 16 || length % 8)
		PioneerKexecStop(false, "invalid_plic_contexts");
	uint32 context = UINT32_MAX;
	for (int offset = 0; offset < length; offset += 8) {
		if (PioneerReadBeCell(contexts + offset, 4) == phandle
			&& PioneerReadBeCell(contexts + offset + 4, 4) == sExternInt) {
			if (context != UINT32_MAX)
				PioneerKexecStop(false, "duplicate_boot_hart_context");
			context = offset / 8;
		}
	}
	if (context == UINT32_MAX)
		PioneerKexecStop(false, "missing_boot_hart_context");
	smp_init_pioneer(hart, phandle, context);
	int uart = fdt_path_offset(gFdt, "/soc/serial@7040000000");
	// Base/size/shift/width were already checked before any C++ initialization.
	strcpy(sUart.kind, UART_KIND_8250);
	sUart.regs = {0x7040000000ULL, 0x1000};
	sUart.reg_shift = 2;
	sUart.reg_io_width = 4;
	sUart.irq = GetInterrupt(gFdt, uart);
	const unsigned char* clock = (const unsigned char*)fdt_getprop(gFdt, uart,
		"clock-frequency", &length);
	if (!clock || length != 4 || !PioneerReadBeCell(clock, 4) || sUart.irq != 112)
		PioneerKexecStop(false, "invalid_pioneer_uart_clock_irq");
	sUart.clock = PioneerReadBeCell(clock, 4);
	dprintf("Pioneer hardware: hart %u, PLIC context %" B_PRIu32
		", timebase %" B_PRIu64 ", UART IRQ %" B_PRIu32 "\n",
		hart, context, sTimerFrequrency, sUart.irq);
}
#endif
