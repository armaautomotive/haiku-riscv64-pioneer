/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 * Optional, root-only Pioneer experiments. No DMA or firmware writes.
 */
#include "tenstorrent_pci_probe.h"

#include <KernelExport.h>
#include <ByteOrder.h>
#include <PCI.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

int32 api_version = B_CUR_DRIVER_API_VERSION;
static pci_module_info* sPCI;
static pci_info sCard;
static char sCookie;
static sem_id sLock = -1;
static device_manager_info* sDeviceManager;
static const char* sNames[] = {TT_PCI_PROBE_DEVICE, NULL};

static inline void
PioneerIoFence()
{
#if defined(__riscv)
	asm volatile("fence iorw, iorw" ::: "memory");
#else
	__sync_synchronize();
#endif
}

static inline uint32
PioneerRead32(const volatile uint32* address)
{
	PioneerIoFence();
	uint32 value = *address;
	PioneerIoFence();
	return B_LENDIAN_TO_HOST_INT32(value);
}

static inline void
PioneerWrite32(volatile uint32* address, uint32 value)
{
	PioneerIoFence();
	*address = B_HOST_TO_LENDIAN_INT32(value);
	PioneerIoFence();
}

static bool
FindCard(pci_module_info* pci, pci_info& card)
{
	unsigned count = 0;
	pci_info info;
	for (long i = 0; pci->get_nth_pci_info(i, &info) == B_OK; i++) {
		if (info.vendor_id == 0x1e52 && info.device_id == 0xb140
			&& info.u.h0.subsystem_vendor_id == 0x1e52
			&& info.u.h0.subsystem_id == 0x0043
			&& (info.header_type & PCI_header_type_mask) == 0) {
			card = info;
			count++;
		}
	}
	return count == 1;
}

status_t
init_hardware()
{
	pci_module_info* pci;
	status_t status = get_module(B_PCI_MODULE_NAME, (module_info**)&pci);
	if (status != B_OK)
		return status;
	pci_info card;
	bool found = FindCard(pci, card);
	put_module(B_PCI_MODULE_NAME);
	return found ? B_OK : B_ENTRY_NOT_FOUND;
}

status_t
init_driver()
{
	status_t status = get_module(B_PCI_MODULE_NAME, (module_info**)&sPCI);
	if (status != B_OK)
		return status;
	if (!FindCard(sPCI, sCard)) {
		put_module(B_PCI_MODULE_NAME);
		sPCI = NULL;
		return B_ENTRY_NOT_FOUND;
	}
	sLock = create_sem(1, "P100A config address probe");
	if (sLock < B_OK) {
		put_module(B_PCI_MODULE_NAME);
		sPCI = NULL;
		return sLock;
	}
	return B_OK;
}

void
uninit_driver()
{
	if (sLock >= B_OK)
		delete_sem(sLock);
	if (sPCI != NULL)
		put_module(B_PCI_MODULE_NAME);
	sPCI = NULL;
	if (sDeviceManager != NULL)
		put_module(B_DEVICE_MANAGER_MODULE_NAME);
	sDeviceManager = NULL;
}

const char**
publish_devices()
{
	return sNames;
}

static status_t
ProbeOpen(const char* name, uint32 flags, void** cookie)
{
	if (geteuid() != 0 || (flags & O_RWMASK) != O_RDWR)
		return B_NOT_ALLOWED;
	if (strcmp(name, TT_PCI_PROBE_DEVICE) != 0)
		return B_ENTRY_NOT_FOUND;
	*cookie = &sCookie;
	return B_OK;
}

static status_t ProbeClose(void*) { return B_OK; }
static status_t ProbeFree(void*) { return B_OK; }

struct ConfigIO {
	uint32 Read(uint16 offset, uint8 size)
	{
		return sPCI->read_pci_config(sCard.bus, sCard.device, sCard.function,
			offset, size);
	}
	void Write(uint16 offset, uint8 size, uint32 value)
	{
		sPCI->write_pci_config(sCard.bus, sCard.device, sCard.function,
			offset, size, value);
	}
	uint64 Size(unsigned bar)
	{
		return (uint64)sCard.u.h0.base_register_sizes[bar]
			| ((uint64)sCard.u.h0.base_register_sizes[bar + 1] << 32);
	}
};

struct PioneerMmioIO : ConfigIO {
	const tt_pioneer_profile& profile;
	area_id area = -1;
	const volatile uint32* nodeID = NULL;

	PioneerMmioIO(const tt_pioneer_profile& p) : profile(p) {}
	PioneerMmioIO(const PioneerMmioIO&) = delete;
	PioneerMmioIO& operator=(const PioneerMmioIO&) = delete;
	~PioneerMmioIO() { if (area >= B_OK) delete_area(area); }
	const tt_pioneer_profile& Profile() { return profile; }
	uint32 CardRead(uint16 offset, uint8 size) { return Read(offset, size); }
	void CardWrite(uint16 offset, uint8 size, uint32 value) { Write(offset, size, value); }
	uint32 RootRead(uint16 offset, uint8 size)
	{
		return sPCI->read_pci_config(profile.root_bus, 0, 0, offset, size);
	}
	void RootWrite(uint16 offset, uint8 size, uint32 value)
	{
		sPCI->write_pci_config(profile.root_bus, 0, 0, offset, size, value);
	}
	bool Quiet()
	{
		if (RootRead(0x10, 4) != 0 || RootRead(0x14, 4) != 0
			|| (RootRead(0x38, 4) & 1) != 0) {
			dprintf("PIONEER-P100A: root BAR/ROM guard refused\n");
			return false;
		}
		pci_info info;
		for (long i = 0; sPCI->get_nth_pci_info(i, &info) == B_OK; i++) {
			if (info.bus != profile.root_bus || info.device != 0 || info.function != 0)
				continue;
			bool quiet = info.vendor_id == 0x1e30 && info.device_id == 0x2042
				&& (info.header_type & PCI_header_type_mask) == 1
				&& info.u.h1.base_register_sizes[0] == 0
				&& info.u.h1.base_register_sizes[1] == 0;
			if (!quiet)
				dprintf("PIONEER-P100A: root cached BAR-size guard refused\n");
			return quiet;
		}
		return false;
	}
	bool Map()
	{
		// Hardware fact from pinned Apache-2.0 tt-umd Blackhole constants:
		// BAR0 NoC2AXI node ID is at 0x1fd04044 (x = 2 or 11, y = 0).
		// Independent implementation: map only this page, no register writes.
		void* address = NULL;
		area = map_physical_memory("P100A NoC ID one-shot",
			profile.bar0_host_start + 0x1fd04000, B_PAGE_SIZE,
			B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_UNCACHED_MEMORY, &address);
		if (area < B_OK) {
			dprintf("PIONEER-P100A: NoC ID map failed %" B_PRId32 "\n", area);
			return false;
		}
		nodeID = (const volatile uint32*)((const uint8*)address + 0x44);
		return true;
	}
	bool Unmap()
	{
		if (area < B_OK)
			return true;
		status_t status = delete_area(area);
		if (status != B_OK)
			return false;
		area = -1;
		nodeID = NULL;
		return true;
	}
	uint32 ReadNodeId()
	{
		return PioneerRead32(nodeID);
	}
	bool AfterNodeId(uint32) { return true; }
};

struct PioneerArcIO : PioneerMmioIO {
	tt_arc_scratch_result& arc;
	area_id tlbArea = -1, scratchArea = -1;
	volatile uint32* tlb = NULL;
	const volatile uint32* scratch = NULL;
	bool dirty = false;

	PioneerArcIO(const tt_pioneer_profile& p, tt_arc_scratch_result& result)
		: PioneerMmioIO(p), arc(result) {}
	~PioneerArcIO()
	{
		// Only virtual mappings here: the outer transaction has already
		// disabled PCI decoding. Never attempt MMIO from this destructor.
		if (scratchArea >= B_OK) delete_area(scratchArea);
		if (tlbArea >= B_OK) delete_area(tlbArea);
	}
	bool Map()
	{
		if (!PioneerMmioIO::Map())
			return false;
		void* address = NULL;
		tlbArea = map_physical_memory("P100A fixed TLB control",
			profile.bar0_host_start + 0x1fc00000, B_PAGE_SIZE,
			B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA
				| B_UNCACHED_MEMORY, &address);
		if (tlbArea < B_OK)
			return false;
		tlb = (volatile uint32*)((uint8*)address + 0x96c);
		scratchArea = map_physical_memory("P100A ARC scratch read-only",
			profile.bar0_host_start + 0x19230000, B_PAGE_SIZE,
			B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_UNCACHED_MEMORY, &address);
		if (scratchArea < B_OK)
			return false;
		scratch = (const volatile uint32*)address;
		return true;
	}
	uint32 TlbRead(unsigned i) { return i < 3 ? PioneerRead32(tlb + i) : UINT32_MAX; }
	void TlbWrite(unsigned i, uint32 value)
	{
		if (i >= 3) return;
		dirty = true;
		PioneerWrite32(tlb + i, value);
	}
	uint32 ScratchRead(unsigned n)
	{
		if (n != 2 && n != 4 && n != 11 && n != 12 && n != 13)
			return UINT32_MAX;
		return PioneerRead32(scratch + (0x400 + n * 4) / 4);
	}
	bool AfterNodeId(uint32 id)
	{
		if ((id & 0xfff) != 2)
			return false;
		dprintf("PIONEER-P100A: ARC scratch attempt NoC0 (8,0), TLB 201, read-only target\n");
		return BlackholeArcScratchProbe(*this, arc);
	}
	bool Unmap()
	{
		bool okay = true;
		if (scratchArea >= B_OK) {
			if (delete_area(scratchArea) == B_OK) scratchArea = -1;
			else okay = false;
		}
		if (dirty && !arc.tlb_restored) {
			// One bounded restore retry, while the outer transaction still
			// has memory decoding enabled. Never write ARC scratch itself.
			for (unsigned i = 0; i < 3; i++) TlbWrite(i, arc.tlb_before[i]);
			arc.tlb_restored = 1;
			for (unsigned i = 0; i < 3; i++) {
				arc.tlb_after[i] = TlbRead(i);
				arc.tlb_restored &= arc.tlb_after[i] == arc.tlb_before[i];
			}
		}
		if (dirty && !arc.tlb_restored) okay = false;
		if (tlbArea >= B_OK) {
			if (delete_area(tlbArea) == B_OK) tlbArea = -1;
			else okay = false;
		}
		return PioneerMmioIO::Unmap() && okay;
	}
};

struct PioneerTelemetryIO : PioneerArcIO {
	tt_arc_telemetry_result& telemetry;
	area_id csmArea = -1;
	const volatile uint32* csm = NULL;
	bool telemetryAttempted = false;

	PioneerTelemetryIO(const tt_pioneer_profile& p, tt_arc_scratch_result& arcResult,
		tt_arc_telemetry_result& result)
		: PioneerArcIO(p, arcResult), telemetry(result) {}
	~PioneerTelemetryIO() { if (csmArea >= B_OK) delete_area(csmArea); }
	bool Map()
	{
		if (!PioneerArcIO::Map()) return false;
		void* address = NULL;
		csmArea = map_physical_memory("P100A bounded telemetry read-only",
			profile.bar0_host_start + 0x19200000, 0x20000,
			B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_UNCACHED_MEMORY, &address);
		if (csmArea < B_OK) return false;
		csm = (const volatile uint32*)address;
		return true;
	}
	uint32 CsmRead(uint32 address)
	{
		if (!BlackholeTelemetryRange(address, 4)) return UINT32_MAX;
		return PioneerRead32(csm + (address - 0x10000000) / 4);
	}
	bool AfterNodeId(uint32 id)
	{
		if (!PioneerArcIO::AfterNodeId(id)) return false;
		telemetryAttempted = true;
		dprintf("PIONEER-P100A: bounded CSM telemetry attempt, whitelisted tags only\n");
		return BlackholeArcTelemetryProbe(*this, arc, telemetry);
	}
	bool Unmap()
	{
		bool okay = true;
		if (csmArea >= B_OK) {
			if (delete_area(csmArea) == B_OK) csmArea = -1;
			else okay = false;
		}
		if (telemetryAttempted && !telemetry.tlb_restored) {
			for (unsigned i = 0; i < 3; i++) TlbWrite(i, telemetry.tlb_before[i]);
			telemetry.tlb_restored = 1;
			for (unsigned i = 0; i < 3; i++) {
				telemetry.tlb_after[i] = TlbRead(i);
				telemetry.tlb_restored &= telemetry.tlb_after[i] == telemetry.tlb_before[i];
			}
		}
		if (telemetryAttempted && !telemetry.tlb_restored) okay = false;
		return PioneerArcIO::Unmap() && okay;
	}
};

static status_t
ProbeControl(void* cookie, uint32 operation, void* data, size_t length)
{
	if (geteuid() != 0 || cookie != &sCookie)
		return B_NOT_ALLOWED;
	if (operation == TT_PCI_PROBE_PIONEER_TELEMETRY) {
		if (data == NULL || length != sizeof(tt_pioneer_telemetry_result)) return B_BAD_VALUE;
		tt_pioneer_telemetry_result result = {};
		status_t status = user_memcpy(data, &result, sizeof(result));
		if (status != B_OK) return status;
		status = acquire_sem(sLock);
		if (status != B_OK) return status;
		status = sPCI->reserve_device(sCard.bus, sCard.device, sCard.function,
			TT_PCI_PROBE_DEVICE, &sCookie);
		if (status != B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager == NULL)
			status = get_module(B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&sDeviceManager);
		tt_pioneer_profile profile = {};
		if (status == B_OK) status = GetPioneerPciProfile(sDeviceManager, sCard, profile);
		profile.status = status;
		result.control.pci.version = result.control.arc.version = result.telemetry.version = 1;
		result.control.pci.profile_status = status;
		result.control.pci.profile_stage = profile.stage;
		result.control.arc.tlb_restored = result.telemetry.tlb_restored = 1;
		if (status == B_OK) {
			PioneerTelemetryIO io(profile, result.control.arc, result.telemetry);
			BlackholePioneerMmioProbe(io, result.control.pci);
		}
		sPCI->unreserve_device(sCard.bus, sCard.device, sCard.function, TT_PCI_PROBE_DEVICE, &sCookie);
		release_sem(sLock);
		dprintf("PIONEER-P100A: telemetry state %" B_PRIu32 " version %#" B_PRIx32
			" count %" B_PRIu32 " mask %#" B_PRIx32 " TLB restored %" B_PRIu32
			" PCI restored %" B_PRIu32 " (no firmware/DMA writes)\n",
			result.telemetry.state, result.telemetry.table_version, result.telemetry.entry_count,
			result.telemetry.present_mask, result.telemetry.tlb_restored, result.control.pci.restored);
		return user_memcpy(data, &result, sizeof(result));
	}
	if (operation == TT_PCI_PROBE_PIONEER_ARC_SCRATCH) {
		if (data == NULL || length != sizeof(tt_pioneer_arc_result))
			return B_BAD_VALUE;
		tt_pioneer_arc_result result = {};
		status_t status = user_memcpy(data, &result, sizeof(result));
		if (status != B_OK) return status;
		status = acquire_sem(sLock);
		if (status != B_OK) return status;
		status = sPCI->reserve_device(sCard.bus, sCard.device, sCard.function,
			TT_PCI_PROBE_DEVICE, &sCookie);
		if (status != B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager == NULL)
			status = get_module(B_DEVICE_MANAGER_MODULE_NAME,
				(module_info**)&sDeviceManager);
		tt_pioneer_profile profile = {};
		if (status == B_OK)
			status = GetPioneerPciProfile(sDeviceManager, sCard, profile);
		profile.status = status;
		result.pci.version = result.arc.version = 1;
		result.pci.profile_status = status;
		result.pci.profile_stage = profile.stage;
		result.arc.tlb_restored = 1;
		if (status == B_OK) {
			PioneerArcIO io(profile, result.arc);
			BlackholePioneerMmioProbe(io, result.pci);
		}
		sPCI->unreserve_device(sCard.bus, sCard.device, sCard.function,
			TT_PCI_PROBE_DEVICE, &sCookie);
		release_sem(sLock);
		dprintf("PIONEER-P100A: ARC scratch state %" B_PRIu32
			" TLB restored %" B_PRIu32 " PCI state %" B_PRIu32
			" restored %" B_PRIu32 " boot %08" B_PRIx32 "/%08" B_PRIx32
			" error %08" B_PRIx32 " (no firmware/DMA writes)\n",
			result.arc.state, result.arc.tlb_restored, result.pci.state,
			result.pci.restored, result.arc.boot_before, result.arc.boot_after,
			result.arc.error_status0);
		return user_memcpy(data, &result, sizeof(result));
	}
	if (operation == TT_PCI_PROBE_PIONEER_NOC_ID) {
		if (data == NULL || length != sizeof(tt_pioneer_mmio_result))
			return B_BAD_VALUE;
		tt_pioneer_mmio_result result = {};
		status_t status = user_memcpy(data, &result, sizeof(result));
		if (status != B_OK)
			return status;
		status = acquire_sem(sLock);
		if (status != B_OK)
			return status;
		status = sPCI->reserve_device(sCard.bus, sCard.device, sCard.function,
			TT_PCI_PROBE_DEVICE, &sCookie);
		if (status != B_OK) {
			release_sem(sLock);
			return status;
		}
		if (sDeviceManager == NULL)
			status = get_module(B_DEVICE_MANAGER_MODULE_NAME,
				(module_info**)&sDeviceManager);
		tt_pioneer_profile profile = {};
		if (status == B_OK)
			status = GetPioneerPciProfile(sDeviceManager, sCard, profile);
		profile.status = status;
		result.version = 1;
		result.profile_status = status;
		result.profile_stage = profile.stage;
		if (status == B_OK) {
			PioneerMmioIO io(profile);
			BlackholePioneerMmioProbe(io, result);
		}
		sPCI->unreserve_device(sCard.bus, sCard.device, sCard.function,
			TT_PCI_PROBE_DEVICE, &sCookie);
		release_sem(sLock);
		dprintf("PIONEER-P100A: NoC ID probe state %" B_PRIu32
			" stage %" B_PRIu32 " restored %" B_PRIu32 " values %08" B_PRIx32
			"/%08" B_PRIx32 "/%08" B_PRIx32 " (DMA never enabled)\n",
			result.state, result.stage, result.restored,
			result.node_id[0], result.node_id[1], result.node_id[2]);
		return user_memcpy(data, &result, sizeof(result));
	}
	if (operation == TT_PCI_PROBE_PIONEER_PROFILE) {
		if (data == NULL || length != sizeof(tt_pioneer_profile))
			return B_BAD_VALUE;
		status_t status = acquire_sem(sLock);
		if (status != B_OK)
			return status;
		tt_pioneer_profile profile = {};
		if (sDeviceManager == NULL)
			status = get_module(B_DEVICE_MANAGER_MODULE_NAME,
				(module_info**)&sDeviceManager);
		if (status == B_OK)
			status = GetPioneerPciProfile(sDeviceManager, sCard, profile);
		profile.status = status;
		release_sem(sLock);
		return user_memcpy(data, &profile, sizeof(profile));
	}
	if (operation != TT_PCI_PROBE_BAR4_ADDRESS || data == NULL
		|| length != sizeof(tt_bar4_probe_result))
		return B_BAD_VALUE;
	tt_bar4_probe_result result = {};
	status_t status = user_memcpy(data, &result, sizeof(result));
	if (status != B_OK)
		return status;
	status = acquire_sem(sLock);
	if (status != B_OK)
		return status;
	status = sPCI->reserve_device(sCard.bus, sCard.device, sCard.function,
		TT_PCI_PROBE_DEVICE, &sCookie);
	if (status != B_OK) {
		release_sem(sLock);
		return status;
	}
	ConfigIO io;
	BlackholeBar4AddressProbe(io, result);
	sPCI->unreserve_device(sCard.bus, sCard.device, sCard.function,
		TT_PCI_PROBE_DEVICE, &sCookie);
	release_sem(sLock);
	dprintf("PIONEER-P100A: BAR4 address probe state %" B_PRIu32
		" trial %08" B_PRIx32 ":%08" B_PRIx32 " command %04" B_PRIx32
		" -> %04" B_PRIx32 " (no MMIO/DMA)\n", result.state,
		result.trial[1], result.trial[0], result.command_before,
		result.command_after);
	return user_memcpy(data, &result, sizeof(result));
}

static status_t
ProbeData(void*, off_t, void*, size_t* length)
{
	*length = 0;
	return B_NOT_ALLOWED;
}

static status_t
ProbeWrite(void*, off_t, const void*, size_t* length)
{
	*length = 0;
	return B_NOT_ALLOWED;
}

device_hooks*
find_device(const char* name)
{
	static device_hooks hooks = {
		ProbeOpen, ProbeClose, ProbeFree, ProbeControl, ProbeData, ProbeWrite,
		NULL, NULL, NULL, NULL
	};
	return strcmp(name, TT_PCI_PROBE_DEVICE) == 0 ? &hooks : NULL;
}
