/*
 * Copyright 2026 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <new>
#include <string.h>

#include <AutoDeleterDrivers.h>
#include <KernelExport.h>
#include <bus/FDT.h>

#include "mmc.h"
#include "sdhci.h"


#define SDHCI_FDT_MMC_BUS_MODULE_NAME "busses/mmc/sdhci/fdt/device/v1"


static volatile uint8* sSg2042ClockBase;


void
dump_sg2042_clock_state()
{
	if (sSg2042ClockBase == NULL) {
		dprintf("P291:CLK external clock mapping unavailable\n");
		return;
	}

	memory_full_barrier();
	dprintf("P291:CLK gate0 %#" B_PRIx32 " gate1 %#" B_PRIx32
		" divSD %#" B_PRIx32 " div100k %#" B_PRIx32 " divAXI %#"
		B_PRIx32 " divHSP %#" B_PRIx32 "\n",
		*(volatile uint32*)(sSg2042ClockBase + 0x00),
		*(volatile uint32*)(sSg2042ClockBase + 0x04),
		*(volatile uint32*)(sSg2042ClockBase + 0x94),
		*(volatile uint32*)(sSg2042ClockBase + 0x98),
		*(volatile uint32*)(sSg2042ClockBase + 0x9c),
		*(volatile uint32*)(sSg2042ClockBase + 0xa0));
	memory_full_barrier();
}


static bool
program_sg2042_divider(volatile uint32* divider, uint32 expectedValue)
{
	uint32 oldValue = *divider;
	if (oldValue == expectedValue)
		return true;

	// Match the vendor clock driver's assert/program/deassert sequence.
	*divider = oldValue & ~1u;
	memory_full_barrier();
	*divider = expectedValue & ~1u;
	memory_full_barrier();
	*divider = expectedValue;
	memory_full_barrier();
	uint32 newValue = *divider;
	memory_full_barrier();
	return newValue == expectedValue;
}


static status_t
enable_sg2042_clocks()
{
	// The SG2042 SDHCI block has three child gates and an AXI parent gate in
	// TOP_MISC CLK_EN_REG1.
	// Firmware does not guarantee that they remain enabled when Haiku starts,
	// and the controller cannot complete a software reset without all three.
	static const phys_addr_t kClockGatePage = 0x7030012000;
	static const size_t kClockGatePageSize = B_PAGE_SIZE;
	static const size_t kClockGateOffset = 0x4;
	static const uint32 kParentClockGateMask = 1u << 12;
	// The working Pioneer vendor kernel leaves CLK_EN_REG1 fully enabled. Some
	// undocumented interconnect dependency is not represented in its clock
	// tree, since the documented SD and parent bits alone do not clock reset.
	static const uint32 kClockGateMask = UINT32_MAX;

	uint8* mappedBase;
	area_id area = map_physical_memory("SG2042 SD clock gates",
		kClockGatePage, kClockGatePageSize,
		B_ANY_KERNEL_BLOCK_ADDRESS | B_WRITE_COMBINING_MEMORY,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void**)&mappedBase);
	if (area < B_OK)
		return area;
	sSg2042ClockBase = mappedBase;
	dprintf("P220:VM0 clock physical %#" B_PRIxPHYSADDR " virtual %p area %"
		B_PRId32 "\n", kClockGatePage, mappedBase, area);

	volatile uint32* clockGate0 = (volatile uint32*)mappedBase;
	volatile uint32* clockGate
		= (volatile uint32*)(mappedBase + kClockGateOffset);
	volatile uint32* sdDivider = (volatile uint32*)(mappedBase + 0x94);
	volatile uint32* sd100kDivider = (volatile uint32*)(mappedBase + 0x98);
	volatile uint32* axi0Divider = (volatile uint32*)(mappedBase + 0x9c);
	volatile uint32* axiHsPeripheralDivider
		= (volatile uint32*)(mappedBase + 0xa0);
	memory_full_barrier();
	uint32 gate0Value = *clockGate0;
	uint32 oldValue = *clockGate;
	dprintf("P219:SC0 clock gates REG0 %#" B_PRIx32 " REG1 %#" B_PRIx32
		"\n", gate0Value, oldValue);
	dprintf("P217:SC0 div SD %#" B_PRIx32 " 100k %#" B_PRIx32
		" AXI0 %#" B_PRIx32 " HSPERI %#" B_PRIx32 "\n", *sdDivider,
		*sd100kDivider, *axi0Divider, *axiHsPeripheralDivider);

	// UEFI leaves these dividers asserted at zero after ExitBootServices. Use
	// the values observed under the working SG2042 vendor Linux clock driver:
	// SD 100 MHz, auxiliary SD clock ~356 kHz, AXI0 ~91 MHz, HSPERI 250 MHz.
	bool dividersReady = program_sg2042_divider(axi0Divider, 0x000b0001);
	dividersReady &= program_sg2042_divider(axiHsPeripheralDivider,
		0x00040009);
	dividersReady &= program_sg2042_divider(sdDivider, 0x000a0009);
	dividersReady &= program_sg2042_divider(sd100kDivider, 0x00ff0009);
	dprintf("P217:SC1 div SD %#" B_PRIx32 " 100k %#" B_PRIx32
		" AXI0 %#" B_PRIx32 " HSPERI %#" B_PRIx32 "\n", *sdDivider,
		*sd100kDivider, *axi0Divider, *axiHsPeripheralDivider);
	if (!dividersReady) {
		delete_area(area);
		return B_ERROR;
	}
	spin(100);

	// clk_gate_axi_sd is downstream of the critical high-speed peripheral
	// AXI gate. Enable and settle that parent before touching any child gate.
	*clockGate = oldValue | kParentClockGateMask;
	memory_full_barrier();
	uint32 parentValue = *clockGate;
	memory_full_barrier();
	if ((parentValue & kParentClockGateMask) != kParentClockGateMask) {
		delete_area(area);
		return B_ERROR;
	}
	spin(100);

	*clockGate = parentValue | kClockGateMask;
	memory_full_barrier();
	uint32 newValue = *clockGate;
	memory_full_barrier();
	dprintf("P219:SC1 SD clocks gate %#" B_PRIx32 " -> %#" B_PRIx32
		" -> %#" B_PRIx32 "\n", oldValue, parentValue, newValue);

	// Keep this mapping alive while the SDHCI bus exists. Besides matching the
	// lifetime of the clocks it controls, this prevents the controller mapping
	// from immediately reusing the same virtual address. P220 uses that as a
	// diagnostic for stale RISC-V TLB translations during device-area reuse.
	if (newValue != kClockGateMask) {
		delete_area(area);
		return B_ERROR;
	}

	// Give the clocks time to propagate before issuing the host reset.
	spin(100);
	return B_OK;
}


float
supports_device_fdt(device_node* parent)
{
	const char* compatible;
	if (gDeviceManager->get_attr_string(parent, "fdt/compatible", &compatible,
			false) != B_OK) {
		return 0.0f;
	}

	if (strcmp(compatible, "bitmain,bm-sd") == 0
		|| strcmp(compatible, "sophgo,sg2042-dwcmshc") == 0
		|| strcmp(compatible, "snps,dwcmshc-sdhci") == 0) {
		return 0.8f;
	}

	return 0.0f;
}


status_t
register_child_devices_fdt(void* cookie)
{
	dprintf("P202:SD0 register FDT child\n");
	SdhciDevice* context = (SdhciDevice*)cookie;
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
			{ .string = "FDT SDHC bus" } },
		{ B_DEVICE_FIXED_CHILD, B_STRING_TYPE,
			{ .string = MMC_BUS_MODULE_NAME } },
		{ B_DEVICE_BUS, B_STRING_TYPE, { .string = "mmc" } },

		{ B_DMA_ALIGNMENT, B_UINT32_TYPE, { .ui32 = 511 } },
		{ B_DMA_HIGH_ADDRESS, B_UINT64_TYPE, { .ui64 = 0x100000000LL } },
		{ B_DMA_BOUNDARY, B_UINT32_TYPE, { .ui32 = (1 << 19) - 1 } },
		{ B_DMA_MAX_SEGMENT_COUNT, B_UINT32_TYPE, { .ui32 = 1 } },
		{ B_DMA_MAX_SEGMENT_BLOCKS, B_UINT32_TYPE,
			{ .ui32 = (1 << 10) - 1 } },
		{}
	};

	status_t status = gDeviceManager->register_node(context->fNode,
		SDHCI_FDT_MMC_BUS_MODULE_NAME, attrs, NULL, NULL);
	dprintf("P202:SD1 FDT child registered: %s\n", strerror(status));
	return status;
}


status_t
init_bus_fdt(device_node* node, void** busCookie)
{
	dprintf("P202:SD2 init FDT bus\n");
	DeviceNodePutter<&gDeviceManager> controller(
		gDeviceManager->get_parent_node(node));
	if (!controller.IsSet())
		return B_BAD_VALUE;

	DeviceNodePutter<&gDeviceManager> fdtNode(
		gDeviceManager->get_parent_node(controller.Get()));
	if (!fdtNode.IsSet())
		return B_BAD_VALUE;

	fdt_device_module_info* fdt;
	fdt_device* device;
	status_t status = gDeviceManager->get_driver(fdtNode.Get(),
		(driver_module_info**)&fdt, (void**)&device);
	if (status != B_OK)
		return status;
	dprintf("P202:SD3 FDT parent ready\n");

	uint64 physicalAddress;
	uint64 registerSize;
	if (!fdt->get_reg(device, 0, &physicalAddress, &registerSize)
		|| registerSize < sizeof(registers)) {
		return B_BAD_DATA;
	}

	uint64 irq;
	if (!fdt->get_interrupt(device, 0, NULL, &irq) || irq > UINT8_MAX)
		return B_BAD_DATA;
	dprintf("P202:SD4 regs %#" B_PRIx64 " size %#" B_PRIx64 " irq %" B_PRIu64 "\n",
		physicalAddress, registerSize, irq);

	const char* compatible;
	uint32 quirks = 0;
	if (gDeviceManager->get_attr_string(fdtNode.Get(), "fdt/compatible",
			&compatible, false) == B_OK
		&& (strcmp(compatible, "bitmain,bm-sd") == 0
			|| strcmp(compatible, "sophgo,sg2042-dwcmshc") == 0)) {
		quirks |= SDHCI_QUIRK_SG2042_PHY;
		status = enable_sg2042_clocks();
		if (status != B_OK) {
			dprintf("P217:SC3 failed to enable SG2042 SD clocks: %s\n",
				strerror(status));
			return status;
		}
	}

	struct registers* mappedRegisters;
	uint32 addressSpec = B_ANY_KERNEL_BLOCK_ADDRESS;
	if ((quirks & SDHCI_QUIRK_SG2042_PHY) != 0) {
		// SG2042's SDHCI command register stalls when mapped with the T-Head
		// strongly ordered I/O PTE type.  Request the T-Head non-cacheable
		// type for this controller without weakening other MMIO mappings such
		// as PCIe.
		addressSpec |= B_WRITE_COMBINING_MEMORY;
	}
	area_id registersArea = map_physical_memory("FDT SDHC registers",
		physicalAddress, registerSize, addressSpec,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		(void**)&mappedRegisters);
	if (registersArea < B_OK)
		return registersArea;
	dprintf("P220:VM1 SDHCI physical %#" B_PRIx64 " virtual %p area %" B_PRId32
		"\n", physicalAddress, mappedRegisters, registersArea);
	dprintf("P202:SD5 registers mapped\n");

	// Poll until the RISC-V interrupt path for this controller is validated.
	dprintf("P202:SD6 construct controller\n");
	SdhciBus* bus = new(std::nothrow) SdhciBus(mappedRegisters, (uint8)irq,
		true, quirks);
	if (bus == NULL) {
		delete_area(registersArea);
		return B_NO_MEMORY;
	}

	status = bus->InitCheck();
	if (status != B_OK) {
		delete bus;
		return status;
	}
	dprintf("P202:SD7 controller ready\n");

	*busCookie = bus;
	return B_OK;
}


mmc_bus_interface gSDHCIFDTDeviceModule = {
	.info = {
		.info = {
			.name = SDHCI_FDT_MMC_BUS_MODULE_NAME,
		},

		.init_driver = init_bus_fdt,
		.uninit_driver = uninit_bus,
		.device_removed = bus_removed,
	},

	.set_clock = set_clock,
	.execute_command = execute_command,
	.do_io = do_io,
	.set_scan_semaphore = set_scan_semaphore,
	.set_bus_width = set_bus_width,
	.terminate_bus = terminate_bus,
	.set_card_type = set_card_type,
};
