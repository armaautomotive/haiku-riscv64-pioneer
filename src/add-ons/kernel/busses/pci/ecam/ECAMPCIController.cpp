/*
 * Copyright 2022, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "ECAMPCIController.h"
#include <acpi.h>

#include <AutoDeleterDrivers.h>

#include "acpi_irq_routing_table.h"

#include <string.h>
#include <new>


static const uint32 kSg2042AtPciAddress0 = 0x0000;
static const uint32 kSg2042AtDescriptor0 = 0x0008;


static uint32
TranslateSg2042BridgeBusNumbers(uint32 value, uint16 offset, uint8 size,
	uint8 startBus, uint8 accessedBus, bool toHardware)
{
	if (startBus == 0)
		return value;

	for (uint8 i = 0; i < size; i++) {
		uint16 registerOffset = offset + i;
		if (registerOffset < PCI_primary_bus || registerOffset > PCI_subordinate_bus)
			continue;

		uint32 shift = i * 8;
		uint8 bus = (value >> shift) & 0xff;
		if (toHardware) {
			// Zero normally means an unconfigured bridge.  The root bridge is
			// the exception: its local primary bus zero is the domain's global
			// starting bus number.
			if (bus != 0 || (accessedBus == 0 && registerOffset == PCI_primary_bus))
				bus += startBus;
		} else if (bus != 0 && bus >= startBus) {
			bus -= startBus;
		}
		value = (value & ~(0xffU << shift)) | ((uint32)bus << shift);
	}
	return value;
}


static bool
IsSg2042Compatible(const char* compatible)
{
	return strcmp(compatible, "sophgo,sg2042-pcie-host") == 0
		|| strcmp(compatible, "sophgo,cdns-pcie-host") == 0;
}


//#pragma mark - driver


float
ECAMPCIController::SupportsDevice(device_node* parent)
{
	const char* bus;
	status_t status = gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false);
	if (status < B_OK)
		return -1.0f;

	if (strcmp(bus, "fdt") == 0) {
		const char* compatible;
		status = gDeviceManager->get_attr_string(parent, "fdt/compatible", &compatible, false);
		if (status < B_OK)
			return -1.0f;

		if (strcmp(compatible, "pci-host-ecam-generic") != 0
			&& !IsSg2042Compatible(compatible)) {
			return 0.0f;
		}

		return 1.0f;
	}

	if (strcmp(bus, "acpi") == 0) {
		const char* hid;
		if (gDeviceManager->get_attr_string(parent, ACPI_DEVICE_HID_ITEM, &hid, false) < B_OK)
			return -1.0f;

		if (strcmp(hid, "PNP0A03") != 0 && strcmp(hid, "PNP0A08") != 0)
			return 0.0f;

		return 1.0f;
	}

	return 0.0f;
}


status_t
ECAMPCIController::RegisterDevice(device_node* parent)
{
	const char* compatible = NULL;
	gDeviceManager->get_attr_string(parent, "fdt/compatible", &compatible, false);
	const char* prettyName = compatible != NULL && IsSg2042Compatible(compatible)
		? "SG2042 Cadence PCIe Host Controller" : "ECAM PCI Host Controller";

	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {.string = prettyName} },
		{ B_DEVICE_FIXED_CHILD, B_STRING_TYPE, {.string = "bus_managers/pci/root/driver_v1"} },
		{}
	};

	return gDeviceManager->register_node(parent, ECAM_PCI_DRIVER_MODULE_NAME, attrs, NULL, NULL);
}


#if !defined(ECAM_PCI_CONTROLLER_NO_INIT)
status_t
ECAMPCIController::InitDriver(device_node* node, ECAMPCIController*& outDriver)
{
	dprintf("P232:ECAM init host\n");
	DeviceNodePutter<&gDeviceManager> parentNode(gDeviceManager->get_parent_node(node));

	ObjectDeleter<ECAMPCIController> driver;

	const char* bus;
	CHECK_RET(gDeviceManager->get_attr_string(parentNode.Get(), B_DEVICE_BUS, &bus, false));
	if (strcmp(bus, "fdt") == 0) {
		driver.SetTo(new(std::nothrow) ECAMPCIControllerFDT());
		if (!driver.IsSet())
			return B_NO_MEMORY;
		const char* compatible;
		if (gDeviceManager->get_attr_string(parentNode.Get(), "fdt/compatible", &compatible,
				false) == B_OK) {
			driver->fIsSg2042 = IsSg2042Compatible(compatible);
		}
	} else if (strcmp(bus, "acpi") == 0)
		driver.SetTo(new(std::nothrow) ECAMPCIControllerACPI());
	else
		return B_ERROR;

	if (!driver.IsSet())
		return B_NO_MEMORY;

	driver->fNode = node;

	CHECK_RET(driver->ReadResourceInfo());
	outDriver = driver.Detach();

	dprintf("-ECAMPCIController::InitDriver()\n");
	return B_OK;
}


void
ECAMPCIController::UninitDriver()
{
	delete this;
}
#endif


/** Compute the virtual address for accessing a PCI ECAM register.
 *
 * \returns NULL if the address is out of bounds.
 */
addr_t
ECAMPCIController::ConfigAddress(uint8 bus, uint8 device, uint8 function, uint16 offset)
{
	PciAddressEcam address {
		.offset = offset,
		.function = function,
		.device = device,
		.bus = bus
	};
	if ((ROUNDDOWN(address.val, 4) + 4) > fRegsLen)
		return 0;

	return (addr_t)fRegs + address.val;
}


//#pragma mark - PCI controller


status_t
ECAMPCIController::ReadConfig(uint8 bus, uint8 device, uint8 function,
	uint16 offset, uint8 size, uint32& value)
{
	if (fIsSg2042)
		return ReadSg2042Config(bus, device, function, offset, size, value);

	addr_t address = ConfigAddress(bus, device, function, offset);
	if (address == 0)
		return ERANGE;

	switch (size) {
		case 1: value = *(vuint8*)address; break;
		case 2: value = *(vuint16*)address; break;
		case 4: value = *(vuint32*)address; break;
		default:
			return B_BAD_VALUE;
	}

	return B_OK;
}


status_t
ECAMPCIController::WriteConfig(uint8 bus, uint8 device, uint8 function,
	uint16 offset, uint8 size, uint32 value)
{
	if (fIsSg2042)
		return WriteSg2042Config(bus, device, function, offset, size, value);

	addr_t address = ConfigAddress(bus, device, function, offset);
	if (address == 0)
		return ERANGE;

	switch (size) {
		case 1: *(vuint8*)address = value; break;
		case 2: *(vuint16*)address = value; break;
		case 4: *(vuint32*)address = value; break;
		default:
			return B_BAD_VALUE;
	}

	return B_OK;
}


status_t
ECAMPCIController::ReadSg2042Config(uint8 bus, uint8 device, uint8 function,
	uint16 offset, uint8 size, uint32& value)
{
	if (size != 1 && size != 2 && size != 4)
		return B_BAD_VALUE;
	if (offset + size > 4096)
		return ERANGE;

	mutex_lock(&fLock);
	addr_t address;
	if (bus == 0) {
		// The SG2042 root port only accepts naturally aligned 32-bit accesses.
		if (device != 0 || function != 0) {
			value = size == 1 ? 0xff : size == 2 ? 0xffff : 0xffffffff;
			mutex_unlock(&fLock);
			return B_OK;
		}
		address = (addr_t)fControllerRegs + ROUNDDOWN(offset, 4);
		if (!fRootConfigReadLogged) {
			dprintf("P236:SG2042 root config read offset %#" B_PRIx16
				" address %#" B_PRIxADDR "\n", offset, address);
		}
		uint32 word = Sg2042MmioRead((vuint32*)address);
		if (!fRootConfigReadLogged) {
			dprintf("P236:SG2042 root config value %#" B_PRIx32 "\n", word);
			fRootConfigReadLogged = true;
		}
		value = word >> ((offset & 3) * 8);
		if (size == 1)
			value &= 0xff;
		else if (size == 2)
			value &= 0xffff;
		value = TranslateSg2042BridgeBusNumbers(value, offset, size,
			fStartBus, bus, false);
	} else {
		// A PCIe Root Port is a point-to-point link.  Its immediate secondary
		// bus can only contain device 0; scanning other device numbers on the
		// SG2042 Cadence window returns stale completion data and creates
		// phantom copies of the endpoint.
		if (bus == 1 && device != 0) {
			value = size == 1 ? 0xff : size == 2 ? 0xffff : 0xffffffff;
			mutex_unlock(&fLock);
			return B_OK;
		}
		uint32 devfn = (device << 3) | function;
		uint32 hardwareBus = fStartBus + bus;
		uint32 pciAddress = 11 | (devfn << 12) | (hardwareBus << 20);
		uint32 descriptor = (1 << 23) | (bus == 1 ? 0xa : 0xb);
		Sg2042MmioWrite((vuint32*)(fAtRegs + kSg2042AtPciAddress0), pciAddress);
		Sg2042MmioWrite((vuint32*)(fAtRegs + kSg2042AtDescriptor0), descriptor);

		address = (addr_t)fRegs + offset;
		switch (size) {
			case 1: value = Sg2042MmioRead((vuint8*)address); break;
			case 2: value = Sg2042MmioRead((vuint16*)address); break;
			case 4: value = Sg2042MmioRead((vuint32*)address); break;
		}
		uint8 headerType = Sg2042MmioRead((vuint8*)(fRegs + PCI_header_type));
		if ((headerType & PCI_header_type_mask) == PCI_header_type_PCI_to_PCI_bridge) {
			value = TranslateSg2042BridgeBusNumbers(value, offset, size,
				fStartBus, bus, false);
		}
		if (!fDownstreamConfigReadLogged) {
			dprintf("P242:SG2042 config bus %#" B_PRIx8 " device %#" B_PRIx8
				" function %#" B_PRIx8 " pci address %#" B_PRIx32
				" descriptor %#" B_PRIx32 " value %#" B_PRIx32 "\n", bus,
				device, function, pciAddress, descriptor, value);
			fDownstreamConfigReadLogged = true;
		}
		if (bus == 1 && device == 0 && offset == 0 && size == 2
			&& (fSg2042FunctionsLogged & (1 << function)) == 0) {
			dprintf("P246:SG2042 endpoint function %u vendor %#" B_PRIx32
				"\n", function, value);
			fSg2042FunctionsLogged |= 1 << function;
		}
	}
	mutex_unlock(&fLock);
	return B_OK;
}


status_t
ECAMPCIController::WriteSg2042Config(uint8 bus, uint8 device, uint8 function,
	uint16 offset, uint8 size, uint32 value)
{
	if (size != 1 && size != 2 && size != 4)
		return B_BAD_VALUE;
	if (offset + size > 4096)
		return ERANGE;

	mutex_lock(&fLock);
	addr_t address;
	if (bus == 0) {
		if (device != 0 || function != 0) {
			mutex_unlock(&fLock);
			return B_ENTRY_NOT_FOUND;
		}
		value = TranslateSg2042BridgeBusNumbers(value, offset, size,
			fStartBus, bus, true);
		address = (addr_t)fControllerRegs + ROUNDDOWN(offset, 4);
		if (size == 4) {
			Sg2042MmioWrite((vuint32*)address, value);
		} else {
			uint32 shift = (offset & 3) * 8;
			uint32 mask = (size == 1 ? 0xff : 0xffff) << shift;
			uint32 word = Sg2042MmioRead((vuint32*)address);
			Sg2042MmioWrite((vuint32*)address,
				(word & ~mask) | ((value << shift) & mask));
		}
	} else {
		if (bus == 1 && device != 0) {
			mutex_unlock(&fLock);
			return B_ENTRY_NOT_FOUND;
		}
		uint32 devfn = (device << 3) | function;
		uint32 hardwareBus = fStartBus + bus;
		Sg2042MmioWrite((vuint32*)(fAtRegs + kSg2042AtPciAddress0),
			11 | (devfn << 12) | (hardwareBus << 20));
		Sg2042MmioWrite((vuint32*)(fAtRegs + kSg2042AtDescriptor0),
			(uint32)((1 << 23) | (bus == 1 ? 0xa : 0xb)));

		uint8 headerType = Sg2042MmioRead((vuint8*)(fRegs + PCI_header_type));
		if ((headerType & PCI_header_type_mask) == PCI_header_type_PCI_to_PCI_bridge) {
			value = TranslateSg2042BridgeBusNumbers(value, offset, size,
				fStartBus, bus, true);
		}
		address = (addr_t)fRegs + offset;
		switch (size) {
			case 1: Sg2042MmioWrite((vuint8*)address, (uint8)value); break;
			case 2: Sg2042MmioWrite((vuint16*)address, (uint16)value); break;
			case 4: Sg2042MmioWrite((vuint32*)address, value); break;
		}
	}
	mutex_unlock(&fLock);
	return B_OK;
}


status_t
ECAMPCIController::GetMaxBusDevices(int32& count)
{
	count = 32;
	if (fIsSg2042)
		dprintf("P236:SG2042 max bus devices ready\n");
	return B_OK;
}


status_t
ECAMPCIController::ReadIrq(uint8 bus, uint8 device, uint8 function,
	uint8 pin, uint8& irq)
{
	return B_UNSUPPORTED;
}


status_t
ECAMPCIController::WriteIrq(uint8 bus, uint8 device, uint8 function,
	uint8 pin, uint8 irq)
{
	return B_UNSUPPORTED;
}


status_t
ECAMPCIController::GetRange(uint32 index, pci_resource_range* range)
{
	if (index >= (uint32)fResourceRanges.Count()) {
		if (fIsSg2042)
			dprintf("P236:SG2042 resource ranges complete\n");
		return B_BAD_INDEX;
	}

	*range = fResourceRanges[index];
	if (fIsSg2042) {
		dprintf("P236:SG2042 resource range %" B_PRIu32 " type %#" B_PRIx32
			" host %#" B_PRIxPHYSADDR " size %#" B_PRIx64 "\n", index,
			range->type, range->host_address, range->size);
	}
	return B_OK;
}
