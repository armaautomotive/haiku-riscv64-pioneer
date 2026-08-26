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

	return gDeviceManager->register_node(context->fNode,
		SDHCI_FDT_MMC_BUS_MODULE_NAME, attrs, NULL, NULL);
}


status_t
init_bus_fdt(device_node* node, void** busCookie)
{
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

	uint64 physicalAddress;
	uint64 registerSize;
	if (!fdt->get_reg(device, 0, &physicalAddress, &registerSize)
		|| registerSize < sizeof(registers)) {
		return B_BAD_DATA;
	}

	uint64 irq;
	if (!fdt->get_interrupt(device, 0, NULL, &irq) || irq > UINT8_MAX)
		return B_BAD_DATA;

	struct registers* mappedRegisters;
	area_id registersArea = map_physical_memory("FDT SDHC registers",
		physicalAddress, registerSize, B_ANY_KERNEL_BLOCK_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		(void**)&mappedRegisters);
	if (registersArea < B_OK)
		return registersArea;

	const char* compatible;
	uint32 quirks = 0;
	if (gDeviceManager->get_attr_string(fdtNode.Get(), "fdt/compatible",
			&compatible, false) == B_OK
		&& (strcmp(compatible, "bitmain,bm-sd") == 0
			|| strcmp(compatible, "sophgo,sg2042-dwcmshc") == 0)) {
		quirks |= SDHCI_QUIRK_SG2042_PHY;
	}

	// Poll until the RISC-V interrupt path for this controller is validated.
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
