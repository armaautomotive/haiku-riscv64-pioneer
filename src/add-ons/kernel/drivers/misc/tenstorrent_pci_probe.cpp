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
// Protected by sLock. A module reload is NOT cold recovery; after this latch
// is set, the board must be cold-recovered before changing/reloading drivers.
static bool sColdRecoveryRequired = false;
static device_manager_info* sDeviceManager;
static const char* sNames[] = {TT_PCI_PROBE_DEVICE, NULL};

// PIONEER_PROBE_LOCK_BEGIN
static status_t
AcquireProbeLock()
{
	status_t status = acquire_sem(sLock);
	if (status == B_OK && sColdRecoveryRequired) {
		release_sem(sLock);
		return B_DEV_NOT_READY;
	}
	return status;
}
// PIONEER_PROBE_LOCK_END

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
	dprintf("PIONEER-P100A: probe hardware PCI module begin\n");
	pci_module_info* pci;
	status_t status = get_module(B_PCI_MODULE_NAME, (module_info**)&pci);
	dprintf("PIONEER-P100A: probe hardware PCI module status %" B_PRId32 "\n", status);
	if (status != B_OK)
		return status;
	pci_info card;
	bool found = FindCard(pci, card);
	dprintf("PIONEER-P100A: probe hardware scan found %d\n", found);
	put_module(B_PCI_MODULE_NAME);
	return found ? B_OK : B_ENTRY_NOT_FOUND;
}

status_t
init_driver()
{
	dprintf("PIONEER-P100A: probe driver init begin\n");
	status_t status = get_module(B_PCI_MODULE_NAME, (module_info**)&sPCI);
	dprintf("PIONEER-P100A: probe driver PCI module status %" B_PRId32 "\n", status);
	if (status != B_OK)
		return status;
	if (!FindCard(sPCI, sCard)) {
		put_module(B_PCI_MODULE_NAME);
		sPCI = NULL;
		return B_ENTRY_NOT_FOUND;
	}
	dprintf("PIONEER-P100A: probe driver scan complete\n");
	sLock = create_sem(1, "P100A config address probe");
	if (sLock < B_OK) {
		put_module(B_PCI_MODULE_NAME);
		sPCI = NULL;
		return sLock;
	}
	dprintf("PIONEER-P100A: probe driver init complete\n");
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
	dprintf("PIONEER-P100A: probe open ready\n");
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

struct PioneerTensixIO : PioneerTelemetryIO {
	tt_tensix_reset_result& tensix;
	area_id resetArea = -1;
	const volatile uint32* reset = NULL;
	bool resetAttempted = false;

	PioneerTensixIO(const tt_pioneer_profile& p, tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t, tt_tensix_reset_result& r)
		: PioneerTelemetryIO(p, a, t), tensix(r) {}
	~PioneerTensixIO() { if (resetArea >= B_OK) delete_area(resetArea); }
	bool Map()
	{
		if (!PioneerTelemetryIO::Map()) return false;
		void* address = NULL;
		// TLB 201 -> local base ffa00000; SOFT_RESET is ffb121b0.
		resetArea = map_physical_memory("P100A Tensix reset read-only",
			profile.bar0_host_start + 0x19312000, B_PAGE_SIZE,
			B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_UNCACHED_MEMORY, &address);
		if (resetArea < B_OK) return false;
		reset = (const volatile uint32*)((const uint8*)address + 0x1b0);
		return true;
	}
	uint32 NiuRead()
	{
		// Same already-mapped PCIe NIU page as NODE_ID; NIU_CFG_0 is +100.
		return PioneerRead32((const volatile uint32*)((const uint8*)nodeID + 0xbc));
	}
	uint32 ResetRead() { return PioneerRead32(reset); }
	bool AfterNodeId(uint32 id)
	{
		if (!PioneerTelemetryIO::AfterNodeId(id)) return false;
		resetAttempted = true;
		dprintf("PIONEER-P100A: enabled physical Tensix (1,2), read-only reset-state attempt\n");
		return BlackholeTensixResetProbe(*this, telemetry, tensix);
	}
	bool Unmap()
	{
		bool okay = true;
		if (resetArea >= B_OK) {
			if (delete_area(resetArea) == B_OK) resetArea = -1;
			else okay = false;
		}
		if (resetAttempted && !tensix.tlb_restored) {
			for (unsigned i = 0; i < 3; i++) TlbWrite(i, tensix.tlb_before[i]);
			tensix.tlb_restored = 1;
			for (unsigned i = 0; i < 3; i++) {
				tensix.tlb_after[i] = TlbRead(i);
				tensix.tlb_restored &= tensix.tlb_after[i] == tensix.tlb_before[i];
			}
		}
		if (resetAttempted && !tensix.tlb_restored) okay = false;
		return PioneerTelemetryIO::Unmap() && okay;
	}
};

struct PioneerL1IO : PioneerTensixIO {
	tt_l1_scratch_result& l1;
	area_id l1Area = -1;
	volatile uint32* memory = NULL;
	bool l1Attempted = false;

	PioneerL1IO(const tt_pioneer_profile& p, tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t, tt_tensix_reset_result& c, tt_l1_scratch_result& r)
		: PioneerTensixIO(p, a, t, c), l1(r) {}
	~PioneerL1IO() { if (l1Area >= B_OK) delete_area(l1Area); }
	bool Map()
	{
		if (!PioneerTensixIO::Map()) return false;
		void* address = NULL;
		// TLB 200 BAR0 base 19000000; final 64 bytes of 180000-byte L1.
		l1Area = map_physical_memory("P100A stopped-core L1 scratch",
			profile.bar0_host_start + 0x1917f000, B_PAGE_SIZE,
			B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA
				| B_UNCACHED_MEMORY, &address);
		if (l1Area < B_OK) return false;
		memory = (volatile uint32*)((uint8*)address + 0xfc0);
		return true;
	}
	uint32 L1TlbRead(unsigned slot, unsigned word)
	{
		if (slot > 1 || word > 2) return UINT32_MAX;
		return PioneerRead32(tlb - 3 + slot * 3 + word);
	}
	void L1TlbWrite(unsigned slot, unsigned word, uint32 value)
	{
		if (slot > 1 || word > 2) return;
		PioneerWrite32(tlb - 3 + slot * 3 + word, value);
	}
	uint32 L1Read(unsigned word) { return word < TT_L1_WORDS ? PioneerRead32(memory + word) : UINT32_MAX; }
	void L1Write(unsigned word, uint32 value) { if (word < TT_L1_WORDS) PioneerWrite32(memory + word, value); }
	bool AfterNodeId(uint32 id)
	{
		if (!PioneerTensixIO::AfterNodeId(id)) return false;
		l1Attempted = true;
		dprintf("PIONEER-P100A: stopped Tensix (1,2), 64-byte reversible L1 data test\n");
		return BlackholeL1ScratchProbe(*this, tensix, l1);
	}
	bool Unmap()
	{
		bool okay = true;
		if (l1Area >= B_OK) {
			if (delete_area(l1Area) == B_OK) l1Area = -1;
			else okay = false;
		}
		if (l1Attempted && !l1.tlb_restored) {
			for (unsigned s = 0; s < 2; s++)
				for (unsigned w = 0; w < 3; w++) L1TlbWrite(s, w, l1.tlb_before[s][w]);
			l1.tlb_restored = 1;
			for (unsigned s = 0; s < 2; s++) {
				for (unsigned w = 0; w < 3; w++) {
					l1.tlb_after[s][w] = L1TlbRead(s, w);
					l1.tlb_restored &= l1.tlb_after[s][w] == l1.tlb_before[s][w];
				}
			}
		}
		if (l1Attempted && (!l1.tlb_restored || !l1.data_restored)) okay = false;
		return PioneerTensixIO::Unmap() && okay;
	}
};

struct PioneerBriscIO : PioneerTensixIO {
	tt_brisc_smoke_result& brisc;
	area_id briscAreas[5] = {-1, -1, -1, -1, -1};
	volatile uint32* boot = NULL;
	volatile uint32* code = NULL;
	volatile uint32* results = NULL;
	volatile uint32* resetWrite = NULL;
	const volatile uint32* pc = NULL;
	bool briscAttempted = false;

	PioneerBriscIO(const tt_pioneer_profile& p, tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t, tt_tensix_reset_result& c, tt_brisc_smoke_result& r)
		: PioneerTensixIO(p, a, t, c), brisc(r) {}
	~PioneerBriscIO()
	{
		// Never perform MMIO in a destructor or retry code/reset restoration.
		for (unsigned i = 0; i < 5; i++)
			if (briscAreas[i] >= B_OK) delete_area(briscAreas[i]);
	}
	bool Map()
	{
		if (!PioneerTensixIO::Map()) return false;
		const uint32 offsets[] = {0x19000000, 0x19070000, 0x1917f000, 0x19312000, 0x19313000};
		void* pages[5] = {};
		for (unsigned i = 0; i < 5; i++) {
			uint32 protection = B_KERNEL_READ_AREA | B_UNCACHED_MEMORY;
			if (i != 4) protection |= B_KERNEL_WRITE_AREA;
			briscAreas[i] = map_physical_memory("P100A fixed BRISC smoke page",
				profile.bar0_host_start + offsets[i], B_PAGE_SIZE,
				B_ANY_KERNEL_ADDRESS, protection, &pages[i]);
			if (briscAreas[i] < B_OK) return false;
		}
		boot = (volatile uint32*)pages[0]; code = (volatile uint32*)pages[1];
		results = (volatile uint32*)((uint8*)pages[2] + 0xfc0);
		resetWrite = (volatile uint32*)((uint8*)pages[3] + 0x1b0);
		pc = (const volatile uint32*)((const uint8*)pages[4] + 0x138);
		return true;
	}
	uint32 BriscTlbRead(unsigned slot, unsigned word)
	{
		return slot < 2 && word < 3 ? PioneerRead32(tlb - 3 + slot * 3 + word) : UINT32_MAX;
	}
	void BriscTlbWrite(unsigned slot, unsigned word, uint32 value)
	{
		if (slot < 2 && word < 3) PioneerWrite32(tlb - 3 + slot * 3 + word, value);
	}
	volatile uint32* ImagePointer(unsigned index)
	{
		if (index == 0) return boot;
		if (index <= TT_BRISC_CODE_WORDS) return code + index - 1;
		if (index < TT_BRISC_IMAGE_WORDS) return results + index - 20;
		return NULL;
	}
	uint32 ImageRead(unsigned index)
	{
		volatile uint32* address = ImagePointer(index);
		return address != NULL ? PioneerRead32(address) : UINT32_MAX;
	}
	void ImageWrite(unsigned index, uint32 value)
	{
		volatile uint32* address = ImagePointer(index);
		if (address != NULL) PioneerWrite32(address, value);
	}
	void ResetWrite(uint32 value)
	{
		if (value == 0x47000 || value == 0x47800) PioneerWrite32(resetWrite, value);
	}
	uint32 PcRead() { return PioneerRead32(pc); }
	void Pause() { spin(100); } // fixed 100 us; policy bounds the iteration count
	bool AfterNodeId(uint32 id)
	{
		if (!PioneerTensixIO::AfterNodeId(id)) return false;
		briscAttempted = true;
		dprintf("PIONEER-P100A: fixed BRISC checksum attempt, sole tile (1,2), no DMA/flash\n");
		return BlackholeBriscSmokeProbe(*this, tensix, brisc);
	}
	bool Unmap()
	{
		bool okay = true;
		for (unsigned i = 0; i < 5; i++) {
			if (briscAreas[i] < B_OK) continue;
			if (delete_area(briscAreas[i]) == B_OK) briscAreas[i] = -1;
			else okay = false;
		}
		boot = code = results = resetWrite = NULL; pc = NULL;
		if (briscAttempted && !brisc.tlb_restored) {
			// Only TLB control cleanup here. Never retry a core start/stop or
			// rewrite code after the policy has declared execution uncertain.
			for (unsigned s = 0; s < 2; s++)
				for (unsigned w = 0; w < 3; w++) BriscTlbWrite(s, w, brisc.tlb_before[s][w]);
			brisc.tlb_restored = 1;
			for (unsigned s = 0; s < 2; s++) {
				for (unsigned w = 0; w < 3; w++) {
					brisc.tlb_after[s][w] = BriscTlbRead(s, w);
					brisc.tlb_restored &= brisc.tlb_after[s][w] == brisc.tlb_before[s][w];
				}
			}
		}
		if (briscAttempted && (!brisc.l1_restored || !brisc.tlb_restored || brisc.recovery_required)) okay = false;
		return PioneerTensixIO::Unmap() && okay;
	}
};

static status_t
ProbeControl(void* cookie, uint32 operation, void* data, size_t length)
{
	if (geteuid() != 0 || cookie != &sCookie)
		return B_NOT_ALLOWED;
	if (operation == TT_PCI_PROBE_PIONEER_BRISC_SUPPORT) {
		if (data == NULL || length != sizeof(tt_brisc_support)) return B_BAD_VALUE;
		status_t status = AcquireProbeLock();
		if (status != B_OK) return status;
		const tt_brisc_support support = {1, TT_BRISC_CODE_WORDS, TT_BRISC_IMAGE_WORDS, 0x70000, 0x17ffc0, 0x70048};
		release_sem(sLock);
		// Loaded-module ABI/image metadata only: no PCI or MMIO access.
		return user_memcpy(data, &support, sizeof(support));
	}
	if (operation == TT_PCI_PROBE_PIONEER_BRISC_SMOKE) {
		if (data == NULL || length != sizeof(tt_pioneer_brisc_result)) return B_BAD_VALUE;
		tt_pioneer_brisc_result result = {};
		status_t status = user_memcpy(data, &result, sizeof(result));
		if (status != B_OK) return status;
		status = AcquireProbeLock();
		if (status != B_OK) return status;
		status = sPCI->reserve_device(sCard.bus, sCard.device, sCard.function, TT_PCI_PROBE_DEVICE, &sCookie);
		if (status != B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager == NULL) status = get_module(B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&sDeviceManager);
		tt_pioneer_profile profile = {};
		if (status == B_OK) status = GetPioneerPciProfile(sDeviceManager, sCard, profile);
		profile.status = status;
		tt_pioneer_arc_result& control = result.control.control.control;
		tt_arc_telemetry_result& telemetry = result.control.control.telemetry;
		tt_tensix_reset_result& tensix = result.control.tensix;
		control.pci.version = control.arc.version = telemetry.version = tensix.version = result.brisc.version = 1;
		control.pci.profile_status = status; control.pci.profile_stage = profile.stage;
		control.arc.tlb_restored = telemetry.tlb_restored = tensix.tlb_restored = 1;
		result.brisc.l1_restored = result.brisc.tlb_restored = 1;
		if (status == B_OK) {
			PioneerBriscIO io(profile, control.arc, telemetry, tensix, result.brisc);
			BlackholePioneerMmioProbe(io, control.pci);
		}
		if (result.brisc.recovery_required || control.pci.state == TT_PIONEER_MMIO_RESTORE_FAILED) {
			result.brisc.recovery_required = 1;
			sColdRecoveryRequired = true;
		}
		sPCI->unreserve_device(sCard.bus, sCard.device, sCard.function, TT_PCI_PROBE_DEVICE, &sCookie);
		release_sem(sLock);
		dprintf("PIONEER-P100A: BRISC state %" B_PRIu32 " started/completed/stopped %" B_PRIu32 "/%" B_PRIu32 "/%" B_PRIu32
			" L1/TLB/PCI restored %" B_PRIu32 "/%" B_PRIu32 "/%" B_PRIu32 " cold recovery %" B_PRIu32 "\n",
			result.brisc.state, result.brisc.started, result.brisc.completed, result.brisc.stopped,
			result.brisc.l1_restored, result.brisc.tlb_restored, control.pci.restored, result.brisc.recovery_required);
		return user_memcpy(data, &result, sizeof(result));
	}
	if (operation == TT_PCI_PROBE_PIONEER_L1_SCRATCH) {
		if (data == NULL || length != sizeof(tt_pioneer_l1_result)) return B_BAD_VALUE;
		tt_pioneer_l1_result result = {};
		status_t status = user_memcpy(data, &result, sizeof(result));
		if (status != B_OK) return status;
		status = AcquireProbeLock();
		if (status != B_OK) return status;
		status = sPCI->reserve_device(sCard.bus, sCard.device, sCard.function,
			TT_PCI_PROBE_DEVICE, &sCookie);
		if (status != B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager == NULL)
			status = get_module(B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&sDeviceManager);
		tt_pioneer_profile profile = {};
		if (status == B_OK) status = GetPioneerPciProfile(sDeviceManager, sCard, profile);
		profile.status = status;
		tt_pioneer_arc_result& control = result.control.control.control;
		tt_arc_telemetry_result& telemetry = result.control.control.telemetry;
		tt_tensix_reset_result& tensix = result.control.tensix;
		control.pci.version = control.arc.version = telemetry.version = tensix.version = result.l1.version = 1;
		control.pci.profile_status = status;
		control.pci.profile_stage = profile.stage;
		control.arc.tlb_restored = telemetry.tlb_restored = tensix.tlb_restored = 1;
		result.l1.tlb_restored = result.l1.data_restored = 1;
		if (status == B_OK) {
			PioneerL1IO io(profile, control.arc, telemetry, tensix, result.l1);
			BlackholePioneerMmioProbe(io, control.pci);
		}
		sPCI->unreserve_device(sCard.bus, sCard.device, sCard.function, TT_PCI_PROBE_DEVICE, &sCookie);
		release_sem(sLock);
		dprintf("PIONEER-P100A: L1 state %" B_PRIu32 " passes %" B_PRIu32 " mismatches %#" B_PRIx32
			" data/TLB/PCI restored %" B_PRIu32 "/%" B_PRIu32 "/%" B_PRIu32 " (no core start/DMA/flash)\n",
			result.l1.state, result.l1.passes, result.l1.mismatch_mask,
			result.l1.data_restored, result.l1.tlb_restored, control.pci.restored);
		return user_memcpy(data, &result, sizeof(result));
	}
	if (operation == TT_PCI_PROBE_PIONEER_TENSIX_RESET) {
		if (data == NULL || length != sizeof(tt_pioneer_tensix_result)) return B_BAD_VALUE;
		tt_pioneer_tensix_result result = {};
		status_t status = user_memcpy(data, &result, sizeof(result));
		if (status != B_OK) return status;
		status = AcquireProbeLock();
		if (status != B_OK) return status;
		status = sPCI->reserve_device(sCard.bus, sCard.device, sCard.function,
			TT_PCI_PROBE_DEVICE, &sCookie);
		if (status != B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager == NULL)
			status = get_module(B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&sDeviceManager);
		tt_pioneer_profile profile = {};
		if (status == B_OK) status = GetPioneerPciProfile(sDeviceManager, sCard, profile);
		profile.status = status;
		tt_pioneer_arc_result& control = result.control.control;
		control.pci.version = control.arc.version = result.control.telemetry.version = result.tensix.version = 1;
		control.pci.profile_status = status;
		control.pci.profile_stage = profile.stage;
		control.arc.tlb_restored = result.control.telemetry.tlb_restored = result.tensix.tlb_restored = 1;
		if (status == B_OK) {
			PioneerTensixIO io(profile, control.arc, result.control.telemetry, result.tensix);
			BlackholePioneerMmioProbe(io, control.pci);
		}
		sPCI->unreserve_device(sCard.bus, sCard.device, sCard.function, TT_PCI_PROBE_DEVICE, &sCookie);
		release_sem(sLock);
		dprintf("PIONEER-P100A: Tensix reset state %" B_PRIu32 " raw %08" B_PRIx32
			" NIU %#" B_PRIx32 " TLB restored %" B_PRIu32 " PCI restored %" B_PRIu32 " (read-only core)\n",
			result.tensix.state, result.tensix.reset[0], result.tensix.niu_config,
			result.tensix.tlb_restored, control.pci.restored);
		return user_memcpy(data, &result, sizeof(result));
	}
	if (operation == TT_PCI_PROBE_PIONEER_TELEMETRY) {
		if (data == NULL || length != sizeof(tt_pioneer_telemetry_result)) return B_BAD_VALUE;
		tt_pioneer_telemetry_result result = {};
		status_t status = user_memcpy(data, &result, sizeof(result));
		if (status != B_OK) return status;
		status = AcquireProbeLock();
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
		status = AcquireProbeLock();
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
		status = AcquireProbeLock();
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
		dprintf("PIONEER-P100A: profile semaphore begin\n");
		status_t status = AcquireProbeLock();
		dprintf("PIONEER-P100A: profile semaphore status %" B_PRId32 "\n", status);
		if (status != B_OK)
			return status;
		tt_pioneer_profile profile = {};
		if (sDeviceManager == NULL) {
			dprintf("PIONEER-P100A: profile device manager module begin\n");
			status = get_module(B_DEVICE_MANAGER_MODULE_NAME,
				(module_info**)&sDeviceManager);
			dprintf("PIONEER-P100A: profile device manager module status %" B_PRId32 "\n", status);
		}
		if (status == B_OK)
			status = GetPioneerPciProfile(sDeviceManager, sCard, profile);
		dprintf("PIONEER-P100A: profile returned status %" B_PRId32 " stage %u\n", status, profile.stage);
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
	status = AcquireProbeLock();
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
