/*
 * Copyright 2022, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "ECAMPCIController.h"

#include <AutoDeleterDrivers.h>

#include <string.h>


static const uint64 kSg2042RootPortConfigOffset = 0x00200000;


status_t
ECAMPCIControllerFDT::ReadResourceInfo()
{
	DeviceNodePutter<&gDeviceManager> fdtNode(gDeviceManager->get_parent_node(fNode));

	fdt_device_module_info *fdtModule;
	fdt_device* fdtDev;
	CHECK_RET(gDeviceManager->get_driver(fdtNode.Get(),
		(driver_module_info**)&fdtModule, (void**)&fdtDev));

	const void* prop;
	int propLen;

	prop = fdtModule->get_prop(fdtDev, "bus-range", &propLen);
	if (prop != NULL && propLen == 8) {
		uint32 busBeg = B_BENDIAN_TO_HOST_INT32(*((uint32*)prop + 0));
		uint32 busEnd = B_BENDIAN_TO_HOST_INT32(*((uint32*)prop + 1));
		fStartBus = busBeg;
		dprintf("  bus-range: %" B_PRIu32 " - %" B_PRIu32 "\n", busBeg, busEnd);
	}

	prop = fdtModule->get_prop(fdtDev, "ranges", &propLen);
	if (prop == NULL) {
		dprintf("  \"ranges\" property not found");
		return B_ERROR;
	}
	dprintf("  ranges:\n");
	for (uint32_t *it = (uint32_t*)prop;
			(uint8_t*)(it + 7) - (uint8_t*)prop <= propLen; it += 7) {
		dprintf("    ");
		uint32_t type      = B_BENDIAN_TO_HOST_INT32(*(it + 0));
		uint64_t childAdr  = ((uint64)B_BENDIAN_TO_HOST_INT32(*(it + 1)) << 32)
			| B_BENDIAN_TO_HOST_INT32(*(it + 2));
		uint64_t parentAdr = ((uint64)B_BENDIAN_TO_HOST_INT32(*(it + 3)) << 32)
			| B_BENDIAN_TO_HOST_INT32(*(it + 4));
		uint64_t len       = ((uint64)B_BENDIAN_TO_HOST_INT32(*(it + 5)) << 32)
			| B_BENDIAN_TO_HOST_INT32(*(it + 6));

		pci_resource_range range = {};
		range.host_address = parentAdr;
		range.pci_address = childAdr;
		range.size = len;

		if ((type & fdtPciRangePrefechable) != 0)
			range.address_type |= PCI_address_prefetchable;

		switch (type & fdtPciRangeTypeMask) {
		case fdtPciRangeIoPort:
			range.type = B_IO_PORT;
			fResourceRanges.Add(range);
			break;
		case fdtPciRangeMmio32Bit:
			range.type = B_IO_MEMORY;
			range.address_type |= PCI_address_type_32;
			fResourceRanges.Add(range);
			break;
		case fdtPciRangeMmio64Bit:
			range.type = B_IO_MEMORY;
			range.address_type |= PCI_address_type_64;
			fResourceRanges.Add(range);
			break;
		}

		switch (type & fdtPciRangeTypeMask) {
		case fdtPciRangeConfig:    dprintf("CONFIG"); break;
		case fdtPciRangeIoPort:    dprintf("IOPORT"); break;
		case fdtPciRangeMmio32Bit: dprintf("MMIO32"); break;
		case fdtPciRangeMmio64Bit: dprintf("MMIO64"); break;
		}

		dprintf(" (0x%08" B_PRIx32 "): ", type);
		dprintf("child: %08" B_PRIx64, childAdr);
		dprintf(", parent: %08" B_PRIx64, parentAdr);
		dprintf(", len: %" B_PRIx64 "\n", len);
	}

	uint64 regs = 0;
	uint64 controllerRegs = 0;
	if (fIsSg2042) {
		if (!fdtModule->get_reg(fdtDev, 0, &regs, &fControllerRegsLen))
			return B_ERROR;
		controllerRegs = regs;
		if (!fdtModule->get_reg(fdtDev, 1, &regs, &fRegsLen)) {
			dprintf("P233:SG2042 controller has no separate register aperture\n");
			return B_UNSUPPORTED;
		}
		fRegsPhysical = regs;

		fControllerRegsLen = B_PAGE_SIZE;
		fControllerRegsArea.SetTo(map_physical_memory("SG2042 PCIe root config",
			controllerRegs + kSg2042RootPortConfigOffset, B_PAGE_SIZE,
			B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
			(void**)&fControllerRegs));
		CHECK_RET(fControllerRegsArea.Get());
		dprintf("P237:SG2042 root config page mapped at %#" B_PRIx64 "\n",
			controllerRegs + kSg2042RootPortConfigOffset);

		fAtRegsArea.SetTo(map_physical_memory("SG2042 PCIe translation registers",
			controllerRegs + 0x00400000, B_PAGE_SIZE, B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void**)&fAtRegs));
		CHECK_RET(fAtRegsArea.Get());
		dprintf("P233:SG2042 translation page mapped\n");

		regs = fRegsPhysical;
	} else {
		if (!fdtModule->get_reg(fdtDev, 0, &regs, &fRegsLen))
			return B_ERROR;
		fRegsPhysical = regs;
	}

	if (fIsSg2042)
		dprintf("P234:SG2042 map config page\n");
	fRegsArea.SetTo(map_physical_memory("PCI Config MMIO", regs, fRegsLen, B_ANY_KERNEL_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void**)&fRegs));
	CHECK_RET(fRegsArea.Get());
	if (fIsSg2042)
		dprintf("P234:SG2042 config page mapped\n");

	if (fIsSg2042) {
		// Cadence outbound region 0 is reserved for PCI configuration transactions.
		*(vuint32*)(fAtRegs + 0x0004) = 0;
		dprintf("P234:SG2042 PCI address high set\n");
		*(vuint32*)(fAtRegs + 0x000c) = fStartBus;
		dprintf("P234:SG2042 descriptor high set\n");
		*(vuint32*)(fAtRegs + 0x0018)
			= ((uint32)fRegsPhysical & 0xffffff00) | 11;
		dprintf("P234:SG2042 CPU address low set\n");
		*(vuint32*)(fAtRegs + 0x001c) = fRegsPhysical >> 32;
		dprintf("P234:SG2042 CPU address high set\n");
		dprintf("P235:SG2042 PCIe buses %#" B_PRIx8 "+: regs %#" B_PRIx64
			", config %#" B_PRIxPHYSADDR "\n", fStartBus, controllerRegs,
			fRegsPhysical);
	}

	return B_OK;
}


status_t
ECAMPCIControllerFDT::Finalize()
{
	dprintf("finalize PCI controller from FDT\n");
	if (fIsSg2042) {
		// SG2042 routes PCIe through its MSI controller rather than interrupt-map.
		// Enumeration and BAR access do not depend on MSI setup.
		return B_OK;
	}

	DeviceNodePutter<&gDeviceManager> parent(gDeviceManager->get_parent_node(fNode));

	fdt_device_module_info* parentModule;
	fdt_device* parentDev;

	CHECK_RET(gDeviceManager->get_driver(parent.Get(), (driver_module_info**)&parentModule,
		(void**)&parentDev));

	struct fdt_interrupt_map* interruptMap = parentModule->get_interrupt_map(parentDev);
	parentModule->print_interrupt_map(interruptMap);

	for (int bus = 0; bus < 8; bus++) {
		// TODO: Proper multiple domain handling. (domain, bus) pair should be converted to virtual
		// bus before calling PCI module interface.
		for (int device = 0; device < 32; device++) {
			uint32 vendorID = gPCI->read_pci_config(bus, device, 0, PCI_vendor_id, 2);
			if ((vendorID != 0xffffffff) && (vendorID != 0xffff)) {
				uint32 headerType = gPCI->read_pci_config(bus, device, 0, PCI_header_type, 1);
				if ((headerType & 0x80) != 0) {
					for (int function = 0; function < 8; function++) {
						FinalizeInterrupts(parentModule, interruptMap, bus, device, function);
					}
				} else {
					FinalizeInterrupts(parentModule, interruptMap, bus, device, 0);
				}
			}
		}
	}

	return B_OK;
}


void
ECAMPCIControllerFDT::FinalizeInterrupts(fdt_device_module_info* fdtModule,
	struct fdt_interrupt_map* interruptMap, int bus, int device, int function)
{
	uint32 childAddr = ((bus & 0xff) << 16) | ((device & 0x1f) << 11) | ((function & 0x07) << 8);
	uint32 interruptPin = gPCI->read_pci_config(bus, device, function, PCI_interrupt_pin, 1);

	if (interruptPin == 0xffffffff) {
		dprintf("Error: Unable to read interrupt pin!\n");
		return;
	}

	uint32 irq = fdtModule->lookup_interrupt_map(interruptMap, childAddr, interruptPin);
	if (irq == 0xffffffff) {
		dprintf("no interrupt mapping for childAddr: (%d:%d:%d), childIrq: %d)\n",
			bus, device, function, interruptPin);
	} else {
		dprintf("configure interrupt (%d,%d,%d) --> %d\n",
			bus, device, function, irq);
		gPCI->update_interrupt_line(bus, device, function, irq);
	}
}
