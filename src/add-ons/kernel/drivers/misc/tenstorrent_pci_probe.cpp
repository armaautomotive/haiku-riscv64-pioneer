/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 * Optional, root-only Pioneer experiments. No DMA or firmware flash.
 */
#include "tenstorrent_pci_probe.h"

#include <KernelExport.h>
#include <ByteOrder.h>
#include <PCI.h>
#include <fcntl.h>
#include <stdlib.h>
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
	const bool operating;
	area_id csmArea = -1;
	const volatile uint32* csm = NULL;
	bool telemetryAttempted = false;

	PioneerTelemetryIO(const tt_pioneer_profile& p, tt_arc_scratch_result& arcResult,
		tt_arc_telemetry_result& result, bool operatingSnapshot = false)
		: PioneerArcIO(p, arcResult), telemetry(result), operating(operatingSnapshot) {}
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
		return BlackholeArcTelemetryProbe(*this, arc, telemetry, operating);
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

struct PioneerArcQueryIO : PioneerTelemetryIO {
	tt_arc_queue_result& query;
	tt_arc_clock_guard_result* const clock;
	area_id queueWriteArea = -1, msiArea = -1;
	volatile uint32* queueWrite = NULL;
	volatile uint32* msi = NULL;
	uint32 queueBase = 0, requestAddress = 0;
	bool queryAttempted = false;
	PioneerArcQueryIO(const tt_pioneer_profile& p, tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t, tt_arc_queue_result& q, tt_arc_clock_guard_result* clockGuard = NULL)
		: PioneerTelemetryIO(p,a,t), query(q), clock(clockGuard) {}
	~PioneerArcQueryIO() {
		if (queueWriteArea >= B_OK) delete_area(queueWriteArea);
		if (msiArea >= B_OK) delete_area(msiArea);
	}
	bool Map() {
		if (!PioneerTelemetryIO::Map()) return false;
		void* address = NULL;
		queueWriteArea = map_physical_memory("P100A bounded ARC host-queue writes",
			profile.bar0_host_start+0x19200000,TT_ARC_QUEUE_CSM_BYTES,B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_UNCACHED_MEMORY,&address);
		if (queueWriteArea < B_OK) return false;
		queueWrite = (volatile uint32*)address;
		msiArea = map_physical_memory("P100A inbound ARC message notification",
			profile.bar0_host_start+0x192b0000,B_PAGE_SIZE,B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_UNCACHED_MEMORY,&address);
		if (msiArea < B_OK) return false;
		msi = (volatile uint32*)address;
		return true;
	}
	uint32 CsmRead(uint32 address) {
		return BlackholeArcQueueRange(address,4) ? PioneerRead32(queueWrite+(address-0x10000000)/4) : UINT32_MAX;
	}
	void SetQueueBounds(uint32 base, uint32 request, uint32) {
		if (!BlackholeArcQueueRange(base,288) || request < base+32 || request > base+128 || (request-base-32)%32 != 0) return;
		queueBase = base; requestAddress = request;
	}
	void QueueWrite(uint32 address, uint32 value) {
		if (queueBase == 0 || !BlackholeArcQueueRange(address,4)) return;
		// Only CPU-owned request bytes and the two CPU-owned queue pointers.
		if (address != queueBase && address != queueBase+4
			&& (address < requestAddress || address >= requestAddress+32)) return;
		PioneerWrite32(queueWrite+(address-0x10000000)/4,value);
	}
	uint32 MsiStatusRead() { return PioneerRead32(msi+2); }
	void NotifyArc() { PioneerWrite32(msi,0); }
	void Pause() { spin(100); }
	bool AfterNodeId(uint32 id) {
		if (!PioneerTelemetryIO::AfterNodeId(id)) return false;
		queryAttempted = true;
		if (clock != NULL) {
			dprintf("PIONEER-P100A: normal firmware busy/idle clock cycle, no forced frequency/voltage/reset/flash/DMA\n");
			bool entered = BlackholeArcClockEnter(*this,arc,telemetry,*clock);
			bool left = BlackholeArcClockLeave(*this,arc,telemetry,*clock);
			return entered && left && clock->state == TT_ARC_CLOCK_RESTORED && !clock->recovery_required;
		}
		dprintf("PIONEER-P100A: fixed ARC GET_AICLK query, application queue only, no clock/voltage/reset/flash/DMA command\n");
		return BlackholeArcQueueProbe(*this,arc,telemetry,query);
	}
	bool Unmap() {
		bool okay = true;
		if (queryAttempted && !query.tlb_restored) {
			for (unsigned attempt = 0; attempt < 2 && !query.tlb_restored; attempt++) {
				for (unsigned w = 0; w < 3; w++) TlbWrite(w,query.tlb_before[w]);
				query.tlb_restored = 1;
				for (unsigned w = 0; w < 3; w++) { query.tlb_after[w] = TlbRead(w); query.tlb_restored &= query.tlb_after[w] == query.tlb_before[w]; }
			}
		}
		if (queryAttempted && (!query.request_restored || !query.tlb_restored || query.recovery_required)) okay = false;
		if (clock != NULL && (clock->recovery_required || !clock->restored)) okay = false;
		if (queueWriteArea >= B_OK) { if (delete_area(queueWriteArea) == B_OK) queueWriteArea = -1; else okay = false; }
		if (msiArea >= B_OK) { if (delete_area(msiArea) == B_OK) msiArea = -1; else okay = false; }
		return PioneerTelemetryIO::Unmap() && okay;
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
	const bool trisc;
	const bool backendReset;
	area_id briscAreas[5] = {-1, -1, -1, -1, -1};
	volatile uint32* boot = NULL;
	volatile uint32* code = NULL;
	volatile uint32* results = NULL;
	volatile uint32* resetWrite = NULL;
	const volatile uint32* pc = NULL;
	bool briscAttempted = false;

	PioneerBriscIO(const tt_pioneer_profile& p, tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t, tt_tensix_reset_result& c, tt_brisc_smoke_result& r, bool trisc0 = false, bool resetBackends = false)
		: PioneerTensixIO(p, a, t, c), brisc(r), trisc(trisc0), backendReset(resetBackends) {}
	~PioneerBriscIO()
	{
		// Never perform MMIO in a destructor or retry code/reset restoration.
		for (unsigned i = 0; i < 5; i++)
			if (briscAreas[i] >= B_OK) delete_area(briscAreas[i]);
	}
	bool Map()
	{
		if (!PioneerTensixIO::Map()) return false;
		const uint32 offsets[] = {trisc ? 0x19006000U : 0x19000000U, 0x19070000, 0x1917f000, 0x19312000, 0x19313000};
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
		pc = (const volatile uint32*)((const uint8*)pages[4] + (trisc ? 0x140 : 0x138));
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
		if (value == (trisc ? 0x46800 : 0x47000) || value == 0x47800 || (trisc && backendReset && value == 0x87ffff)) PioneerWrite32(resetWrite, value);
	}
	uint32 ResetEntryOverride() { return PioneerRead32(resetWrite + (0x234-0x1b0)/4); }
	uint32 PcRead() { return PioneerRead32(pc); }
	void Pause() { spin(100); } // fixed 100 us; policy bounds the iteration count
	bool AfterNodeId(uint32 id)
	{
		if (!PioneerTensixIO::AfterNodeId(id)) return false;
		briscAttempted = true;
		dprintf("PIONEER-P100A: fixed %s scalar checksum, sole tile (1,2), no coprocessor/DMA/flash\n", trisc ? "TRISC0" : "BRISC");
		return BlackholeBriscSmokeProbe(*this, tensix, brisc, trisc, backendReset);
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

struct PioneerDramRouteIO : PioneerTensixIO {
	tt_dram_route_result& dram;
	area_id dramArea = -1;
	const volatile uint32* dramNiu = NULL;
	bool dramAttempted = false;
	PioneerDramRouteIO(const tt_pioneer_profile& p, tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t, tt_tensix_reset_result& c, tt_dram_route_result& r)
		: PioneerTensixIO(p, a, t, c), dram(r) {}
	~PioneerDramRouteIO() { if (dramArea >= B_OK) delete_area(dramArea); }
	bool Map()
	{
		if (!PioneerTensixIO::Map()) return false;
		void* address = NULL;
		// TLB 201, local ffa00000 + 120000 => ffb20000. Only one NIU
		// page, read-only and uncached, not GDDR payload memory.
		dramArea = map_physical_memory("P100A fixed GDDR bank-0 NIU read-only",
			profile.bar0_host_start + 0x19320000, B_PAGE_SIZE,
			B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_UNCACHED_MEMORY, &address);
		if (dramArea < B_OK) return false;
		dramNiu = (const volatile uint32*)address;
		return true;
	}
	uint32 SourceRead(unsigned word)
	{
		uint32 offset = BlackholeDramSourceOffset(word);
		return offset == UINT32_MAX ? UINT32_MAX
			: PioneerRead32((const volatile uint32*)((const uint8*)nodeID - 0x44 + offset));
	}
	uint32 DramIdRead(unsigned word)
	{
		uint32 offset = BlackholeDramIdOffset(word);
		return offset == UINT32_MAX ? UINT32_MAX : PioneerRead32(dramNiu + offset / 4);
	}
	bool AfterNodeId(uint32 id)
	{
		if (!PioneerTensixIO::AfterNodeId(id)) return false;
		dramAttempted = true;
		dprintf("PIONEER-P100A: GDDR bank-0 NIU route proof, read-only target\n");
		return BlackholeDramRouteProbe(*this, telemetry, tensix, dram);
	}
	bool Unmap()
	{
		bool okay = true;
		if (dramArea >= B_OK) {
			if (delete_area(dramArea) == B_OK) dramArea = -1;
			else okay = false;
		}
		if (dramAttempted && !dram.tlb_restored) {
			for (unsigned i = 0; i < 3; i++) TlbWrite(i, dram.tlb_before[i]);
			dram.tlb_restored = 1;
			for (unsigned i = 0; i < 3; i++) {
				dram.tlb_after[i] = TlbRead(i);
				dram.tlb_restored &= dram.tlb_after[i] == dram.tlb_before[i];
			}
		}
		if (dramAttempted && (!dram.tlb_restored || dram.recovery_required)) okay = false;
		return PioneerTensixIO::Unmap() && okay;
	}
};

struct PioneerDramSamplesIO : PioneerTensixIO {
	tt_dram_samples_result& dram;
	const bool writeSamples;
	area_id dramArea = -1, dataArea = -1;
	const volatile uint32* dramNiu = NULL;
	volatile uint32* data = NULL;
	bool dramAttempted = false;
	PioneerDramSamplesIO(const tt_pioneer_profile& p, tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t, tt_tensix_reset_result& c, tt_dram_samples_result& r, bool writes)
		: PioneerTensixIO(p, a, t, c), dram(r), writeSamples(writes) {}
	~PioneerDramSamplesIO() {
		if (dataArea >= B_OK) delete_area(dataArea);
		if (dramArea >= B_OK) delete_area(dramArea);
	}
	bool Map()
	{
		if (!PioneerTensixIO::Map()) return false;
		void* address = NULL;
		dramArea = map_physical_memory("P100A enabled GDDR NIU read-only",
			profile.bar0_host_start + 0x19320000, B_PAGE_SIZE,
			B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_UNCACHED_MEMORY, &address);
		if (dramArea < B_OK) return false;
		dramNiu = (const volatile uint32*)address;
		if (writeSamples) {
			dataArea = map_physical_memory("P100A fixed reversible GDDR samples",
				profile.bar0_host_start + 0x19000000, B_PAGE_SIZE,
				B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_UNCACHED_MEMORY, &address);
			if (dataArea < B_OK) return false;
			data = (volatile uint32*)address;
		}
		return true;
	}
	uint32 SourceRead(unsigned word) {
		uint32 offset = BlackholeDramSourceOffset(word);
		return offset == UINT32_MAX ? UINT32_MAX
			: PioneerRead32((const volatile uint32*)((const uint8*)nodeID - 0x44 + offset));
	}
	uint32 DramIdRead(unsigned word) {
		uint32 offset = BlackholeDramIdOffset(word);
		return offset == UINT32_MAX ? UINT32_MAX : PioneerRead32(dramNiu + offset / 4);
	}
	uint32 DramTlbRead(unsigned slot, unsigned word) {
		return slot < 2 && word < 3 ? PioneerRead32(tlb - 3 + slot * 3 + word) : UINT32_MAX;
	}
	void DramTlbWrite(unsigned slot, unsigned word, uint32 value) {
		if (slot < 2 && word < 3) PioneerWrite32(tlb - 3 + slot * 3 + word, value);
	}
	uint32 DramRead(unsigned word) { return data != NULL && word < TT_DRAM_WORDS ? PioneerRead32(data + word) : UINT32_MAX; }
	void DramWrite(unsigned word, uint32 value) { if (data != NULL && word < TT_DRAM_WORDS) PioneerWrite32(data + word, value); }
	bool AfterNodeId(uint32 id) {
		if (!PioneerTensixIO::AfterNodeId(id)) return false;
		dramAttempted = true;
		dprintf("PIONEER-P100A: seven-bank DRAM %s transaction, no core start/DMA/flash\n", writeSamples ? "reversible sample" : "read-only route");
		return BlackholeDramSamplesProbe(*this, telemetry, tensix, writeSamples, dram);
	}
	bool Unmap()
	{
		bool okay = true;
		if (dataArea >= B_OK) {
			if (delete_area(dataArea) == B_OK) dataArea = -1;
			else okay = false;
		}
		if (dramArea >= B_OK) {
			if (delete_area(dramArea) == B_OK) dramArea = -1;
			else okay = false;
		}
		if (dramAttempted && !dram.tlb_restored) {
			for (unsigned slot = 0; slot < 2; slot++) for (unsigned word = 0; word < 3; word++) DramTlbWrite(slot,word,dram.tlb_before[slot][word]);
			dram.tlb_restored = 1;
			for (unsigned slot = 0; slot < 2; slot++) for (unsigned word = 0; word < 3; word++) {
				dram.tlb_after[slot][word] = DramTlbRead(slot,word);
				dram.tlb_restored &= dram.tlb_after[slot][word] == dram.tlb_before[slot][word];
			}
		}
		// Never restore payload in cleanup after ownership/route uncertainty.
		if (dramAttempted && (!dram.data_restored || !dram.tlb_restored || dram.recovery_required)) okay = false;
		return PioneerTensixIO::Unmap() && okay;
	}
};

struct PioneerMatmulIO : PioneerDramSamplesIO {
	tt_tensix_matmul_result& math;
	const bool configSmoke;
	const unsigned sfpuSmoke;
	tt_arc_queue_result unusedQuery = {};
	PioneerArcQueryIO clockIO;
	tt_arc_clock_guard_result* const clock;
	area_id mathAreas[5] = {-1,-1,-1,-1,-1};
	volatile uint32* entry = NULL;
	volatile uint32* code = NULL;
	volatile uint32* tail = NULL;
	volatile uint32* controls = NULL;
	const volatile uint32* pc = NULL;
	bool mathAttempted = false;
	uint32* savedWords = NULL;
	const tt_tensix_tile_data* const tileInput;
	uint32* const tileOutput;
	PioneerMatmulIO(const tt_pioneer_profile& p, tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t, tt_tensix_reset_result& c, tt_dram_samples_result& d, tt_tensix_matmul_result& m,
		bool configOnly = false, tt_arc_clock_guard_result* clockGuard = NULL, unsigned sfpuOnly = 0,
		const tt_tensix_tile_data* input = NULL, uint32* output = NULL)
		: PioneerDramSamplesIO(p,a,t,c,d,false), math(m), configSmoke(configOnly), sfpuSmoke(sfpuOnly),
		clockIO(p,a,t,clockGuard != NULL ? clockGuard->last : unusedQuery,clockGuard), clock(clockGuard),
		tileInput(input), tileOutput(output) {}
	~PioneerMatmulIO() { free(savedWords); for (unsigned i = 0; i < 5; i++) if (mathAreas[i] >= B_OK) delete_area(mathAreas[i]); }
	bool Map()
	{
		if (!PioneerDramSamplesIO::Map()) return false;
		savedWords = (uint32*)malloc(BlackholeTensixImageWords(sfpuSmoke)*sizeof(uint32));
		if (savedWords == NULL) return false;
		const uint32 offsets[] = {0x19006000,0x19070000,0x1917f000,0x19312000,0x19313000};
		void* pages[5] = {};
		for (unsigned i = 0; i < 5; i++) {
			uint32 protection = B_KERNEL_READ_AREA | B_UNCACHED_MEMORY;
			if (i != 4) protection |= B_KERNEL_WRITE_AREA;
			mathAreas[i] = map_physical_memory("P100A fixed TRISC0 matrix-test page",
				profile.bar0_host_start + offsets[i],B_PAGE_SIZE,B_ANY_KERNEL_ADDRESS,protection,&pages[i]);
			if (mathAreas[i] < B_OK) return false;
		}
		entry = (volatile uint32*)pages[0]; code = (volatile uint32*)pages[1];
		tail = (volatile uint32*)pages[2]; controls = (volatile uint32*)pages[3];
		pc = (const volatile uint32*)((const uint8*)pages[4]+0x140); // TRISC0 snapshot
		// Additional virtual views only; no nested PCI configuration transaction.
		if (clock != NULL && !clockIO.Map()) return false;
		return true;
	}
	volatile uint32* MathPointer(unsigned index) {
		if (index >= BlackholeTensixImageWords(sfpuSmoke)) return NULL;
		uint32 address = BlackholeTensixImageAddress(index,sfpuSmoke);
		if (index == 0) return entry;
		if (index <= BlackholeTensixCodeWords(sfpuSmoke)) return code + (address-0x70000)/4;
		return tail + (address-0x17f000)/4;
	}
	uint32 MathImageRead(unsigned index) { volatile uint32* p = MathPointer(index); return p != NULL ? PioneerRead32(p) : UINT32_MAX; }
	uint32* MathSavedWords() { return savedWords; }
	void MathImageWrite(unsigned index, uint32 value) { volatile uint32* p = MathPointer(index); if (p != NULL) PioneerWrite32(p,value); }
	uint32 MathControlRead(unsigned index) {
		const uint32 offsets[] = {0x234,0xa0,0x90};
		return index < 3 ? PioneerRead32(controls+offsets[index]/4) : UINT32_MAX;
	}
	void MathResetWrite(uint32 value) {
		if (value == 0x87ffff || value == 0x46800 || value == 0x47800) PioneerWrite32(controls+0x1b0/4,value);
	}
	uint32 MathPcRead() { return PioneerRead32(pc); }
	void Pause() { spin(100); }
	bool EnterNormalClock(tt_arc_clock_guard_result& r) {
		clockIO.queryAttempted = true;
		return BlackholeArcClockEnter(clockIO,arc,telemetry,r);
	}
	bool LeaveNormalClock(tt_arc_clock_guard_result& r) { return BlackholeArcClockLeave(clockIO,arc,telemetry,r); }
	bool AfterNodeId(uint32 id) {
		if (!PioneerDramSamplesIO::AfterNodeId(id)) return false;
		mathAttempted = true;
		dprintf("PIONEER-P100A: fixed TRISC0 matrix fixtures, one idle tile context reset, no NoC/DMA commands\n");
		if (clock != NULL) return BlackholeClockedMatmulProbe(*this,dram,math,*clock);
		return BlackholeTensixMatmulProbe(*this,dram,math,configSmoke,sfpuSmoke,tileInput,tileOutput);
	}
	bool Unmap()
	{
		bool okay = true;
		for (unsigned i = 0; i < 5; i++) {
			if (mathAreas[i] < B_OK) continue;
			if (delete_area(mathAreas[i]) == B_OK) mathAreas[i] = -1;
			else okay = false;
		}
		if (mathAttempted && !math.tlb_restored) {
			for (unsigned s = 0; s < 2; s++) for (unsigned w = 0; w < 3; w++) DramTlbWrite(s,w,math.tlb_before[s][w]);
			math.tlb_restored = 1;
			for (unsigned s = 0; s < 2; s++) for (unsigned w = 0; w < 3; w++) {
				math.tlb_after[s][w] = DramTlbRead(s,w);
				math.tlb_restored &= math.tlb_after[s][w] == math.tlb_before[s][w];
			}
		}
		if (mathAttempted && (!math.l1_restored || !math.tlb_restored || math.recovery_required)) okay = false;
		if (clock != NULL && !clockIO.Unmap()) okay = false;
		return PioneerDramSamplesIO::Unmap() && okay;
	}
};

static status_t
ProbeControl(void* cookie, uint32 operation, void* data, size_t length)
{
	if (geteuid() != 0 || cookie != &sCookie)
		return B_NOT_ALLOWED;
	if (operation == TT_PCI_PROBE_PIONEER_CLOCKED_MATMUL_SUPPORT) {
		if (data == NULL || length != sizeof(tt_matmul_support)) return B_BAD_VALUE;
		status_t status = AcquireProbeLock(); if (status != B_OK) return status;
		const tt_matmul_support support = {7,TT_TENSIX_CODE_WORDS,TT_TENSIX_IMAGE_WORDS,0x6000,0x70000,TT_TENSIX_PARK_PC,0x17f800,0x87ffff,0x46800};
		release_sem(sLock); return user_memcpy(data,&support,sizeof(support));
	}
	if (operation == TT_PCI_PROBE_PIONEER_CLOCKED_MATMUL) {
		if (data == NULL || length != sizeof(tt_pioneer_clocked_matmul_result)) return B_BAD_VALUE;
		tt_pioneer_clocked_matmul_result result = {};
		status_t status = user_memcpy(data,&result,sizeof(result)); if (status != B_OK) return status;
		status = AcquireProbeLock(); if (status != B_OK) return status;
		status = sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		if (status != B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager == NULL) status = get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
		tt_pioneer_profile profile = {};
		if (status == B_OK) status = GetPioneerPciProfile(sDeviceManager,sCard,profile);
		profile.status = status;
		tt_pioneer_arc_result& control = result.matmul.baseline.control.control.control;
		tt_arc_telemetry_result& health = result.matmul.baseline.control.control.telemetry;
		tt_tensix_reset_result& tensix = result.matmul.baseline.control.tensix;
		tt_tensix_matmul_result& math = result.matmul.math;
		control.pci.version = control.arc.version = health.version = tensix.version = result.matmul.baseline.dram.version = math.version = result.clock.version = 1;
		control.pci.profile_status = status; control.pci.profile_stage = profile.stage;
		control.arc.tlb_restored = health.tlb_restored = tensix.tlb_restored = result.matmul.baseline.dram.tlb_restored = result.matmul.baseline.dram.data_restored = 1;
		math.l1_restored = math.tlb_restored = result.clock.restored = result.clock.last.request_restored = result.clock.last.tlb_restored = 1;
		if (status == B_OK) {
			PioneerMatmulIO io(profile,control.arc,health,tensix,result.matmul.baseline.dram,math,false,&result.clock);
			BlackholePioneerMmioProbe(io,control.pci);
		}
		if (math.recovery_required || result.clock.recovery_required || result.clock.last.recovery_required
			|| result.matmul.baseline.dram.recovery_required || control.pci.state == TT_PIONEER_MMIO_RESTORE_FAILED) {
			math.recovery_required = result.clock.recovery_required = 1; sColdRecoveryRequired = true;
		}
		sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		release_sem(sLock);
		dprintf("PIONEER-P100A: clocked matrix state %" B_PRIu32 " complete/stop %" B_PRIu32 "/%" B_PRIu32
			" clock restored %" B_PRIu32 " PCI restored %" B_PRIu32 " recovery %" B_PRIu32 "\n",
			math.state,math.completed,math.stopped,result.clock.restored,control.pci.restored,math.recovery_required);
		return user_memcpy(data,&result,sizeof(result));
	}
	if (operation == TT_PCI_PROBE_PIONEER_ARC_CLOCK_CYCLE_SUPPORT) {
		if (data == NULL || length != sizeof(tt_arc_clock_support)) return B_BAD_VALUE;
		status_t status = AcquireProbeLock(); if (status != B_OK) return status;
		const tt_arc_clock_support support = {1,0x34,0x52,0x54,800,1400};
		release_sem(sLock); return user_memcpy(data,&support,sizeof(support));
	}
	if (operation == TT_PCI_PROBE_PIONEER_ARC_CLOCK_CYCLE) {
		if (data == NULL || length != sizeof(tt_pioneer_arc_clock_result)) return B_BAD_VALUE;
		tt_pioneer_arc_clock_result result = {};
		status_t status = user_memcpy(data,&result,sizeof(result)); if (status != B_OK) return status;
		status = AcquireProbeLock(); if (status != B_OK) return status;
		status = sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		if (status != B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager == NULL) status = get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
		tt_pioneer_profile profile = {};
		if (status == B_OK) status = GetPioneerPciProfile(sDeviceManager,sCard,profile);
		profile.status = status;
		tt_pioneer_mmio_result& pci = result.control.control.pci;
		tt_arc_scratch_result& arc = result.control.control.arc;
		tt_arc_telemetry_result& health = result.control.telemetry;
		pci.version = arc.version = health.version = result.clock.version = 1;
		pci.profile_status = status; pci.profile_stage = profile.stage;
		arc.tlb_restored = health.tlb_restored = result.clock.restored = 1;
		result.clock.last.tlb_restored = result.clock.last.request_restored = 1;
		if (status == B_OK) {
			PioneerArcQueryIO io(profile,arc,health,result.clock.last,&result.clock);
			BlackholePioneerMmioProbe(io,pci);
		}
		if (result.clock.recovery_required || result.clock.last.recovery_required || pci.state == TT_PIONEER_MMIO_RESTORE_FAILED) {
			result.clock.recovery_required = 1; sColdRecoveryRequired = true;
		}
		sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		release_sem(sLock);
		dprintf("PIONEER-P100A: clock cycle state %" B_PRIu32 " MHz %" B_PRIu32 "/%" B_PRIu32 "/%" B_PRIu32
			" restored %" B_PRIu32 " PCI restored %" B_PRIu32 " recovery %" B_PRIu32 "\n",
			result.clock.state,result.clock.before_mhz,result.clock.busy_mhz,result.clock.after_mhz,
			result.clock.restored,pci.restored,result.clock.recovery_required);
		return user_memcpy(data,&result,sizeof(result));
	}
	if (operation == TT_PCI_PROBE_PIONEER_ARC_QUERY_SUPPORT) {
		if (data == NULL || length != sizeof(tt_arc_query_support)) return B_BAD_VALUE;
		status_t status = AcquireProbeLock(); if (status != B_OK) return status;
		const tt_arc_query_support support = {2,0x34,3,4,TT_ARC_QUEUE_CSM_BYTES};
		release_sem(sLock);
		return user_memcpy(data,&support,sizeof(support));
	}
	if (operation == TT_PCI_PROBE_PIONEER_ARC_CLOCK_QUERY) {
		if (data == NULL || length != sizeof(tt_pioneer_arc_query_result)) return B_BAD_VALUE;
		tt_pioneer_arc_query_result result = {};
		status_t status = user_memcpy(data,&result,sizeof(result)); if (status != B_OK) return status;
		status = AcquireProbeLock(); if (status != B_OK) return status;
		status = sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		if (status != B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager == NULL) status = get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
		tt_pioneer_profile profile = {};
		if (status == B_OK) status = GetPioneerPciProfile(sDeviceManager,sCard,profile);
		profile.status = status;
		tt_pioneer_mmio_result& pci = result.control.control.pci;
		tt_arc_scratch_result& arc = result.control.control.arc;
		tt_arc_telemetry_result& health = result.control.telemetry;
		pci.version = arc.version = health.version = result.query.version = 1;
		pci.profile_status = status; pci.profile_stage = profile.stage;
		arc.tlb_restored = health.tlb_restored = result.query.tlb_restored = result.query.request_restored = 1;
		if (status == B_OK) {
			PioneerArcQueryIO io(profile,arc,health,result.query);
			BlackholePioneerMmioProbe(io,pci);
		}
		if (result.query.recovery_required || pci.state == TT_PIONEER_MMIO_RESTORE_FAILED) {
			result.query.recovery_required = 1; sColdRecoveryRequired = true;
		}
		sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		release_sem(sLock);
		dprintf("PIONEER-P100A: ARC query state %" B_PRIu32 " submit/reply/ack %" B_PRIu32 "/%" B_PRIu32 "/%" B_PRIu32
			" request/TLB/PCI restored %" B_PRIu32 "/%" B_PRIu32 "/%" B_PRIu32 " recovery %" B_PRIu32 "\n",
			result.query.state,result.query.submitted,result.query.responded,result.query.acknowledged,
			result.query.request_restored,result.query.tlb_restored,pci.restored,result.query.recovery_required);
		return user_memcpy(data,&result,sizeof(result));
	}
	if (operation == TT_PCI_PROBE_PIONEER_MATMUL_SUPPORT || operation == TT_PCI_PROBE_PIONEER_TENSIX_CONFIG_SUPPORT || operation == TT_PCI_PROBE_PIONEER_SFPU_LOAD_SUPPORT || operation == TT_PCI_PROBE_PIONEER_SFPU_STORE_SUPPORT || operation == TT_PCI_PROBE_PIONEER_OPERAND_SUPPORT || operation == TT_PCI_PROBE_PIONEER_MATRIX_EXEC_SUPPORT || operation == TT_PCI_PROBE_PIONEER_ZEROACC_SUPPORT || operation == TT_PCI_PROBE_PIONEER_MOVD_SUPPORT || operation == TT_PCI_PROBE_PIONEER_KERNEL_OPERAND_SUPPORT || operation == TT_PCI_PROBE_PIONEER_KERNEL_MATRIX_EXEC_SUPPORT || operation == TT_PCI_PROBE_PIONEER_KERNEL_MATMUL_SUPPORT || operation == TT_PCI_PROBE_PIONEER_DST_READ_SUPPORT
		|| operation == TT_PCI_PROBE_PIONEER_SRCA_ROUNDTRIP_SUPPORT || operation == TT_PCI_PROBE_PIONEER_SRCB_ROUNDTRIP_SUPPORT
		|| operation == TT_PCI_PROBE_PIONEER_PURE_SRCA_SUPPORT || operation == TT_PCI_PROBE_PIONEER_PURE_SRCB_SUPPORT || operation == TT_PCI_PROBE_PIONEER_PURE_MATMUL_SUPPORT
		|| operation == TT_PCI_PROBE_PIONEER_UNPACK_MATMUL_SUPPORT || operation == TT_PCI_PROBE_PIONEER_UNPACK_SRCA_SUPPORT || operation == TT_PCI_PROBE_PIONEER_UNPACK_SRCB_SUPPORT || operation == TT_PCI_PROBE_PIONEER_UNPACK_CONFIG_SUPPORT
		|| operation == TT_PCI_PROBE_PIONEER_IDENTITY_MATMUL_SUPPORT || operation == TT_PCI_PROBE_PIONEER_DENSE_MATMUL_SUPPORT || operation == TT_PCI_PROBE_PIONEER_DENSE_SRCA_SUPPORT || operation == TT_PCI_PROBE_PIONEER_PATTERN_SRCB_SUPPORT || operation == TT_PCI_PROBE_PIONEER_DATA_TILE_SUPPORT) {
		if (data == NULL || length != sizeof(tt_matmul_support)) return B_BAD_VALUE;
		status_t status = AcquireProbeLock(); if (status != B_OK) return status;
		uint32 version = 5;
		if (operation == TT_PCI_PROBE_PIONEER_TENSIX_CONFIG_SUPPORT) version = 6;
		if (operation == TT_PCI_PROBE_PIONEER_SFPU_LOAD_SUPPORT) version = 8;
		if (operation == TT_PCI_PROBE_PIONEER_SFPU_STORE_SUPPORT) version = 9;
		if (operation == TT_PCI_PROBE_PIONEER_OPERAND_SUPPORT) version = 10;
		if (operation == TT_PCI_PROBE_PIONEER_MATRIX_EXEC_SUPPORT) version = 11;
		if (operation == TT_PCI_PROBE_PIONEER_ZEROACC_SUPPORT) version = 12;
		if (operation == TT_PCI_PROBE_PIONEER_MOVD_SUPPORT) version = 13;
		if (operation == TT_PCI_PROBE_PIONEER_KERNEL_OPERAND_SUPPORT) version = 14;
		if (operation == TT_PCI_PROBE_PIONEER_KERNEL_MATRIX_EXEC_SUPPORT) version = 15;
		if (operation == TT_PCI_PROBE_PIONEER_KERNEL_MATMUL_SUPPORT) version = 16;
		if (operation == TT_PCI_PROBE_PIONEER_DST_READ_SUPPORT) version = 17;
		if (operation == TT_PCI_PROBE_PIONEER_SRCA_ROUNDTRIP_SUPPORT) version = 18;
		if (operation == TT_PCI_PROBE_PIONEER_SRCB_ROUNDTRIP_SUPPORT) version = 19;
		if (operation == TT_PCI_PROBE_PIONEER_PURE_SRCA_SUPPORT) version = 20;
		if (operation == TT_PCI_PROBE_PIONEER_PURE_SRCB_SUPPORT) version = 21;
		if (operation == TT_PCI_PROBE_PIONEER_PURE_MATMUL_SUPPORT) version = 22;
		if (operation == TT_PCI_PROBE_PIONEER_UNPACK_MATMUL_SUPPORT) version = 31;
		if (operation == TT_PCI_PROBE_PIONEER_UNPACK_SRCA_SUPPORT) version = 32;
		if (operation == TT_PCI_PROBE_PIONEER_UNPACK_SRCB_SUPPORT) version = 33;
		if (operation == TT_PCI_PROBE_PIONEER_UNPACK_CONFIG_SUPPORT) version = 34;
		if (operation == TT_PCI_PROBE_PIONEER_IDENTITY_MATMUL_SUPPORT) version = 35;
		if (operation == TT_PCI_PROBE_PIONEER_DENSE_MATMUL_SUPPORT) version = 36;
		if (operation == TT_PCI_PROBE_PIONEER_DENSE_SRCA_SUPPORT) version = 37;
		if (operation == TT_PCI_PROBE_PIONEER_PATTERN_SRCB_SUPPORT) version = 38;
		if (operation == TT_PCI_PROBE_PIONEER_DATA_TILE_SUPPORT) version = 39;
		unsigned stage = version >= 31 ? version-15 : 0;
		const tt_matmul_support support = {version,BlackholeTensixCodeWords(stage),BlackholeTensixImageWords(stage),0x6000,0x70000,BlackholeTensixParkPc(stage),0x17f800,0x87ffff,0x46800};
		release_sem(sLock);
		return user_memcpy(data,&support,sizeof(support));
	}
	if (operation == TT_PCI_PROBE_PIONEER_DATA_TILE) {
		if (data == NULL || length != sizeof(tt_pioneer_data_tile)) return B_BAD_VALUE;
		// Heap-owned snapshot: never consult userspace during device execution.
		// Result buffers are zeroed, not copied from the caller.
		struct Transaction {
			tt_pioneer_data_tile* p;
			Transaction() : p((tt_pioneer_data_tile*)calloc(1,sizeof(tt_pioneer_data_tile))) {}
			~Transaction() { free(p); }
		} transaction;
		if (transaction.p == NULL) return B_NO_MEMORY;
		status_t status = user_memcpy(&transaction.p->input,data,sizeof(tt_tensix_tile_data));
		if (status != B_OK) return status;
		if (!BlackholeTileDataValid(transaction.p->input)) return B_BAD_VALUE;
		status = AcquireProbeLock(); if (status != B_OK) return status;
		status = sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		if (status != B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager == NULL) status = get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
		tt_pioneer_profile profile = {};
		if (status == B_OK) status = GetPioneerPciProfile(sDeviceManager,sCard,profile);
		profile.status = status;
		tt_pioneer_matmul_result& result = transaction.p->result;
		tt_pioneer_arc_result& control = result.baseline.control.control.control;
		tt_arc_telemetry_result& telemetry = result.baseline.control.control.telemetry;
		tt_tensix_reset_result& tensix = result.baseline.control.tensix;
		control.pci.version = control.arc.version = telemetry.version = tensix.version = result.baseline.dram.version = 1;
		control.pci.profile_status = status; control.pci.profile_stage = profile.stage;
		control.arc.tlb_restored = telemetry.tlb_restored = tensix.tlb_restored = result.baseline.dram.tlb_restored = result.baseline.dram.data_restored = 1;
		result.math.l1_restored = result.math.tlb_restored = 1;
		if (status == B_OK) {
			PioneerMatmulIO io(profile,control.arc,telemetry,tensix,result.baseline.dram,result.math,
				false,NULL,24,&transaction.p->input,transaction.p->output);
			BlackholePioneerMmioProbe(io,control.pci);
		}
		if (result.math.recovery_required || result.baseline.dram.recovery_required || control.pci.state == TT_PIONEER_MMIO_RESTORE_FAILED) {
			result.math.recovery_required = 1; sColdRecoveryRequired = true;
		}
		sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		release_sem(sLock);
		// Execution is not a numerical pass: the caller validates the result.
		if (result.math.state != TT_MATMUL_DATA_EXECUTED || !result.math.completed
			|| !result.math.stopped || !result.math.l1_restored || !result.math.tlb_restored
			|| !control.pci.restored || result.math.recovery_required)
			memset(transaction.p->output,0,sizeof(transaction.p->output));
		return user_memcpy(data,transaction.p,sizeof(tt_pioneer_data_tile));
	}
	if (operation == TT_PCI_PROBE_PIONEER_TENSIX_MATMUL || operation == TT_PCI_PROBE_PIONEER_TENSIX_CONFIG || operation == TT_PCI_PROBE_PIONEER_SFPU_LOAD || operation == TT_PCI_PROBE_PIONEER_SFPU_STORE || operation == TT_PCI_PROBE_PIONEER_OPERAND_SMOKE || operation == TT_PCI_PROBE_PIONEER_MATRIX_EXEC_SMOKE || operation == TT_PCI_PROBE_PIONEER_ZEROACC_SMOKE || operation == TT_PCI_PROBE_PIONEER_MOVD_SMOKE || operation == TT_PCI_PROBE_PIONEER_KERNEL_OPERAND || operation == TT_PCI_PROBE_PIONEER_KERNEL_MATRIX_EXEC || operation == TT_PCI_PROBE_PIONEER_KERNEL_MATMUL || operation == TT_PCI_PROBE_PIONEER_DST_READ
		|| operation == TT_PCI_PROBE_PIONEER_SRCA_ROUNDTRIP || operation == TT_PCI_PROBE_PIONEER_SRCB_ROUNDTRIP
		|| operation == TT_PCI_PROBE_PIONEER_PURE_SRCA || operation == TT_PCI_PROBE_PIONEER_PURE_SRCB || operation == TT_PCI_PROBE_PIONEER_PURE_MATMUL
		|| operation == TT_PCI_PROBE_PIONEER_UNPACK_MATMUL || operation == TT_PCI_PROBE_PIONEER_UNPACK_SRCA || operation == TT_PCI_PROBE_PIONEER_UNPACK_SRCB || operation == TT_PCI_PROBE_PIONEER_UNPACK_CONFIG
		|| operation == TT_PCI_PROBE_PIONEER_IDENTITY_MATMUL || operation == TT_PCI_PROBE_PIONEER_DENSE_MATMUL || operation == TT_PCI_PROBE_PIONEER_DENSE_SRCA || operation == TT_PCI_PROBE_PIONEER_PATTERN_SRCB) {
		if (data == NULL || length != sizeof(tt_pioneer_matmul_result)) return B_BAD_VALUE;
		tt_pioneer_matmul_result result = {};
		status_t status = user_memcpy(data,&result,sizeof(result)); if (status != B_OK) return status;
		status = AcquireProbeLock(); if (status != B_OK) return status;
		status = sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		if (status != B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager == NULL) status = get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
		tt_pioneer_profile profile = {};
		if (status == B_OK) status = GetPioneerPciProfile(sDeviceManager,sCard,profile);
		profile.status = status;
		tt_pioneer_arc_result& control = result.baseline.control.control.control;
		tt_arc_telemetry_result& telemetry = result.baseline.control.control.telemetry;
		tt_tensix_reset_result& tensix = result.baseline.control.tensix;
		control.pci.version = control.arc.version = telemetry.version = tensix.version = result.baseline.dram.version = result.math.version = 1;
		control.pci.profile_status = status; control.pci.profile_stage = profile.stage;
		control.arc.tlb_restored = telemetry.tlb_restored = tensix.tlb_restored = result.baseline.dram.tlb_restored = result.baseline.dram.data_restored = 1;
		result.math.l1_restored = result.math.tlb_restored = 1;
		if (status == B_OK) {
			unsigned sfpuOnly = operation == TT_PCI_PROBE_PIONEER_SFPU_STORE ? 2U : operation == TT_PCI_PROBE_PIONEER_SFPU_LOAD ? 1U : 0U;
			if (operation == TT_PCI_PROBE_PIONEER_OPERAND_SMOKE) sfpuOnly = 3;
			if (operation == TT_PCI_PROBE_PIONEER_MATRIX_EXEC_SMOKE) sfpuOnly = 4;
			if (operation == TT_PCI_PROBE_PIONEER_ZEROACC_SMOKE) sfpuOnly = 5;
			if (operation == TT_PCI_PROBE_PIONEER_MOVD_SMOKE) sfpuOnly = 6;
			if (operation == TT_PCI_PROBE_PIONEER_KERNEL_MATMUL) sfpuOnly = 7;
			if (operation == TT_PCI_PROBE_PIONEER_KERNEL_OPERAND) sfpuOnly = 8;
			if (operation == TT_PCI_PROBE_PIONEER_KERNEL_MATRIX_EXEC) sfpuOnly = 9;
			if (operation == TT_PCI_PROBE_PIONEER_DST_READ) sfpuOnly = 10;
			if (operation == TT_PCI_PROBE_PIONEER_SRCA_ROUNDTRIP) sfpuOnly = 11;
			if (operation == TT_PCI_PROBE_PIONEER_SRCB_ROUNDTRIP) sfpuOnly = 12;
			if (operation == TT_PCI_PROBE_PIONEER_PURE_SRCA) sfpuOnly = 13;
			if (operation == TT_PCI_PROBE_PIONEER_PURE_SRCB) sfpuOnly = 14;
			if (operation == TT_PCI_PROBE_PIONEER_PURE_MATMUL) sfpuOnly = 15;
			if (operation == TT_PCI_PROBE_PIONEER_UNPACK_MATMUL) sfpuOnly = 16;
			if (operation == TT_PCI_PROBE_PIONEER_UNPACK_SRCA) sfpuOnly = 17;
			if (operation == TT_PCI_PROBE_PIONEER_UNPACK_SRCB) sfpuOnly = 18;
			if (operation == TT_PCI_PROBE_PIONEER_UNPACK_CONFIG) sfpuOnly = 19;
			if (operation == TT_PCI_PROBE_PIONEER_IDENTITY_MATMUL) sfpuOnly = 20;
			if (operation == TT_PCI_PROBE_PIONEER_DENSE_MATMUL) sfpuOnly = 21;
			if (operation == TT_PCI_PROBE_PIONEER_DENSE_SRCA) sfpuOnly = 22;
			if (operation == TT_PCI_PROBE_PIONEER_PATTERN_SRCB) sfpuOnly = 23;
			PioneerMatmulIO io(profile,control.arc,telemetry,tensix,result.baseline.dram,result.math,operation == TT_PCI_PROBE_PIONEER_TENSIX_CONFIG,NULL,sfpuOnly);
			BlackholePioneerMmioProbe(io,control.pci);
		}
		if (result.math.recovery_required || result.baseline.dram.recovery_required || control.pci.state == TT_PIONEER_MMIO_RESTORE_FAILED) {
			result.math.recovery_required = 1; sColdRecoveryRequired = true;
		}
		sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		release_sem(sLock);
		dprintf("PIONEER-P100A: matrix state %" B_PRIu32 " started/completed/stopped %" B_PRIu32 "/%" B_PRIu32 "/%" B_PRIu32
			" mismatch %" B_PRIu32 " L1/TLB/PCI restored %" B_PRIu32 "/%" B_PRIu32 "/%" B_PRIu32 " recovery %" B_PRIu32 "\n",
			result.math.state,result.math.started,result.math.completed,result.math.stopped,result.math.mismatches,
			result.math.l1_restored,result.math.tlb_restored,control.pci.restored,result.math.recovery_required);
		return user_memcpy(data,&result,sizeof(result));
	}
	if (operation == TT_PCI_PROBE_PIONEER_DRAM_SAMPLES_SUPPORT) {
		if (data == NULL || length != sizeof(tt_dram_samples_support)) return B_BAD_VALUE;
		status_t status = AcquireProbeLock();
		if (status != B_OK) return status;
		const tt_dram_samples_support support = {2, TT_DRAM_BANKS, TT_DRAM_REGIONS, TT_DRAM_WORDS, 0xfff, 0xf7};
		release_sem(sLock);
		return user_memcpy(data, &support, sizeof(support));
	}
	if (operation == TT_PCI_PROBE_PIONEER_DRAM_ALL_ROUTES || operation == TT_PCI_PROBE_PIONEER_DRAM_SAMPLES) {
		if (data == NULL || length != sizeof(tt_pioneer_dram_samples_result)) return B_BAD_VALUE;
		tt_pioneer_dram_samples_result result = {};
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
		control.pci.version = control.arc.version = telemetry.version = tensix.version = result.dram.version = 1;
		control.pci.profile_status = status; control.pci.profile_stage = profile.stage;
		control.arc.tlb_restored = telemetry.tlb_restored = tensix.tlb_restored = result.dram.tlb_restored = result.dram.data_restored = 1;
		if (status == B_OK) {
			PioneerDramSamplesIO io(profile, control.arc, telemetry, tensix, result.dram, operation == TT_PCI_PROBE_PIONEER_DRAM_SAMPLES);
			BlackholePioneerMmioProbe(io, control.pci);
		}
		if (result.dram.recovery_required || control.pci.state == TT_PIONEER_MMIO_RESTORE_FAILED) {
			result.dram.recovery_required = 1; sColdRecoveryRequired = true;
		}
		sPCI->unreserve_device(sCard.bus, sCard.device, sCard.function, TT_PCI_PROBE_DEVICE, &sCookie);
		release_sem(sLock);
		dprintf("PIONEER-P100A: DRAM samples state %" B_PRIu32 " routes/cores %" B_PRIu32 "/%" B_PRIu32
			" data/TLB/PCI restored %" B_PRIu32 "/%" B_PRIu32 "/%" B_PRIu32 " recovery %" B_PRIu32 "\n",
			result.dram.state, result.dram.routes_verified, result.dram.cores_checked,
			result.dram.data_restored, result.dram.tlb_restored, control.pci.restored, result.dram.recovery_required);
		return user_memcpy(data, &result, sizeof(result));
	}
	if (operation == TT_PCI_PROBE_PIONEER_DRAM_SUPPORT) {
		if (data == NULL || length != sizeof(tt_dram_route_support)) return B_BAD_VALUE;
		status_t status = AcquireProbeLock();
		if (status != B_OK) return status;
		const tt_dram_route_support support = {2, 0, TT_DRAM_SOURCE_WORDS, TT_DRAM_ID_WORDS, 18, 12};
		release_sem(sLock);
		return user_memcpy(data, &support, sizeof(support));
	}
	if (operation == TT_PCI_PROBE_PIONEER_DRAM_ROUTE) {
		if (data == NULL || length != sizeof(tt_pioneer_dram_route_result)) return B_BAD_VALUE;
		tt_pioneer_dram_route_result result = {};
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
		control.pci.version = control.arc.version = telemetry.version = tensix.version = result.dram.version = 1;
		control.pci.profile_status = status; control.pci.profile_stage = profile.stage;
		control.arc.tlb_restored = telemetry.tlb_restored = tensix.tlb_restored = result.dram.tlb_restored = 1;
		if (status == B_OK) {
			PioneerDramRouteIO io(profile, control.arc, telemetry, tensix, result.dram);
			BlackholePioneerMmioProbe(io, control.pci);
		}
		if (result.dram.recovery_required || control.pci.state == TT_PIONEER_MMIO_RESTORE_FAILED) {
			result.dram.recovery_required = 1;
			sColdRecoveryRequired = true;
		}
		sPCI->unreserve_device(sCard.bus, sCard.device, sCard.function, TT_PCI_PROBE_DEVICE, &sCookie);
		release_sem(sLock);
		dprintf("PIONEER-P100A: DRAM route state %" B_PRIu32 " TLB/PCI restored %" B_PRIu32 "/%" B_PRIu32
			" cold recovery %" B_PRIu32 "\n", result.dram.state,
			result.dram.tlb_restored, control.pci.restored, result.dram.recovery_required);
		return user_memcpy(data, &result, sizeof(result));
	}
	if (operation == TT_PCI_PROBE_PIONEER_BRISC_SUPPORT || operation == TT_PCI_PROBE_PIONEER_TRISC_SUPPORT || operation == TT_PCI_PROBE_PIONEER_TRISC_RESET_SUPPORT) {
		if (data == NULL || length != sizeof(tt_brisc_support)) return B_BAD_VALUE;
		status_t status = AcquireProbeLock();
		if (status != B_OK) return status;
		const tt_brisc_support support = {operation == TT_PCI_PROBE_PIONEER_TRISC_RESET_SUPPORT ? 3U : operation == TT_PCI_PROBE_PIONEER_TRISC_SUPPORT ? 2U : 1U, TT_BRISC_CODE_WORDS, TT_BRISC_IMAGE_WORDS, 0x70000, 0x17ffc0, 0x70048};
		release_sem(sLock);
		// Loaded-module ABI/image metadata only: no PCI or MMIO access.
		return user_memcpy(data, &support, sizeof(support));
	}
	if (operation == TT_PCI_PROBE_PIONEER_BRISC_SMOKE || operation == TT_PCI_PROBE_PIONEER_TRISC_SMOKE || operation == TT_PCI_PROBE_PIONEER_TRISC_RESET_SMOKE) {
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
			PioneerBriscIO io(profile, control.arc, telemetry, tensix, result.brisc, operation != TT_PCI_PROBE_PIONEER_BRISC_SMOKE, operation == TT_PCI_PROBE_PIONEER_TRISC_RESET_SMOKE);
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
	if (operation == TT_PCI_PROBE_PIONEER_TELEMETRY || operation == TT_PCI_PROBE_PIONEER_OPERATING_TELEMETRY) {
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
			PioneerTelemetryIO io(profile, result.control.arc, result.telemetry, operation == TT_PCI_PROBE_PIONEER_OPERATING_TELEMETRY);
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
