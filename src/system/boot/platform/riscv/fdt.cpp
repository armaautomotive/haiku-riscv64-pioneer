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
#include "pioneer_memory.h"


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
// PIONEER_MEMORY_PARSER_BEGIN
static void
PioneerExcludeMemory(PioneerMemoryPlan& plan, uint64_t start, uint64_t size)
{
	if (!PioneerMemoryExclude(plan, start, size))
		PioneerKexecStop(false, "invalid_or_excessive_memory_reservations");
}

bool
fdt_pioneer_memory_plan(const void* fdt, uint64_t initrd,
	uint64_t initrdSize, PioneerMemoryPlan& plan)
{
	int length;
	int chosen = fdt_path_offset(fdt, "/chosen");
	const unsigned char* limit = (const unsigned char*)fdt_getprop(fdt, chosen,
		"haiku,pioneer-memory-gib", &length);
	if (limit == NULL)
		return false; // Preserve the existing 3038 MiB profile exactly.
	if (length != 4 || !PioneerMemoryPlanInit(plan, PioneerReadBeCell(limit, 4)))
		PioneerKexecStop(false, "invalid_requested_memory_limit");
	if (fdt_address_cells(fdt, 0) != 2 || fdt_size_cells(fdt, 0) != 2)
		PioneerKexecStop(false, "invalid_memory_root_cells");

	// This private profile is for the recorded 128 GiB Pioneer only. Do not
	// infer usable memory merely from an address ceiling or accept a gap.
	unsigned banks = 0;
	int node;
	fdt_for_each_subnode(node, fdt, 0) {
		const char* type = (const char*)fdt_getprop(fdt, node, "device_type", &length);
		if (type == NULL || length != 7 || memcmp(type, "memory", 7) != 0)
			continue;
		const char* status = (const char*)fdt_getprop(fdt, node, "status", &length);
		if (status != NULL && !((length == 5 && memcmp(status, "okay", 5) == 0)
			|| (length == 3 && memcmp(status, "ok", 3) == 0)))
			PioneerKexecStop(false, "disabled_memory_bank");
		const unsigned char* reg = (const unsigned char*)fdt_getprop(fdt, node,
			"reg", &length);
		if (reg == NULL || length != 16)
			PioneerKexecStop(false, "invalid_memory_bank_reg");
		uint64_t start = PioneerReadBeCell(reg, 8);
		uint64_t size = PioneerReadBeCell(reg + 8, 8);
		unsigned bank = start / UINT64_C(0x800000000);
		if (start >= UINT64_C(0x2000000000)
			|| start % UINT64_C(0x800000000) != 0
			|| size != UINT64_C(0x800000000) || (banks & (1U << bank)))
			PioneerKexecStop(false, "unexpected_memory_bank_layout");
		banks |= 1U << bank;
	}
	if (banks != 15)
		PioneerKexecStop(false, "incomplete_128GiB_memory_layout");

	int reservations = fdt_num_mem_rsv(fdt);
	if (reservations < 0 || reservations > 32)
		PioneerKexecStop(false, "invalid_FDT_memory_reservation_table");
	for (int i = 0; i < reservations; i++) {
		uint64_t start, size;
		if (fdt_get_mem_rsv(fdt, i, &start, &size) != 0)
			PioneerKexecStop(false, "invalid_FDT_memory_reservation");
		PioneerExcludeMemory(plan, start, size);
	}
	int reserved = fdt_path_offset(fdt, "/reserved-memory");
	if (reserved < 0 || fdt_address_cells(fdt, reserved) != 2
		|| fdt_size_cells(fdt, reserved) != 2)
		PioneerKexecStop(false, "invalid_reserved_memory_node");
	const void* ranges = fdt_getprop(fdt, reserved, "ranges", &length);
	if (ranges == NULL || length != 0)
		PioneerKexecStop(false, "reserved_memory_translation_not_supported");
	unsigned nodes = 0;
	fdt_for_each_subnode(node, fdt, reserved) {
		if (++nodes > 32)
			PioneerKexecStop(false, "excessive_reserved_memory_nodes");
		const unsigned char* reg = (const unsigned char*)fdt_getprop(fdt, node,
			"reg", &length);
		if (reg != NULL) {
			if (length <= 0 || length > 32 * 16 || length % 16 != 0)
				PioneerKexecStop(false, "invalid_reserved_memory_reg");
			for (int offset = 0; offset < length; offset += 16)
				PioneerExcludeMemory(plan, PioneerReadBeCell(reg + offset, 8),
					PioneerReadBeCell(reg + offset + 8, 8));
		} else {
			// The known, reusable Linux CMA pool has no fixed address here;
			// Linux/device shutdown has completed before this loader runs.
			// Reject unknown dynamic reservations instead of silently ignoring.
			if (strcmp(fdt_get_name(fdt, node, NULL), "linux,cma") != 0
				|| fdt_node_check_compatible(fdt, node, "shared-dma-pool") != 0
				|| fdt_getprop(fdt, node, "reusable", NULL) == NULL
				|| fdt_getprop(fdt, node, "linux,cma-default", NULL) == NULL)
				PioneerKexecStop(false, "unknown_dynamic_memory_reservation");
		}
	}
	PioneerExcludeMemory(plan, (uintptr_t)fdt, fdt_totalsize(fdt));
	PioneerExcludeMemory(plan, initrd, initrdSize);
	return true;
}
// PIONEER_MEMORY_PARSER_END

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
	// An absent property retains the already-tested single-hart adapter.
	int chosen = fdt_path_offset(gFdt, "/chosen");
	const unsigned char* requested = (const unsigned char*)fdt_getprop(gFdt,
		chosen, "haiku,pioneer-cpu-count", &length);
	if (requested != NULL) {
		if (length != 4 || !PioneerCpuCountValid(PioneerReadBeCell(requested, 4)))
			PioneerKexecStop(false, "invalid_requested_CPU_count");
		uint32 count = PioneerReadBeCell(requested, 4);
		PioneerCpuTopology topology = {};
		PioneerCpuRecord selected[64];
		fdt_for_each_subnode(cpu, gFdt, cpus) {
			int typeLength;
			const char* type = (const char*)fdt_getprop(gFdt, cpu, "device_type", &typeLength);
			if (type == NULL || typeLength != 4 || memcmp(type, "cpu", 4) != 0) continue;
			const char* status = (const char*)fdt_getprop(gFdt, cpu, "status", &length);
			if (status != NULL && !((length == 5 && memcmp(status, "okay", 5) == 0)
				|| (length == 3 && memcmp(status, "ok", 3) == 0)))
				PioneerKexecStop(false, "disabled_Pioneer_CPU");
			const unsigned char* reg = (const unsigned char*)fdt_getprop(gFdt, cpu, "reg", &length);
			if (reg == NULL || length != 4 || PioneerReadBeCell(reg, 4) >= 64)
				PioneerKexecStop(false, "invalid_Pioneer_CPU_reg");
			uint32 id = PioneerReadBeCell(reg, 4);
			int intc = fdt_subnode_offset(gFdt, cpu, "interrupt-controller");
			if (intc < 0 || fdt_node_check_compatible(gFdt, intc, "riscv,cpu-intc") != 0)
				PioneerKexecStop(false, "invalid_CPU_interrupt_controller");
			const unsigned char* cells = (const unsigned char*)fdt_getprop(gFdt, intc,
				"#interrupt-cells", &length);
			if (cells == NULL || length != 4 || PioneerReadBeCell(cells, 4) != 1
				|| !PioneerAddCpu(topology, id, fdt_get_phandle(gFdt, intc)))
				PioneerKexecStop(false, "invalid_CPU_topology");
		}
		contexts = (const unsigned char*)fdt_getprop(gFdt, plic, "interrupts-extended", &length);
		if (contexts == NULL || length != 1024) PioneerKexecStop(false, "invalid_SMP_PLIC_table");
		for (int offset = 0; offset < length; offset += 8) {
			if (!PioneerAddCpuContext(topology, PioneerReadBeCell(contexts + offset, 4),
				PioneerReadBeCell(contexts + offset + 4, 4), offset / 8))
				PioneerKexecStop(false, "invalid_SMP_PLIC_context");
		}
		if (!PioneerSelectCpus(topology, hart, count, selected))
			PioneerKexecStop(false, "incomplete_Pioneer_CPU_topology");
		smp_set_pioneer_cpus(selected, count);
		dprintf("Pioneer SMP opt-in: %u CPUs, boot hart %u first; all 64 FDT/PLIC records verified\n", count, hart);
	}
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
