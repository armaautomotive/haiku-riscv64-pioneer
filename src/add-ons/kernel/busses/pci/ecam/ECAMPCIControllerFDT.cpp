/*
 * Copyright 2022, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "ECAMPCIController.h"

#include <AutoDeleterDrivers.h>
#include <arch/generic/msi.h>
#include <interrupts.h>
#include <util/AutoLock.h>

#include <string.h>
#include <new>


static const uint64 kSg2042RootPortConfigOffset = 0x00200000;
static const uint64 kSg2042LocalManagementOffset = 0x00100000;
static const uint32 kSg2042RootBarConfig = 0x001e0000;
static const uint32 kSg2042VendorId = 0x1e30;
static const uint32 kSg2042DeviceId = 0x2042;

static const uint32 kCadenceOutboundRegionStride = 0x20;
static const uint32 kCadenceOutboundPciAddress0 = 0x00;
static const uint32 kCadenceOutboundPciAddress1 = 0x04;
static const uint32 kCadenceOutboundDescriptor0 = 0x08;
static const uint32 kCadenceOutboundDescriptor1 = 0x0c;
static const uint32 kCadenceOutboundCpuAddress0 = 0x18;
static const uint32 kCadenceOutboundCpuAddress1 = 0x1c;
static const uint32 kCadenceDescriptorMemory = 0x2;
static const uint32 kCadenceDescriptorIo = 0x6;
static const uint32 kCadenceDescriptorHardcodedRequester = 1 << 23;
static const uint32 kCadenceInboundNoBarAddress0 = 0x0810;
static const uint32 kCadenceInboundNoBarAddress1 = 0x0814;
static const uint32 kSg2042InboundAddressBits = 48;

static const phys_addr_t kSg2042TopIntcPage = 0x7030010000;
static const uint32 kSg2042TopIntcStatusOffset = 0x2e0;
static const uint32 kSg2042TopIntcClearOffset = 0x304;
static const uint64 kSg2042TopIntcSetAddress = 0x7030010300;
static const int32 kSg2042TopIntcPlicBase = 64;
static const uint32 kSg2042TopIntcVectorCount = 32;


class Sg2042MsiInterruptController final : public MSIInterface {
public:
	status_t Init()
	{
		if (fInitialized)
			return B_OK;

		fRegsArea.SetTo(map_physical_memory("SG2042 PCIe top interrupt controller",
			kSg2042TopIntcPage, B_PAGE_SIZE, B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void**)&fRegs));
		if (fRegsArea.Get() < B_OK)
			return fRegsArea.Get();

		status_t status = allocate_io_interrupt_vectors(kSg2042TopIntcVectorCount,
			&fMsiStartIrq, INTERRUPT_TYPE_IRQ);
		if (status != B_OK)
			return status;

		for (uint32 i = 0; i < kSg2042TopIntcVectorCount; i++) {
			fInterrupts[i].controller = this;
			fInterrupts[i].index = i;
			status = install_io_interrupt_handler(kSg2042TopIntcPlicBase + i,
				InterruptReceived, &fInterrupts[i], 0);
			if (status != B_OK)
				return status;
		}

		fInitialized = true;
		msi_set_interface(this);
		fPollThread = spawn_kernel_thread(PollThread, "SG2042 MSI status poll",
			B_REAL_TIME_DISPLAY_PRIORITY, this);
		if (fPollThread < B_OK)
			return fPollThread;
		resume_thread(fPollThread);
		dprintf("P253:SG2042 MSI interface %p vtable %p, vectors %" B_PRId32
			"-%" B_PRId32 " route through PLIC 64-95\n", this,
			*(void**)this, fMsiStartIrq,
			fMsiStartIrq + kSg2042TopIntcVectorCount - 1);
		return B_OK;
	}

	status_t AllocateVectors(uint32 count, uint32& startVector, uint64& address,
		uint32& data) final
	{
		if (count != 1)
			return B_UNSUPPORTED;

		MutexLocker locker(&fLock);
		for (uint32 i = 0; i < kSg2042TopIntcVectorCount; i++) {
			if ((fAllocated & (1U << i)) != 0)
				continue;
			fAllocated |= 1U << i;
			startVector = fMsiStartIrq + i;
			address = kSg2042TopIntcSetAddress;
			data = 1U << i;
			dprintf("P249:SG2042 MSI allocate index %" B_PRIu32 " irq %" B_PRIu32
				" data %#" B_PRIx32 "\n", i, startVector, data);
			return B_OK;
		}
		return B_NO_MEMORY;
	}

	void FreeVectors(uint32 count, uint32 startVector) final
	{
		MutexLocker locker(&fLock);
		while (count-- > 0 && startVector >= (uint32)fMsiStartIrq) {
			uint32 index = startVector++ - fMsiStartIrq;
			if (index < kSg2042TopIntcVectorCount)
				fAllocated &= ~(1U << index);
		}
	}

private:
	struct InterruptContext {
		Sg2042MsiInterruptController* controller;
		uint32 index;
	};

	static int32 InterruptReceived(void* argument)
	{
		InterruptContext* context = (InterruptContext*)argument;
		uint32 mask = 1U << context->index;
		uint32 status = Sg2042MmioRead((vuint32*)(context->controller->fRegs
			+ kSg2042TopIntcStatusOffset));
		if ((status & mask) == 0)
			return B_UNHANDLED_INTERRUPT;
		Sg2042MmioWrite((vuint32*)(context->controller->fRegs
			+ kSg2042TopIntcClearOffset), mask);
		if (context->controller->fPhysicalInterrupts++ < 8) {
			dprintf("P254:SG2042 MSI physical PLIC index %" B_PRIu32
				" status %#" B_PRIx32 "\n", context->index, status);
		}
		return io_interrupt_handler(context->controller->fMsiStartIrq
			+ context->index, false);
	}

	static int32 PollThread(void* argument)
	{
		Sg2042MsiInterruptController* controller
			= (Sg2042MsiInterruptController*)argument;
		while (true) {
			uint32 status = Sg2042MmioRead((vuint32*)(controller->fRegs
				+ kSg2042TopIntcStatusOffset));
			while (status != 0) {
				uint32 index = __builtin_ctz(status);
				uint32 mask = 1U << index;
				Sg2042MmioWrite((vuint32*)(controller->fRegs
					+ kSg2042TopIntcClearOffset), mask);
				uint32 statusAfterClear = Sg2042MmioRead(
					(vuint32*)(controller->fRegs + kSg2042TopIntcStatusOffset));
				if (controller->fPolledInterrupts++ < 8) {
					dprintf("P258:SG2042 MSI poll index %" B_PRIu32
						" status %#" B_PRIx32 " after-clear %#" B_PRIx32 "\n",
						index, status, statusAfterClear);
				}
				cpu_status interruptState = disable_interrupts();
				io_interrupt_handler(controller->fMsiStartIrq + index, false);
				restore_interrupts(interruptState);
				status &= ~mask;
			}
			snooze(1000);
		}
		return B_OK;
	}

	struct mutex fLock = MUTEX_INITIALIZER("SG2042 MSI allocation");
	AreaDeleter fRegsArea;
	uint8 volatile* fRegs{};
	int32 fMsiStartIrq{-1};
	uint32 fAllocated{};
	InterruptContext fInterrupts[kSg2042TopIntcVectorCount]{};
	thread_id fPollThread{-1};
	uint32 fPhysicalInterrupts{};
	uint32 fPolledInterrupts{};
	bool fInitialized{};
};


static Sg2042MsiInterruptController* sSg2042MsiController;


static uint32
Sg2042OutboundAddressBits(uint64 size)
{
	uint32 bits = 8;
	uint64 aperture = 1ULL << bits;
	while (aperture < size && bits < 63) {
		bits++;
		aperture <<= 1;
	}
	return bits;
}


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
		fEndBus = busEnd;
		dprintf("  bus-range: %" B_PRIu32 " - %" B_PRIu32 "\n", busBeg, busEnd);
	}
	if (fIsSg2042 && fStartBus != 0 && fStartBus != 0xc0) {
		// The Pioneer USB tree is domain 3 (hardware buses c0-ff).  Other
		// nonzero roots may have their links down and return zero-filled config
		// completions, which the generic PCI manager mistakes for endpoints.
		dprintf("P250:SG2042 skipping inactive root bus %#" B_PRIx8 "\n",
			fStartBus);
		return B_UNSUPPORTED;
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
		if (fStartBus == 0xc0) {
			// Cadence link 1 shares its controller register block with link 0.
			// Its sole FDT reg entry is the config aperture; Linux derives the
			// controller address as the paired link-0 base plus 8 MiB.
			if (!fdtModule->get_reg(fdtDev, 0, &regs, &fRegsLen))
				return B_ERROR;
			fRegsPhysical = regs;
			controllerRegs = 0x7062800000ULL;
			dprintf("P251:SG2042 link 1 config %#" B_PRIx64
				" shared controller %#" B_PRIx64 "\n", fRegsPhysical,
				controllerRegs);
		} else {
			if (!fdtModule->get_reg(fdtDev, 0, &regs, &fControllerRegsLen))
				return B_ERROR;
			controllerRegs = regs;
			if (!fdtModule->get_reg(fdtDev, 1, &regs, &fRegsLen)) {
				dprintf("P233:SG2042 controller has no separate register aperture\n");
				return B_UNSUPPORTED;
			}
			fRegsPhysical = regs;
		}

		fControllerRegsLen = B_PAGE_SIZE;
		fControllerRegsArea.SetTo(map_physical_memory("SG2042 PCIe root config",
			controllerRegs + kSg2042RootPortConfigOffset, B_PAGE_SIZE,
			B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
			(void**)&fControllerRegs));
		CHECK_RET(fControllerRegsArea.Get());
		dprintf("P237:SG2042 root config page mapped at %#" B_PRIx64 "\n",
			controllerRegs + kSg2042RootPortConfigOffset);

		fLmRegsArea.SetTo(map_physical_memory("SG2042 PCIe local management",
			controllerRegs + kSg2042LocalManagementOffset, B_PAGE_SIZE,
			B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
			(void**)&fLmRegs));
		CHECK_RET(fLmRegsArea.Get());
		dprintf("P238:SG2042 local management page mapped\n");
		uint32 linkState = Sg2042MmioRead((vuint32*)fLmRegs);
		dprintf("P242:SG2042 link state %#" B_PRIx32 "\n", linkState);

		// Match the root-complex initialization performed by Sophgo's Linux
		// driver before it attempts any root-port configuration-space reads.
		Sg2042MmioWrite((vuint32*)(fLmRegs + 0x0300), kSg2042RootBarConfig);
		dprintf("P238:SG2042 root BAR policy initialized\n");
		Sg2042MmioWrite((vuint32*)(fLmRegs + 0x0044),
			kSg2042VendorId | (kSg2042VendorId << 16));
		dprintf("P238:SG2042 vendor and subsystem initialized\n");
		Sg2042MmioWrite((vuint32*)(fControllerRegs + 0x0000),
			kSg2042VendorId | (kSg2042DeviceId << 16));
		dprintf("P238:SG2042 root vendor and device initialized\n");
		Sg2042MmioWrite((vuint32*)(fControllerRegs + 0x0008), (uint32)0x06040000);
		dprintf("P238:SG2042 root class initialized\n");
		Sg2042MmioWrite((vuint32*)(fControllerRegs + 0x0018),
			((uint32)fEndBus << 16) | ((uint32)(fStartBus + 1) << 8) | fStartBus);
		dprintf("P249:SG2042 root hardware buses %#" B_PRIx8 " - %#" B_PRIx8
			" - %#" B_PRIx8 " initialized\n", fStartBus, fStartBus + 1, fEndBus);

		fAtRegsArea.SetTo(map_physical_memory("SG2042 PCIe translation registers",
			controllerRegs + 0x00400000, B_PAGE_SIZE, B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void**)&fAtRegs));
		CHECK_RET(fAtRegsArea.Get());
		dprintf("P233:SG2042 translation page mapped\n");

		// Match Sophgo's Cadence host initialization: Root Port BAR0 and BAR1
		// are disabled, so all endpoint DMA and MSI writes depend on the
		// no-BAR-match inbound translation entry.  Firmware state is not a
		// stable interface and may leave this entry disabled after a cold boot.
		uint32 oldInboundLow = Sg2042MmioRead(
			(vuint32*)(fAtRegs + kCadenceInboundNoBarAddress0));
		uint32 oldInboundHigh = Sg2042MmioRead(
			(vuint32*)(fAtRegs + kCadenceInboundNoBarAddress1));
		Sg2042MmioWrite((vuint32*)(fAtRegs + kCadenceInboundNoBarAddress0),
			(kSg2042InboundAddressBits - 1) & 0x3f);
		Sg2042MmioWrite((vuint32*)(fAtRegs + kCadenceInboundNoBarAddress1),
			(uint32)0);
		dprintf("P255:SG2042 inbound no-BAR %#" B_PRIx32 ":%#" B_PRIx32
			" -> 0:%#" B_PRIx32 " (%" B_PRIu32 " bits)\n", oldInboundHigh,
			oldInboundLow, (kSg2042InboundAddressBits - 1) & 0x3f,
			kSg2042InboundAddressBits);

		// Region zero is the dynamically selected configuration aperture.  Map
		// every PCI host range in the following Cadence outbound regions so BAR
		// accesses are translated just as they are by the Linux host driver.
		for (int32 index = 0; index < fResourceRanges.Count(); index++) {
			const pci_resource_range& range = fResourceRanges[index];
			uint32 region = index + 1;
			uint32 offset = region * kCadenceOutboundRegionStride;
			uint32 bits = Sg2042OutboundAddressBits(range.size);
			uint32 pciLow = ((uint32)range.pci_address & 0xffffff00)
				| ((bits - 1) & 0x3f);
			uint32 cpuLow = ((uint32)range.host_address & 0xffffff00)
				| ((bits - 1) & 0x3f);
			uint32 descriptor = kCadenceDescriptorHardcodedRequester
				| (range.type == B_IO_PORT
					? kCadenceDescriptorIo : kCadenceDescriptorMemory);

			Sg2042MmioWrite((vuint32*)(fAtRegs + offset
				+ kCadenceOutboundPciAddress0), pciLow);
			Sg2042MmioWrite((vuint32*)(fAtRegs + offset
				+ kCadenceOutboundPciAddress1),
				(uint32)(range.pci_address >> 32));
			Sg2042MmioWrite((vuint32*)(fAtRegs + offset
				+ kCadenceOutboundDescriptor0), descriptor);
			Sg2042MmioWrite((vuint32*)(fAtRegs + offset
				+ kCadenceOutboundDescriptor1), (uint32)fStartBus);
			Sg2042MmioWrite((vuint32*)(fAtRegs + offset
				+ kCadenceOutboundCpuAddress0), cpuLow);
			Sg2042MmioWrite((vuint32*)(fAtRegs + offset
				+ kCadenceOutboundCpuAddress1),
				(uint32)(range.host_address >> 32));
			dprintf("P247:SG2042 outbound %" B_PRIu32 " PCI %#" B_PRIx64
				" host %#" B_PRIx64 " size %#" B_PRIx64 " type %s\n",
				region, range.pci_address, range.host_address, range.size,
				range.type == B_IO_PORT ? "io" : "memory");
		}

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
		Sg2042MmioWrite((vuint32*)(fAtRegs + 0x0004), (uint32)0);
		dprintf("P234:SG2042 PCI address high set\n");
		Sg2042MmioWrite((vuint32*)(fAtRegs + 0x000c), (uint32)fStartBus);
		dprintf("P234:SG2042 descriptor high set\n");
		Sg2042MmioWrite((vuint32*)(fAtRegs + 0x0018),
			((uint32)fRegsPhysical & 0xffffff00) | 11);
		dprintf("P234:SG2042 CPU address low set\n");
		Sg2042MmioWrite((vuint32*)(fAtRegs + 0x001c),
			(uint32)(fRegsPhysical >> 32));
		dprintf("P234:SG2042 CPU address high set\n");
		dprintf("P235:SG2042 PCIe buses %#" B_PRIx8 "+: regs %#" B_PRIx64
			", config %#" B_PRIxPHYSADDR "\n", fStartBus, controllerRegs,
			fRegsPhysical);

		if (fStartBus == 0xc0) {
			// Pioneer domain 3 contains an ASMedia switch, NVMe, and the xHCI
			// controller used by the external keyboard and mouse.  Firmware leaves
			// parts of this tree unassigned for Haiku, so establish the known bridge
			// routing before the PCI bus manager discovers it.  Bus values passed
			// here are local; the config accessor adds the c0 hardware bus base.
			WriteConfig(0, 0, 0, PCI_primary_bus, 4, 0x000a0100);
			WriteConfig(0, 0, 0, PCI_io_base, 4, 0x00002101);
			WriteConfig(0, 0, 0, PCI_memory_base, 4, 0xf040f000);
			WriteConfig(0, 0, 0, PCI_prefetchable_memory_base, 4, 0x0001fff1);
			WriteConfig(0, 0, 0, PCI_io_base_upper16, 4, 0x00c000c0);
			WriteConfig(0, 0, 0, PCI_command, 2, 0x0007);

			WriteConfig(1, 0, 0, PCI_primary_bus, 4, 0x000a0201);
			WriteConfig(1, 0, 0, PCI_io_base, 4, 0x00002101);
			WriteConfig(1, 0, 0, PCI_memory_base, 4, 0xf040f000);
			WriteConfig(1, 0, 0, PCI_prefetchable_memory_base, 4, 0x0001fff1);
			WriteConfig(1, 0, 0, PCI_io_base_upper16, 4, 0x00c000c0);
			WriteConfig(1, 0, 0, PCI_command, 2, 0x0007);

			// Switch port 0 leads to the NVMe controller on bus 3.  Its 16 KiB
			// BAR resides at PCI 0xf0020000 (host 0x4cf0020000), inside a 1 MiB
			// bridge memory window matching the working Linux configuration.
			WriteConfig(2, 0, 0, PCI_primary_bus, 4, 0x00030302);
			WriteConfig(2, 0, 0, PCI_io_base, 4, 0x000001f1);
			WriteConfig(2, 0, 0, PCI_memory_base, 4, 0xf000f000);
			WriteConfig(2, 0, 0, PCI_prefetchable_memory_base, 4, 0x0001fff1);
			WriteConfig(2, 0, 0, PCI_command, 2, 0x0006);

			WriteConfig(3, 0, 0, PCI_base_registers, 4, 0xf0020004);
			WriteConfig(3, 0, 0, PCI_base_registers + 4, 4, 0);
			WriteConfig(3, 0, 0, PCI_rom_base, 4, 0xf0000000);
			WriteConfig(3, 0, 0, PCI_command, 2, 0x0406);

			// Keep all three bus numbers local here.  The config accessor
			// translates 2/4/4 to the hardware's c2/c4/c4 numbering.
			WriteConfig(2, 4, 0, PCI_primary_bus, 4, 0x00040402);
			WriteConfig(2, 4, 0, PCI_io_base, 4, 0x000001f1);
			WriteConfig(2, 4, 0, PCI_memory_base, 4, 0xf010f010);
			WriteConfig(2, 4, 0, PCI_prefetchable_memory_base, 4, 0x0001fff1);
			WriteConfig(2, 4, 0, PCI_command, 2, 0x0006);

			WriteConfig(4, 0, 0, PCI_base_registers, 4, 0xf0100004);
			WriteConfig(4, 0, 0, PCI_base_registers + 4, 4, 0);
			WriteConfig(4, 0, 0, PCI_command, 2, 0x0406);

			// Switch port 12 leads to the JMicron JMB585 AHCI controller on
			// bus 8. Linux assigns a 1 MiB bridge window at PCI 0xf0300000
			// and the controller's 8 KiB ABAR at PCI 0xf0310000 (host
			// 0x4cf0310000).
			WriteConfig(2, 12, 0, PCI_primary_bus, 4, 0x00080802);
			WriteConfig(2, 12, 0, PCI_io_base, 4, 0x000001f1);
			WriteConfig(2, 12, 0, PCI_memory_base, 4, 0xf030f030);
			WriteConfig(2, 12, 0, PCI_prefetchable_memory_base, 4, 0x0001fff1);
			WriteConfig(2, 12, 0, PCI_command, 2, 0x0006);

			WriteConfig(8, 0, 0, PCI_base_registers + 5 * 4, 4, 0xf0310000);
			WriteConfig(8, 0, 0, PCI_rom_base, 4, 0xf0300000);
			WriteConfig(8, 0, 0, PCI_command, 2, 0x0406);

			dprintf("P277:SG2042 Pioneer NVMe, xHCI, and AHCI topology configured\n");
			// Kernel add-ons do not initialize C++ objects in static storage.
			// Construct this polymorphic object explicitly so its virtual table is
			// valid when generic_msi calls AllocateVectors().
			if (sSg2042MsiController == NULL) {
				sSg2042MsiController
					= new(std::nothrow) Sg2042MsiInterruptController();
				if (sSg2042MsiController == NULL)
					return B_NO_MEMORY;
			}
			CHECK_RET(sSg2042MsiController->Init());
		}
	}

	return B_OK;
}


status_t
ECAMPCIControllerFDT::Finalize()
{
	dprintf("finalize PCI controller from FDT\n");
	if (fIsSg2042 && fStartBus == 0) {
		// The Pioneer firmware does not assign endpoint resources before handing
		// control to Haiku.  Configure the board's Caicos display adapter using
		// addresses from the FDT windows.  This is deliberately hardware-specific
		// until the PCI bus manager grows a general firmware-less BAR allocator.
		uint32 vendor = gPCI->read_pci_config(1, 0, 0, PCI_vendor_id, 2);
		uint32 device = gPCI->read_pci_config(1, 0, 0, PCI_device_id, 2);
		if (vendor != 0x1002 || device != 0x6779) {
			dprintf("P247:SG2042 expected Caicos endpoint not found (%#" B_PRIx32
				":%#" B_PRIx32 ")\n", vendor, device);
			return B_OK;
		}

		// Root-port windows: I/O 0x1000-0x1fff, memory
		// 0x50000000-0x500fffff, prefetchable memory
		// 0x4100000000-0x410fffffff.  These values match the FDT resources and
		// the working Linux configuration on this Pioneer.
		gPCI->write_pci_config(0, 0, 0, PCI_io_base, 4, 0x00001111);
		gPCI->write_pci_config(0, 0, 0, PCI_memory_base, 4, 0x50005000);
		gPCI->write_pci_config(0, 0, 0, PCI_prefetchable_memory_base, 4,
			0x0ff10001);
		gPCI->write_pci_config(0, 0, 0,
			PCI_prefetchable_memory_base_upper32, 4, 0x00000041);
		gPCI->write_pci_config(0, 0, 0,
			PCI_prefetchable_memory_limit_upper32, 4, 0x00000041);
		gPCI->write_pci_config(0, 0, 0, PCI_io_base_upper16, 4, 0);

		// Function 0: 256 MiB framebuffer, 128 KiB MMIO registers, 256-byte
		// I/O aperture, and a disabled 128 KiB option ROM.
		gPCI->write_pci_config(1, 0, 0, PCI_base_registers + 0, 4, 0x0000000c);
		gPCI->write_pci_config(1, 0, 0, PCI_base_registers + 4, 4, 0x00000041);
		gPCI->write_pci_config(1, 0, 0, PCI_base_registers + 8, 4, 0x50000004);
		gPCI->write_pci_config(1, 0, 0, PCI_base_registers + 12, 4, 0);
		gPCI->write_pci_config(1, 0, 0, PCI_base_registers + 16, 4, 0x00001001);
		gPCI->write_pci_config(1, 0, 0, PCI_base_registers + 20, 4, 0);
		gPCI->write_pci_config(1, 0, 0, PCI_rom_base, 4, 0x50020000);

		uint32 command = gPCI->read_pci_config(1, 0, 0, PCI_command, 2);
		gPCI->write_pci_config(1, 0, 0, PCI_command, 2, command
			| PCI_command_io | PCI_command_memory | PCI_command_master);

		// Function 1 is the HDMI audio function sharing the same card.
		if (gPCI->read_pci_config(1, 0, 1, PCI_vendor_id, 2) == 0x1002) {
			gPCI->write_pci_config(1, 0, 1, PCI_base_registers, 4, 0x50040004);
			gPCI->write_pci_config(1, 0, 1, PCI_base_registers + 4, 4, 0);
			command = gPCI->read_pci_config(1, 0, 1, PCI_command, 2);
			gPCI->write_pci_config(1, 0, 1, PCI_command, 2, command
				| PCI_command_memory | PCI_command_master);
		}

		command = gPCI->read_pci_config(0, 0, 0, PCI_command, 2);
		gPCI->write_pci_config(0, 0, 0, PCI_command, 2, command
			| PCI_command_io | PCI_command_memory | PCI_command_master);
		dprintf("P247:SG2042 Caicos BARs assigned: FB 0x4100000000, MMIO "
			"0x50000000, IO 0x1000\n");

		// SG2042 routes PCIe through its MSI controller rather than interrupt-map.
		// Enumeration and BAR access do not depend on MSI setup.
		return B_OK;
	}
	if (fIsSg2042)
		return B_OK;

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
