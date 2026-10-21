/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 * Optional, root-only Pioneer experiments. Only fixed owned host-buffer DMA;
 * no arbitrary DMA, unowned host writes or firmware flash.
 */
#include "tenstorrent_pci_probe.h"
#ifdef PIONEER_Q38_SIGNED_MAG2_QUALIFICATION
#error Signed magnitude2 image selection is OFFLINE registry proof ONLY;NO native driver qualification yet
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102100010026ULL && !defined(PIONEER_Q38_Q4_NIBBLE_QUALIFICATION)
#error Reserved tag26 must identify ONLY the exact Q4 nibble image qualification
#endif
#ifdef PIONEER_Q38_Q4_NIBBLE_QUALIFICATION
#if !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026102100010026ULL || !defined(PIONEER_Q38_NORMAL_CLOCK) || !defined(PIONEER_Q38_ROOT_CYCLE_PROFILE) || !defined(PIONEER_Q38_SENTINEL_POLL_QUALIFICATION) || !defined(PIONEER_Q38_DIRECT_SRCB_QUALIFICATION) || defined(TT_PIONEER_Q38_DISPATCH_PROFILE)
#error Q4 nibble requires separate exact-tag26 retained-owner qualification, not ordinary Work/model promotion
#endif
#endif

#include <KernelExport.h>
#include <ByteOrder.h>
#include <PCI.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <new>
#include <vm/vm.h>
#ifdef PIONEER_Q38_PEER_PHASE_QUALIFICATION
#if !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026102100010025ULL || !defined(PIONEER_Q38_NORMAL_CLOCK) || !defined(PIONEER_Q38_ROOT_CYCLE_PROFILE) || !defined(PIONEER_Q38_SENTINEL_POLL_QUALIFICATION) || defined(TT_PIONEER_Q38_DISPATCH_PROFILE)
#error Private timing DATA requires separate exact-tag25 retained-owner qualification,not ordinary Work
#endif
#endif
#ifdef PIONEER_Q38_COMMAND_REUSE_QUALIFICATION
#if !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026102100010024ULL
#error Command reuse requires separate exact-tag24 retained-owner qualifier
#endif
#endif
#ifdef PIONEER_Q38_TILE_PREFETCH_QUALIFICATION
#if !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026102100010023ULL
#error Tile prefetch requires its separate exact-tag23 retained-owner qualifier
#endif
#endif
#ifdef PIONEER_Q38_RAW_UNPACK_CACHE_QUALIFICATION
#if !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026102100010022ULL
#error Raw-Dst cache native qualifier requires separate exact-tag22 retained-owner build
#endif
#endif
#ifdef PIONEER_Q38_FRONTEND_BARRIER_QUALIFICATION
#if !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026102100010021ULL
#error Frontend-barrier native qualifier requires separate exact-tag21 retained-owner build
#endif
#endif
#ifdef PIONEER_Q38_BATCH4_FETCH_QUALIFICATION
#ifdef PIONEER_Q38_BATCH_RAW_QUALIFICATION
#if !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026102100010020ULL
#error Direct batch input native qualifier requires its separate exact-tag20 retained-owner build
#endif
#else
#if !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026102100010019ULL
#error Batch4 native qualifier requires its separate exact-tag19 retained-owner build
#endif
#endif
#endif
#ifdef PIONEER_Q38_HOST_PHASE_PROFILE
#if !defined(TT_PIONEER_Q38_DISPATCH_PROFILE) || !defined(PIONEER_Q38_SENTINEL_POLL_QUALIFICATION) || !defined(PIONEER_Q38_ROOT_CYCLE_PROFILE) || !defined(PIONEER_Q38_NORMAL_CLOCK) || !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026102100010018ULL
#error Host phase diagnostic requires its separate exact-tag18 sentinel/root/normal-clock qualification
#endif
#include "PioneerQ38HostPhaseProfile.h"
#endif
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
#include "PioneerQ38StateZeroController.h"
#include "PioneerQ38NonRootHeld.h"
#include "PioneerQ38GdnFreshPlan.h"
#include "PioneerQ38InitializeOwnedState.h"
#if !defined(PIONEER_Q38_NORMAL_CLOCK) || !defined(PIONEER_Q38_LOCAL_DELTA_PROBE) || !defined(PIONEER_Q38_SPAN_PRELOAD)
#error Private mutable adapter requires the complete retained-owner normal-clock bank driver
#endif
#endif
#ifdef PIONEER_Q38_GDN_ZERO_DIAGNOSTIC
#ifndef PIONEER_Q38_GDN_MUTABLE_STATE
#error Fixed zero diagnostic requires the complete private mutable-state owner
#endif
#include "PioneerQ38GdnZeroProtocol.h"
#endif
#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
#if !defined(PIONEER_Q38_GDN_MUTABLE_STATE) || !defined(PIONEER_Q38_GDN_ZERO_DIAGNOSTIC)
#error Resident ROOT0 qualification requires the complete fixed zero diagnostic owner
#endif
#define PIONEER_Q38_RGDN_ROOT_QUALIFICATION
#include "PioneerQ38ResidentGdnRootController.h"
#endif
#ifdef PIONEER_Q38_NORMAL_CLOCK
#if !defined(PIONEER_Q38_WORK_SPARSE_EXPORT) || !defined(TT_PIONEER_Q38_WORK_MODE) || !defined(TT_PIONEER_DRIVER_RELOAD) || !defined(PIONEER_Q38_SPAN_PRELOAD)
#error Normal Q38 clocks require the retained-owner reloadable sparse Work driver
#endif
#include "PioneerQ38ClockLease.h"
#endif
#if defined(PIONEER_Q38_LOCAL_DELTA_PROBE) && (!defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR) || !defined(TT_PIONEER_DRIVER_RELOAD) || !defined(TT_PIONEER_Q38_WORK_MODE))
#error LOCAL vector delta probe requires the retained-owner reloadable vector Work driver
#endif
#if defined(PIONEER_Q38_LOCAL_DELTA_GGML) && (!defined(PIONEER_Q38_LOCAL_DELTA_PROBE) || !defined(PIONEER_Q38_NORMAL_CLOCK))
#error Post-dot LOCAL GDN candidate requires the complete normal-clock local probe driver
#endif
#if defined(TT_PIONEER_EXPERIMENTAL_Q38_PANEL) && !defined(TT_PIONEER_EXPERIMENTAL_Q38_BANKS)
#error Q38 local panel probe requires the exclusive seven-bank owner
#endif
#if defined(TT_PIONEER_EXPERIMENTAL_Q38_AFFINE) && !defined(TT_PIONEER_EXPERIMENTAL_Q38_PANEL)
#error Q38 affine probe requires the guarded local panel protocol
#endif
#if defined(TT_PIONEER_EXPERIMENTAL_Q38_BLOCK) && !defined(TT_PIONEER_EXPERIMENTAL_Q38_AFFINE)
#error Q38 block probe requires the guarded affine protocol
#endif
#if defined(TT_PIONEER_EXPERIMENTAL_Q38_STREAM) && !defined(TT_PIONEER_EXPERIMENTAL_Q38_BLOCK)
#error Q38 fullK stream requires the qualified block protocol and all-bank owner
#endif
#if defined(TT_PIONEER_EXPERIMENTAL_Q38_ROWS) && !defined(TT_PIONEER_EXPERIMENTAL_Q38_STREAM)
#error Q38 batch rows require the guarded fullK/all-bank owner
#endif
#if defined(TT_PIONEER_EXPERIMENTAL_Q38_COHORT) && !defined(TT_PIONEER_EXPERIMENTAL_Q38_ROWS)
#error Q38 cohort requires full saved-view/banked row ownership
#endif
#if defined(TT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED) && !defined(TT_PIONEER_EXPERIMENTAL_Q38_COHORT)
#error Q38 sealed cohort requires the exclusive immutable-image cohort owner
#endif
#if defined(TT_PIONEER_EXPERIMENTAL_Q38_COHORT_PAYLOAD) && !defined(TT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED)
#error Q38 payload blocks require the exclusive boundary-code-sealed cohort owner
#endif
#if defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH) && !defined(TT_PIONEER_EXPERIMENTAL_Q38_COHORT_PAYLOAD)
#error Q38 persistent dispatcher requires the guarded payload/all-bank owner
#endif
#if defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR) && !defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH)
#error Q38 vector peer requires the complete private persistent dispatcher lifecycle
#endif
#if defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_BFLOAT) && !defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR)
#error Q38 BF16 peer requires the complete guarded vector dispatcher lifecycle
#endif
#if defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_FAULT_RETURN) && (!defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR) || defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_BFLOAT))
#error Q38 fault-return57 requires the guarded integer vector dispatcher only
#endif
#if defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_TRANSFER_DIAGNOSTIC) && !defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_FAULT_RETURN)
#error Q38 transfer58 diagnostics require the guarded fault-return lifecycle
#endif
#if defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_PEER_PROGRESS) && !defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_TRANSFER_DIAGNOSTIC)
#error Q38 progress59 diagnostics require the guarded transfer-diagnostic lifecycle
#endif

int32 api_version = B_CUR_DRIVER_API_VERSION;
static pci_module_info* sPCI;
static pci_info sCard;
static char sCookie;
struct PioneerStoreContext;
struct PioneerProbeCookie {
	uint32 magic=0x50313030;
	team_id team=-1;
	PioneerStoreContext* context=NULL;
};
static PioneerProbeCookie* sRuntimeOwner=NULL;
static sem_id sLock = -1;
#ifdef TT_PIONEER_DRIVER_RELOAD
#if !defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH) || !defined(PIONEER_DRIVER_BUILD_TAG)
#error Reload support requires the guarded persistent driver and an explicit build identity
#endif
static_assert(PIONEER_DRIVER_BUILD_TAG!=0,"reload build identity must be nonzero");
// PIONEER_DRIVER_RELOAD_STATE_BEGIN
static pioneer_driver_reload_guard_info* sReloadGuard=NULL;
static uint64 sReloadGeneration=0;
static uint32 sOpenCookies=0;
static bool sReloadQuiesced=false;
struct PioneerColdRecoveryLatch {
	bool value=false;
	operator bool() const { return value || !sReloadGuard || sReloadGuard->cold(); }
	void operator=(bool failure) {
		if (failure) { value=true; if (sReloadGuard) sReloadGuard->fault(sReloadGeneration); }
	}
};
static PioneerColdRecoveryLatch sColdRecoveryRequired;
// PIONEER_DRIVER_RELOAD_STATE_END
#else
// Protected by sLock. A module reload is NOT cold recovery; after this latch
// is set, the board must be cold-recovered before changing/reloading drivers.
static bool sColdRecoveryRequired = false;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_BANKS
static bool sBankStorageOnly = false; // sLock; blocks EVERY legacy runtime operation
#endif
static device_manager_info* sDeviceManager;
static const char* sNames[] = {TT_PCI_PROBE_DEVICE, NULL};

// PIONEER_PROBE_LOCK_BEGIN
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
// Instantiation sees the complete native context later in this TU. Keep
// this a typed presence check,not a separately maintained global hint.
template<class Cookie> static bool Q38MutableScopeOwned(const Cookie* cookie)
{
	return cookie && cookie->context && (cookie->context->q38GdnBanks || cookie->context->q38StateZero
#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
		|| cookie->context->q38ResidentGdnRoot
#endif
		);
}
#endif
#ifdef PIONEER_Q38_LOCAL_DELTA_PROBE
static const void* sQ38LocalDeltaOwner=NULL; // sLock;cleared ONLY after clean END
#endif
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
// Every OTHER successful lock invalidates resident-input hints, including
// bulk/bank operations and support queries. Saturation permanently disables
// reuse for this driver lifetime, never wraps an old hint back into use.
static uint64_t sQ38InputEpoch=1;
#endif
static status_t
AcquireProbeLock(const void* runtimeOwner=NULL,bool bankCommand=false
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
	,bool preserveQ38Input=false
#endif
#ifdef PIONEER_Q38_LOCAL_DELTA_PROBE
	,bool localDeltaCommand=false
#endif
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
	,bool gdnSessionCommand=false
#endif
	)
{
	status_t status = acquire_sem(sLock);
	if (status == B_OK && sColdRecoveryRequired) {
		release_sem(sLock);
		return B_DEV_NOT_READY;
	}
#ifdef TT_PIONEER_DRIVER_RELOAD
	if (status==B_OK && sReloadQuiesced) {
		release_sem(sLock); return B_BUSY; // EVERY hardware operation is gated
	}
#endif
	if (status==B_OK && sRuntimeOwner!=NULL && sRuntimeOwner!=runtimeOwner) {
		release_sem(sLock);
		return B_BUSY;
	}
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_BANKS
	if (status==B_OK && sBankStorageOnly && !bankCommand) {
		release_sem(sLock);
		return B_NOT_ALLOWED;
	}
#else
	(void)bankCommand;
#endif
#ifdef PIONEER_Q38_LOCAL_DELTA_PROBE
	if (status==B_OK && sQ38LocalDeltaOwner!=NULL && (!localDeltaCommand || sQ38LocalDeltaOwner!=runtimeOwner)) {
		release_sem(sLock); return B_NOT_ALLOWED; // deny overlapping ownership UNDER the lock
	}
#endif
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
	// EVERY ordinary hardware command is denied once either private owner
	// exists,even before first zero or after zero CLOSE. Only SAME-owner
	// session STATUS/explicit END use the separate private call site below.
	if (status==B_OK && Q38MutableScopeOwned(sRuntimeOwner) && (!gdnSessionCommand || sRuntimeOwner!=runtimeOwner)) {
		release_sem(sLock); return B_NOT_ALLOWED;
	}
#endif
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
	if (status==B_OK && !preserveQ38Input && sQ38InputEpoch!=UINT64_MAX) sQ38InputEpoch++;
#endif
	return status;
}
#ifdef PIONEER_Q38_LOCAL_DELTA_PROBE
static status_t AcquireQ38DeltaLock(const void* cookie)
{
	return AcquireProbeLock(cookie,true
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
		,false
#endif
		,true);
}
#endif
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
[[maybe_unused]] static status_t AcquireQ38GdnSessionLock(const void* cookie)
{
	return AcquireProbeLock(cookie,true
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
		,false
#endif
#ifdef PIONEER_Q38_LOCAL_DELTA_PROBE
		,true
#endif
		,true);
}
#endif
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

struct PioneerHostPageIO {
	area_id area=-1;
	volatile uint32* page=NULL;
	uint64 physical=0;
	bool releaseAttempted=false,quarantined=false;
	~PioneerHostPageIO() {
		if (area>=B_OK && !releaseAttempted && !quarantined) ReleaseHostPage();
	}
	int32 AllocateHostPage() {
		if (area>=B_OK || B_PAGE_SIZE!=TT_HOST_PAGE_BYTES) return B_BAD_VALUE;
		virtual_address_restrictions virtualRestrictions={};
		virtualRestrictions.address_specification=B_ANY_KERNEL_ADDRESS;
		physical_address_restrictions physicalRestrictions={};
		physicalRestrictions.low_address=UINT64_C(0x100000000);
		physicalRestrictions.high_address=UINT64_C(0x1000000000000); // exclusive
		physicalRestrictions.alignment=TT_HOST_PAGE_BYTES;
		void* address=NULL;
		area=create_area_etc(B_SYSTEM_TEAM,"P100A dedicated high host-read page",
			TT_HOST_PAGE_BYTES,B_CONTIGUOUS,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA,
			0,0,&virtualRestrictions,&physicalRestrictions,&address);
		if (area<B_OK) return area;
		page=(volatile uint32*)address; return B_OK;
	}
	int32 HostPageMap(uint64_t& address,uint64_t& bytes) {
		if (area<B_OK || page==NULL) return B_BAD_VALUE;
		physical_entry entry={};
		status_t status=get_memory_map((const void*)page,TT_HOST_PAGE_BYTES,&entry,1);
		if (status!=B_OK) return status;
		address=entry.address; bytes=entry.size;
		if (PioneerHostPageAddress(address,bytes)) physical=address;
		return B_OK;
	}
	int32 SetHostPageNonCached() {
#if defined(__riscv)
		if (area<B_OK || !PioneerHostPageAddress(physical,TT_HOST_PAGE_BYTES)) return B_BAD_VALUE;
		// Same ordinary non-cacheable RAM type used by this port's working
		// NVMe queue/bounce allocations, not an MMIO alias or cached buffer.
		return vm_set_area_memory_type(area,physical,B_WRITE_THROUGH_MEMORY);
#else
		return B_UNSUPPORTED;
#endif
	}
	uint32 HostPageRead(unsigned w) { return w<TT_HOST_PAGE_WORDS?PioneerRead32(page+w):UINT32_MAX; }
	void HostPageWrite(unsigned w,uint32 value) { if (w<TT_HOST_PAGE_WORDS) PioneerWrite32(page+w,value); }
	int32 ReleaseHostPage() {
		if (area<B_OK || releaseAttempted || quarantined) return B_BAD_VALUE;
		releaseAttempted=true;
		status_t status=delete_area(area);
		if (status==B_OK) { area=-1; page=NULL; }
		return status;
	}
	void QuarantineHostPage() {
		// Future DMA callers must retain a page after uncertain completion:
		// late device reads must never observe newly reused private memory.
		// A system-team area stays wired even if this module later unloads.
		quarantined=true;
	}
};

// Private contiguous host owner. Only the separately guarded bulk-upload
// controller hands this address to the device; the allocation probe never does.
struct PioneerOutputBufferIO {
	area_id outputArea=-1;
	volatile uint32* outputPage=NULL;
	uint64 outputPhysical=0;
	bool outputReleaseAttempted=false,outputQuarantined=false;
	~PioneerOutputBufferIO() {
		if (outputArea>=B_OK && !outputReleaseAttempted && !outputQuarantined) ReleaseOutputBuffer();
	}
	int32 AllocateOutputBuffer() {
		if (outputArea>=B_OK || B_PAGE_SIZE!=4096) return B_BAD_VALUE;
		virtual_address_restrictions virtualRestrictions={};
		virtualRestrictions.address_specification=B_ANY_KERNEL_ADDRESS;
		physical_address_restrictions physicalRestrictions={};
		physicalRestrictions.low_address=UINT64_C(0x100000000);
		physicalRestrictions.high_address=UINT64_C(0x1000000000000);
		physicalRestrictions.alignment=4096;
		void* address=NULL;
		outputArea=create_area_etc(B_SYSTEM_TEAM,"P100A private high host-transfer buffer",
			TT_OUTPUT_BUFFER_BYTES,B_CONTIGUOUS,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA,
			0,0,&virtualRestrictions,&physicalRestrictions,&address);
		if (outputArea<B_OK) return outputArea;
		outputPage=(volatile uint32*)address; return B_OK;
	}
	int32 OutputBufferMap(uint64_t& address,uint64_t& bytes) {
		if (outputArea<B_OK || !outputPage) return B_BAD_VALUE;
		physical_entry entry={};
		status_t status=get_memory_map((const void*)outputPage,TT_OUTPUT_BUFFER_BYTES,&entry,1);
		if (status!=B_OK) return status;
		address=entry.address; bytes=entry.size;
		if (PioneerOutputBufferAddress(address,bytes)) outputPhysical=address;
		return B_OK;
	}
	int32 OutputBufferPageMap(unsigned page,uint64_t& address,uint64_t& bytes) {
		if (page>=4 || outputArea<B_OK || !outputPage) return B_BAD_VALUE;
		physical_entry entry={};
		status_t status=get_memory_map((const void*)(outputPage+page*1024),4096,&entry,1);
		if (status!=B_OK) return status;
		address=entry.address; bytes=entry.size; return B_OK;
	}
	int32 SetOutputBufferNonCached() {
#if defined(__riscv)
		if (outputArea<B_OK || !PioneerOutputBufferAddress(outputPhysical,TT_OUTPUT_BUFFER_BYTES)) return B_BAD_VALUE;
		return vm_set_area_memory_type(outputArea,outputPhysical,B_WRITE_THROUGH_MEMORY);
#else
		return B_UNSUPPORTED;
#endif
	}
	uint32 OutputBufferRead(unsigned w) { return outputArea>=B_OK && w<TT_OUTPUT_BUFFER_WORDS?PioneerRead32(outputPage+w):UINT32_MAX; }
	void OutputBufferWrite(unsigned w,uint32 value) { if (outputArea>=B_OK && w<TT_OUTPUT_BUFFER_WORDS) PioneerWrite32(outputPage+w,value); }
	int32 ReleaseOutputBuffer() {
		if (outputArea<B_OK || outputReleaseAttempted || outputQuarantined) return B_BAD_VALUE;
		outputReleaseAttempted=true;
		status_t status=delete_area(outputArea);
		if (status==B_OK) { outputArea=-1; outputPage=NULL; }
		return status;
	}
	void QuarantineOutputBuffer() { outputQuarantined=true; } // retain wired system-team ownership
};
struct PioneerOutputBufferProbeIO : PioneerHostPageIO,PioneerOutputBufferIO {};

struct PioneerWideOutputIO {
	area_id wideArea=-1;
	volatile uint32* widePage=NULL;
	uint64 widePhysical=0;
	bool wideReleaseAttempted=false,wideQuarantined=false;
	~PioneerWideOutputIO() { if (wideArea>=B_OK && !wideReleaseAttempted && !wideQuarantined) ReleaseWideOutput(); }
	int32 AllocateWideOutput() {
		if (wideArea>=B_OK || B_PAGE_SIZE!=4096) return B_BAD_VALUE;
		virtual_address_restrictions virtualRestrictions={}; virtualRestrictions.address_specification=B_ANY_KERNEL_ADDRESS;
		physical_address_restrictions physicalRestrictions={};
		physicalRestrictions.low_address=UINT64_C(0x100000000); physicalRestrictions.high_address=UINT64_C(0x1000000000000);
		physicalRestrictions.alignment=TT_WIDE_OUTPUT_BYTES;
		void* address=NULL;
		wideArea=create_area_etc(B_SYSTEM_TEAM,"P100A private1MiB wide result owner",TT_WIDE_OUTPUT_BYTES,B_CONTIGUOUS,
			B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA,0,0,&virtualRestrictions,&physicalRestrictions,&address);
		if (wideArea<B_OK) return wideArea;
		widePage=(volatile uint32*)address; return B_OK;
	}
	int32 WideOutputMap(uint64_t& address,uint64_t& bytes) {
		if (wideArea<B_OK || !widePage) return B_BAD_VALUE;
		physical_entry entry={}; status_t status=get_memory_map((const void*)widePage,TT_WIDE_OUTPUT_BYTES,&entry,1);
		if (status!=B_OK) return status;
		address=entry.address; bytes=entry.size;
		if (PioneerWideOutputAddress(address,bytes)) widePhysical=address;
		return B_OK;
	}
	int32 WideOutputPageMap(unsigned page,uint64_t& address,uint64_t& bytes) {
		if (page>=TT_WIDE_OUTPUT_PAGES || wideArea<B_OK || !widePage) return B_BAD_VALUE;
		physical_entry entry={}; status_t status=get_memory_map((const void*)(widePage+page*1024),4096,&entry,1);
		if (status!=B_OK) return status;
		address=entry.address; bytes=entry.size; return B_OK;
	}
	int32 SetWideOutputNonCached() {
#if defined(__riscv)
		if (wideArea<B_OK || !PioneerWideOutputAddress(widePhysical,TT_WIDE_OUTPUT_BYTES)) return B_BAD_VALUE;
		return vm_set_area_memory_type(wideArea,widePhysical,B_WRITE_THROUGH_MEMORY);
#else
		return B_UNSUPPORTED;
#endif
	}
	void WideHostFence() { PioneerIoFence(); }
	uint32 WideHostLoad(unsigned w) { return wideArea>=B_OK && widePage && w<TT_WIDE_OUTPUT_WORDS?B_LENDIAN_TO_HOST_INT32(widePage[w]):UINT32_MAX; }
	void WideHostStore(unsigned w,uint32 value) { if (wideArea>=B_OK && widePage && w<TT_WIDE_OUTPUT_WORDS) widePage[w]=B_HOST_TO_LENDIAN_INT32(value); }
	int32 ReleaseWideOutput() {
		if (wideArea<B_OK || wideReleaseAttempted || wideQuarantined) return B_BAD_VALUE;
		wideReleaseAttempted=true; status_t status=delete_area(wideArea);
		if (status==B_OK) { wideArea=-1; widePage=NULL; }
		return status;
	}
	void QuarantineWideOutput() { wideQuarantined=true; } // retain wired system-team ownership
};

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
#ifdef TT_PIONEER_DRIVER_RELOAD
	// Even PCI enumeration must not be used as a recovery retry after an
	// UNKNOWN event. This check is ordinary resident CPU metadata only.
	pioneer_driver_reload_guard_info* guard=NULL;
	status_t guarded=get_module(PIONEER_DRIVER_RELOAD_GUARD_MODULE,(module_info**)&guard);
	if (guarded!=B_OK) return guarded;
	bool cold=guard->version!=1 || guard->cold();
	put_module(PIONEER_DRIVER_RELOAD_GUARD_MODULE);
	if (cold) return B_DEV_NOT_READY;
#endif
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
#ifdef TT_PIONEER_DRIVER_RELOAD
	status_t guarded=get_module(PIONEER_DRIVER_RELOAD_GUARD_MODULE,(module_info**)&sReloadGuard);
	if (guarded!=B_OK) return guarded;
	if (sReloadGuard->version!=1) guarded=B_BAD_VALUE;
	else guarded=sReloadGuard->attach(&sReloadGeneration);
	if (guarded!=B_OK) {
		put_module(PIONEER_DRIVER_RELOAD_GUARD_MODULE); sReloadGuard=NULL; return guarded;
	}
#endif
	dprintf("PIONEER-P100A: probe driver init begin\n");
	status_t status = get_module(B_PCI_MODULE_NAME, (module_info**)&sPCI);
	dprintf("PIONEER-P100A: probe driver PCI module status %" B_PRId32 "\n", status);
	if (status != B_OK) {
#ifdef TT_PIONEER_DRIVER_RELOAD
		sReloadGuard->detach(sReloadGeneration,true); put_module(PIONEER_DRIVER_RELOAD_GUARD_MODULE); sReloadGuard=NULL;
#endif
		return status;
	}
	if (!FindCard(sPCI, sCard)) {
		put_module(B_PCI_MODULE_NAME);
		sPCI = NULL;
#ifdef TT_PIONEER_DRIVER_RELOAD
		sReloadGuard->detach(sReloadGeneration,true); put_module(PIONEER_DRIVER_RELOAD_GUARD_MODULE); sReloadGuard=NULL;
#endif
		return B_ENTRY_NOT_FOUND;
	}
	dprintf("PIONEER-P100A: probe driver scan complete\n");
	sLock = create_sem(1, "P100A config address probe");
	if (sLock < B_OK) {
		put_module(B_PCI_MODULE_NAME);
		sPCI = NULL;
#ifdef TT_PIONEER_DRIVER_RELOAD
		sReloadGuard->detach(sReloadGeneration,true); put_module(PIONEER_DRIVER_RELOAD_GUARD_MODULE); sReloadGuard=NULL;
#endif
		return sLock;
	}
	dprintf("PIONEER-P100A: probe driver init complete\n");
	return B_OK;
}

void
uninit_driver()
{
#ifdef TT_PIONEER_DRIVER_RELOAD
	// uninit_driver cannot veto an OS unload. Persist any unsafe state in
	// the separate KEEP_LOADED guard BEFORE dropping this driver's image.
	if (sReloadGuard) {
		bool clean=!sColdRecoveryRequired && sRuntimeOwner==NULL && sOpenCookies==0;
		sReloadGuard->detach(sReloadGeneration,clean);
		put_module(PIONEER_DRIVER_RELOAD_GUARD_MODULE); sReloadGuard=NULL;
	}
#endif
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

static status_t ReleaseResidentCookie(PioneerProbeCookie* cookie);

static status_t
ProbeOpen(const char* name, uint32 flags, void** cookie)
{
	if (geteuid() != 0 || (flags & O_RWMASK) != O_RDWR)
		return B_NOT_ALLOWED;
	if (strcmp(name, TT_PCI_PROBE_DEVICE) != 0)
		return B_ENTRY_NOT_FOUND;
	thread_info thread={};
	status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	PioneerProbeCookie* instance=new(std::nothrow) PioneerProbeCookie;
	if (instance==NULL) return B_NO_MEMORY;
	instance->team=thread.team;
#ifdef TT_PIONEER_DRIVER_RELOAD
	status_t locked=acquire_sem(sLock);
	if (locked!=B_OK) { delete instance; return locked; }
	if (sOpenCookies==UINT32_MAX) { release_sem(sLock); delete instance; return B_BUSY; }
	sOpenCookies++;
	release_sem(sLock);
#endif
	*cookie=instance;
	dprintf("PIONEER-P100A: probe open ready\n");
	return B_OK;
}

static status_t ProbeClose(void*) { return B_OK; }
static status_t ProbeFree(void* cookie) {
	status_t status=ReleaseResidentCookie((PioneerProbeCookie*)cookie);
#ifdef TT_PIONEER_DRIVER_RELOAD
	if (status==B_OK) {
		status_t locked=acquire_sem(sLock);
		if (locked!=B_OK) { sColdRecoveryRequired=true; return locked; }
		if (!sOpenCookies) sColdRecoveryRequired=true;
		else sOpenCookies--;
		release_sem(sLock);
	}
#endif
	return status;
}

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

#ifdef PIONEER_Q38_NORMAL_CLOCK
// PIONEER_Q38_CLOCK_IO_BEGIN
// Only private clock transport uses this adapter. Map creates VIRTUAL
// views; Unmap deletes VIRTUAL views ONLY. Never inherit diagnostic ARC
// rollback, which is forbidden after a Q38 ownership/completion fault.
struct PioneerQ38ClockIO : PioneerArcQueryIO {
	PioneerQ38ClockIO(const tt_pioneer_profile& p,tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t,tt_arc_queue_result& unused)
		:PioneerArcQueryIO(p,a,t,unused) {}
	bool AfterNodeId(uint32)=delete;
	bool Unmap() {
		area_id* views[]={&queueWriteArea,&msiArea,&csmArea,&scratchArea,&tlbArea,&area};
		for (auto* view:views) if (*view>=B_OK) {
			if (delete_area(*view)!=B_OK) return false; // no second disposal attempt
			*view=-1;
		}
		queueWrite=msi=NULL; csm=scratch=nodeID=NULL; tlb=NULL;
		return true;
	}
};
// PIONEER_Q38_CLOCK_IO_END
#endif

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

struct PioneerComputeNocIO : PioneerDramSamplesIO {
	tt_compute_noc_result& noc;
	area_id nocArea=-1;
	const volatile uint32* nocRegisters=NULL;
	bool nocAttempted=false;
	const unsigned targetY;
	PioneerComputeNocIO(const tt_pioneer_profile& p,tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t,tt_tensix_reset_result& c,tt_dram_samples_result& d,
		tt_compute_noc_result& n,unsigned y=2) : PioneerDramSamplesIO(p,a,t,c,d,false),noc(n),targetY(y) {}
	~PioneerComputeNocIO() { if (nocArea>=B_OK) delete_area(nocArea); }
	bool Map() {
		if (targetY!=2 && targetY!=3) return false;
		if (!PioneerDramSamplesIO::Map()) return false;
		void* address=NULL;
		// Both NIU command-register pages; target reads only. TLB 201 is
		// saved/restored by policy, forced to the fixed selected (1,2)/(1,3).
		nocArea=map_physical_memory("P100A compute NoC0 read-only state",
			profile.bar0_host_start+0x19320000,2*B_PAGE_SIZE,B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA|B_UNCACHED_MEMORY,&address);
		if (nocArea<B_OK) return false;
		nocRegisters=(const volatile uint32*)address; return true;
	}
	uint32 ComputeNocRead(uint32 offset) {
		return BlackholeComputeNocReadOffset(offset) ? PioneerRead32(nocRegisters+offset/4) : UINT32_MAX;
	}
	bool AfterNodeId(uint32 id) {
		if (!PioneerDramSamplesIO::AfterNodeId(id)) return false;
		nocAttempted=true;
		return BlackholeComputeNocProbe(*this,dram,noc,targetY);
	}
	bool Unmap() {
		bool okay=true;
		if (nocArea>=B_OK) {
			if (delete_area(nocArea)==B_OK) nocArea=-1; else okay=false;
		}
		if (nocAttempted && !noc.tlb_restored) {
			for (unsigned w=0;w<3;w++) DramTlbWrite(1,w,noc.tlb_before[w]);
			noc.tlb_restored=1;
			for (unsigned w=0;w<3;w++) noc.tlb_restored &= DramTlbRead(1,w)==noc.tlb_before[w];
		}
		if (nocAttempted && (!noc.tlb_restored || noc.recovery_required)) okay=false;
		return PioneerDramSamplesIO::Unmap() && okay;
	}
};

struct PioneerFleetNocIO : PioneerComputeNocIO {
	tt_pioneer_fleet_noc_result& fleet;
	PioneerFleetNocIO(const tt_pioneer_profile& p,tt_pioneer_fleet_noc_result& r)
		: PioneerComputeNocIO(p,r.baseline.control.control.control.arc,r.baseline.control.control.telemetry,
			r.baseline.control.tensix,r.baseline.dram,r.worker[0]),fleet(r) {}
	bool AfterNodeId(uint32 id) {
		fleet.version=1;
		if (!PioneerDramSamplesIO::AfterNodeId(id) || !BlackholeDramAllCoresHeld(*this,dram)) return false;
		for (unsigned worker=0;worker<TT_PIONEER_WORKERS;worker++) {
			fleet.checked++;
			bool okay=PioneerWorkerNocProbe(*this,dram,worker,fleet.worker[worker]);
			fleet.recovery_required |= fleet.worker[worker].recovery_required;
			if (!okay) return false;
			fleet.passed++;
		}
		return true;
	}
};

struct PioneerResidentReadIO : PioneerComputeNocIO {
	tt_resident_read_result& read;
	area_id sourceArea=-1,scratchArea=-1,writeArea=-1;
	const volatile uint32* sourceData=NULL;
	volatile uint32* scratchData=NULL;
	volatile uint32* niuWrite=NULL;
	bool readAttempted=false;
	PioneerResidentReadIO(const tt_pioneer_profile& p,tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t,tt_tensix_reset_result& c,tt_dram_samples_result& d,
		tt_compute_noc_result& n,tt_resident_read_result& r)
		: PioneerComputeNocIO(p,a,t,c,d,n),read(r) {}
	~PioneerResidentReadIO() {
		if (sourceArea>=B_OK) delete_area(sourceArea);
		if (scratchArea>=B_OK) delete_area(scratchArea);
		if (writeArea>=B_OK) delete_area(writeArea);
	}
	bool Map() {
		if (!PioneerComputeNocIO::Map()) return false;
		void* address=NULL;
		sourceArea=map_physical_memory("P100A fixed GDDR read-only source",
			profile.bar0_host_start+0x19000000,B_PAGE_SIZE,B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA|B_UNCACHED_MEMORY,&address);
		if (sourceArea<B_OK) return false;
		sourceData=(const volatile uint32*)address;
		scratchArea=map_physical_memory("P100A sole tile final 64-byte L1 destination",
			profile.bar0_host_start+0x1917f000,B_PAGE_SIZE,B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&address);
		if (scratchArea<B_OK) return false;
		scratchData=(volatile uint32*)((uint8*)address+0xfc0);
		writeArea=map_physical_memory("P100A fixed NIU0 initiator 0 control",
			profile.bar0_host_start+0x19320000,B_PAGE_SIZE,B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&address);
		if (writeArea<B_OK) return false;
		niuWrite=(volatile uint32*)address; return true;
	}
	uint32 ResidentNocRead(uint32 offset) {
		return BlackholeResidentReadRegister(offset)?PioneerRead32(nocRegisters+offset/4):UINT32_MAX;
	}
	void ResidentNocWrite(uint32 offset,uint32 value) {
		if (BlackholeResidentWriteRegister(offset)) PioneerWrite32(niuWrite+offset/4,value);
	}
	uint32 ResidentGddrRead(unsigned word) { return word<16?PioneerRead32(sourceData+word):UINT32_MAX; }
	uint32 ResidentL1Read(unsigned word) { return word<16?PioneerRead32(scratchData+word):UINT32_MAX; }
	void ResidentL1Write(unsigned word,uint32 value) { if (word<16) PioneerWrite32(scratchData+word,value); }
	void Pause() { spin(100); }
	bool AfterNodeId(uint32 id) {
		if (!PioneerComputeNocIO::AfterNodeId(id)) return false;
		readAttempted=true;
		dprintf("PIONEER-P100A: single fixed 64-byte GDDR-to-L1 read, cores held, no host DMA\n");
		return BlackholeResidentReadProbe(*this,dram,noc,read);
	}
	bool Unmap() {
		bool okay=true;
		area_id* areas[]={&sourceArea,&scratchArea,&writeArea};
		for (area_id* area:areas) if (*area>=B_OK) {
			if (delete_area(*area)==B_OK) *area=-1; else okay=false;
		}
		// The policy alone owns NIU/L1 cleanup. Uncertain completion permits
		// only TLB control rollback, never a request/reset/payload retry.
		if (readAttempted && (!read.l1_restored || !read.niu_restored || !read.tlb_restored || read.recovery_required)) okay=false;
		return PioneerComputeNocIO::Unmap() && okay;
	}
};

struct PioneerHostPathIO : PioneerComputeNocIO {
	tt_host_path_result& path;
	area_id inboundArea=-1,siiArea=-1;
	const volatile uint32* inbound=NULL;
	const volatile uint32* sii=NULL;
	bool pathAttempted=false;
	PioneerHostPathIO(const tt_pioneer_profile& p,tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t,tt_tensix_reset_result& c,tt_dram_samples_result& d,
		tt_compute_noc_result& n,tt_host_path_result& r)
		: PioneerComputeNocIO(p,a,t,c,d,n),path(r) {}
	~PioneerHostPathIO() {
		if (inboundArea>=B_OK) delete_area(inboundArea);
		if (siiArea>=B_OK) delete_area(siiArea);
	}
	bool Map() {
		if (!PioneerComputeNocIO::Map()) return false;
		void* address=NULL;
		// Same Cadence AT page already configured by ECAM, strictly read-only.
		inboundArea=map_physical_memory("P100A root inbound read-only audit",
			profile.controller+0x00400000,B_PAGE_SIZE,B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA|B_UNCACHED_MEMORY,&address);
		if (inboundArea<B_OK) return false;
		inbound=(const volatile uint32*)((uint8*)address+0x810);
		siiArea=map_physical_memory("P100A fixed PCIe SII read-only audit",
			profile.bar0_host_start+0x19000000,B_PAGE_SIZE,B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA|B_UNCACHED_MEMORY,&address);
		if (siiArea<B_OK) return false;
		sii=(const volatile uint32*)((uint8*)address+0x134); return true;
	}
	uint32 HostInboundRead(unsigned w) { return w<2?PioneerRead32(inbound+w):UINT32_MAX; }
	uint32 HostSiiRead(unsigned w) { return w<8?PioneerRead32(sii+w):UINT32_MAX; }
	bool AfterNodeId(uint32 id) {
		if (!PioneerComputeNocIO::AfterNodeId(id)) return false;
		pathAttempted=true;
		dprintf("PIONEER-P100A: read-only host-transfer route/inbound/SII audit; no DMA or bus mastering\n");
		return BlackholeHostPathProbe(*this,dram,noc,id,path);
	}
	bool Unmap() {
		bool okay=true;
		area_id* areas[]={&inboundArea,&siiArea};
		for (area_id* area:areas) if (*area>=B_OK) {
			if (delete_area(*area)==B_OK) *area=-1; else okay=false;
		}
		if (pathAttempted && (!path.tlb_restored || path.recovery_required)) okay=false;
		return PioneerComputeNocIO::Unmap() && okay;
	}
};

struct PioneerHostDmaAuditIO : PioneerHostPathIO {
	tt_host_dma_audit_result& audit;
	area_id hdmaArea=-1;
	const volatile uint32* hdma=NULL;
	bool auditAttempted=false;
	PioneerHostDmaAuditIO(const tt_pioneer_profile& p,tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t,tt_tensix_reset_result& c,tt_dram_samples_result& d,
		tt_compute_noc_result& n,tt_host_path_result& h,tt_host_dma_audit_result& r)
		: PioneerHostPathIO(p,a,t,c,d,n,h),audit(r) {}
	~PioneerHostDmaAuditIO() { if (hdmaArea>=B_OK) delete_area(hdmaArea); }
	bool Map() {
		if (!PioneerHostPathIO::Map()) return false;
		void* address=NULL;
		hdmaArea=map_physical_memory("P100A firmware HDMA ch0 read-only audit",
			profile.bar0_host_start+0x19180000,B_PAGE_SIZE,B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA|B_UNCACHED_MEMORY,&address);
		if (hdmaArea<B_OK) return false;
		hdma=(const volatile uint32*)address; return true;
	}
	uint32 HostPciRead(unsigned peer,uint16 offset,uint8 size) {
		return peer==0?CardRead(offset,size):peer==1?RootRead(offset,size):UINT32_MAX;
	}
	uint32 HostHdmaRead(unsigned w) { return w<4?PioneerRead32(hdma+BlackholeHostHdmaOffset(w)/4):UINT32_MAX; }
	bool AfterNodeId(uint32 id) {
		if (!PioneerHostPathIO::AfterNodeId(id)) return false;
		auditAttempted=true;
		dprintf("PIONEER-P100A: read-only PCIe pending/firmware HDMA ch0 audit; no DMA or bus mastering\n");
		return BlackholeHostDmaAudit(*this,dram,noc,path,audit);
	}
	bool Unmap() {
		bool okay=true;
		if (hdmaArea>=B_OK) { if (delete_area(hdmaArea)==B_OK) hdmaArea=-1; else okay=false; }
		if (auditAttempted && (!audit.tlb_restored || audit.recovery_required)) okay=false;
		return PioneerHostPathIO::Unmap() && okay;
	}
};

struct PioneerHostReadIO : PioneerHostDmaAuditIO, PioneerHostPageIO {
	tt_host_read_result& read;
	const unsigned payloadBytes;
	uint32* backup=NULL;
	area_id scratchArea=-1,writeArea=-1;
	volatile uint32* scratchData=NULL;
	volatile uint32* niuWrite=NULL;
	bool readAttempted=false;
	PioneerHostReadIO(const tt_pioneer_profile& p,tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t,tt_tensix_reset_result& c,tt_dram_samples_result& d,
		tt_compute_noc_result& n,tt_host_path_result& h,tt_host_dma_audit_result& auditResult,tt_host_read_result& r,unsigned bytes)
		: PioneerHostDmaAuditIO(p,a,t,c,d,n,h,auditResult),read(r),payloadBytes(bytes) {}
	~PioneerHostReadIO() {
		free(backup);
		if (scratchArea>=B_OK) delete_area(scratchArea);
		if (writeArea>=B_OK) delete_area(writeArea);
	}
	bool Map() {
		// 128/8192 are only the separate fixed write/readback policies:
		// two distinct L1 regions, never a user-selected size.
		if (payloadBytes!=64 && payloadBytes!=128 && payloadBytes!=4096 && payloadBytes!=8192) return false;
		if (!PioneerHostDmaAuditIO::Map()) return false;
		void* address=NULL;
		unsigned mappedBytes=payloadBytes==8192?8192:4096;
		scratchArea=map_physical_memory("P100A owned host transfer fixed L1 scratch",
			profile.bar0_host_start+0x19180000-mappedBytes,mappedBytes,B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&address);
		if (scratchArea<B_OK) return false;
		scratchData=(volatile uint32*)((uint8*)address+(mappedBytes-payloadBytes));
		writeArea=map_physical_memory("P100A owned host read fixed NIU0 initiator",
			profile.bar0_host_start+0x19320000,B_PAGE_SIZE,B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&address);
		if (writeArea<B_OK) return false;
		niuWrite=(volatile uint32*)address; return true;
	}
	void HostPciWrite(unsigned peer,uint16 offset,uint8 size,uint32 value) {
		if (offset!=4 || size!=2 || (value!=0x402 && value!=0x406)) return;
		if (peer==0) CardWrite(offset,size,value); else if (peer==1) RootWrite(offset,size,value);
	}
	uint32 ResidentNocRead(uint32 offset) {
		return BlackholeResidentReadRegister(offset)?PioneerRead32(nocRegisters+offset/4):UINT32_MAX;
	}
	void ResidentNocWrite(uint32 offset,uint32 value) {
		if (BlackholeResidentWriteRegister(offset)) PioneerWrite32(niuWrite+offset/4,value);
	}
	uint32* HostReadBackup(unsigned words) {
		if (backup!=NULL || words!=payloadBytes/4 || (words!=16 && words!=32 && words!=1024 && words!=2048)) return NULL;
		backup=(uint32*)malloc(words*sizeof(uint32)); return backup;
	}
	uint32 ResidentL1Read(unsigned w) { return w<payloadBytes/4?PioneerRead32(scratchData+w):UINT32_MAX; }
	void ResidentL1Write(unsigned w,uint32 value) { if (w<payloadBytes/4) PioneerWrite32(scratchData+w,value); }
	void Pause() { spin(100); }
	bool AfterNodeId(uint32 id) {
		if (!PioneerHostDmaAuditIO::AfterNodeId(id)) return false;
		readAttempted=true;
		dprintf("PIONEER-P100A: single %u-byte owned high host-page read; all cores held, no host writes\n",payloadBytes);
		if (payloadBytes==4096) return BlackholeHostReadProbe<PioneerHostReadIO,4096>(*this,dram,noc,path,audit,read);
		return payloadBytes==64 && BlackholeHostReadProbe(*this,dram,noc,path,audit,read);
	}
	bool Unmap() {
		bool okay=true;
		area_id* areas[]={&scratchArea,&writeArea};
		for (area_id* mapped:areas) if (*mapped>=B_OK) {
			if (delete_area(*mapped)==B_OK) *mapped=-1; else okay=false;
		}
		if (readAttempted && (!read.bme_restored || !read.l1_restored || !read.niu_restored
			|| !read.tlb_restored || read.recovery_required)) okay=false;
		return PioneerHostDmaAuditIO::Unmap() && okay;
	}
};

struct PioneerHostWriteIO : PioneerHostReadIO {
	tt_host_write_result& write;
	const unsigned writeBytes;
	PioneerHostWriteIO(const tt_pioneer_profile& p,tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t,tt_tensix_reset_result& c,tt_dram_samples_result& d,
		tt_compute_noc_result& n,tt_host_path_result& h,tt_host_dma_audit_result& auditResult,tt_host_write_result& r,unsigned bytes)
		: PioneerHostReadIO(p,a,t,c,d,n,h,auditResult,r.transfer,2*bytes),write(r),writeBytes(bytes) {}
	uint32 ResidentNocRead(uint32 offset) {
		return BlackholeHostWriteReadRegister(offset)?PioneerRead32(nocRegisters+offset/4):UINT32_MAX;
	}
	bool AfterNodeId(uint32 id) {
		if (!PioneerHostDmaAuditIO::AfterNodeId(id)) return false;
		readAttempted=true;
		dprintf("PIONEER-P100A: single owned host %u-byte write + distinct L1 readback; all cores held\n",writeBytes);
		if (writeBytes==4096) return BlackholeHostWriteProbe<PioneerHostWriteIO,4096>(*this,dram,noc,path,audit,write);
		return writeBytes==64 && BlackholeHostWriteProbe(*this,dram,noc,path,audit,write);
	}
};

struct PioneerSessionIO : PioneerHostWriteIO,PioneerOutputBufferIO,PioneerWideOutputIO {
#ifdef PIONEER_Q38_HOST_PHASE_PROFILE
	PioneerQ38HostPhaseProfile q38HostPhaseProfile;
	bool q38HostPhaseReportAllowed=false; // set ONLY by validated explicit BANK_END
#endif
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
	PioneerStoreContext* q38MutableContext=NULL;
	bool StateZeroBeforeCohort();
	bool StateZeroOtherCoresHeld();
	bool NonRootHeldOwner();
	bool NonRootHeldMappingStable();
#endif
#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
	bool ResidentGdnBeforeRoot();
	bool ResidentGdnOtherCoresHeld();
#endif
#ifdef PIONEER_Q38_SPAN_PRELOAD
	bool q38SpanAllowed=true;
	bool SpanBeforeCohort() { return q38SpanAllowed && !fleetEnabled; }
	bool SpanViewCapacity(unsigned words) { return words<=TT_Q38_PANEL_VIEW_WORDS+1; }
	bool SpanOtherCoresHeld() { return BlackholeDramAllCoresHeld(*this,dram); }
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_PANEL
	area_id q38PanelArea=-1;
	volatile uint32* q38PanelView=NULL;
	uint32* q38PanelSaved=NULL;
	bool q38PanelMapAttempted=false;
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT
	uint32* q38CohortSaved[TT_Q38_COHORT_WORKERS]={};
#endif
#endif
	PioneerRuntimeProfile runtimeProfile;
	area_id runtimeAreas[5]={-1,-1,-1,-1,-1};
	volatile uint32* runtimeEntry=NULL;
	volatile uint32* runtimeCode=NULL;
	volatile uint32* runtimeBuffers=NULL;
	volatile uint32* runtimeControl=NULL;
	const volatile uint32* runtimePc=NULL;
	uint32* runtimeSaved=NULL;
	uint32* secondRuntimeSaved=NULL;
	uint32* secondLeaseSaved=NULL;
	uint32* runtimeStage=NULL;
	area_id attentionArea=-1;
	volatile uint32* attentionView=NULL;
	uint32* attentionSaved=NULL;
	uint32* peerAttentionSaved[8]={};
	bool peerAttentionEnabled=false;
	bool peerRopeEnabled=false; // selected once before ANY peer code owner
	bool attentionMapAttempted=false;
	area_id bulkArea=-1;
	volatile uint32* bulkView=NULL;
	uint32* bulkSaved=NULL;
	bool bulkMapAttempted=false;
	unsigned runtimeCore=0;
	bool fleetEnabled=false;
	bool persistentHostStages=false;
	bool payloadBlocks=false;
	area_id coordinatorArea=-1;
	volatile uint32* coordinatorView=NULL;
	uint32* coordinatorSaved=NULL;
	bool coordinatorMapAttempted=false;
	area_id layerArea=-1;
	volatile uint32* layerView=NULL;
	uint32* layerSaved=NULL;
	bool layerMapAttempted=false;
	uint32* decoderSaved=NULL;
	bool decoderMapAttempted=false;
	area_id fleetArea=-1;
	volatile uint32* fleetView=NULL;
	uint32* fleetRuntimeSaved[TT_PIONEER_WORKERS]={};
	uint32* fleetLeaseSaved[TT_PIONEER_WORKERS]={};
	uint32* fleetCodeSaved[TT_PIONEER_WORKERS]={};
	bool runtimeMapAttempted=false,runtimeMapped=false;
	unsigned runtimeBufferWords=8192;
	PioneerSessionIO(const tt_pioneer_profile& p,tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t,tt_tensix_reset_result& c,tt_dram_samples_result& d,
		tt_compute_noc_result& n,tt_host_path_result& h,tt_host_dma_audit_result& auditResult,tt_host_write_result& r)
		: PioneerHostWriteIO(p,a,t,c,d,n,h,auditResult,r,4096) {}
	~PioneerSessionIO() {
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_PANEL
		if (q38PanelArea>=B_OK) delete_area(q38PanelArea);
		free(q38PanelSaved);
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT
		for (unsigned w=1;w<TT_Q38_COHORT_WORKERS;w++) free(q38CohortSaved[w]);
#endif
#endif
		for (unsigned i=0;i<5;i++) if (runtimeAreas[i]>=B_OK) delete_area(runtimeAreas[i]);
		if (attentionArea>=B_OK) delete_area(attentionArea);
		if (bulkArea>=B_OK) delete_area(bulkArea);
		if (coordinatorArea>=B_OK) delete_area(coordinatorArea);
		free(coordinatorSaved);
		if (layerArea>=B_OK) delete_area(layerArea);
		free(layerSaved);
		free(decoderSaved);
		free(runtimeSaved); free(secondRuntimeSaved); free(secondLeaseSaved); free(runtimeStage);
		free(attentionSaved);
		for (unsigned i=0;i<8;i++) free(peerAttentionSaved[i]);
		free(bulkSaved);
		if (fleetArea>=B_OK) delete_area(fleetArea);
		for (unsigned w=0;w<TT_PIONEER_WORKERS;w++) { free(fleetRuntimeSaved[w]); free(fleetLeaseSaved[w]); free(fleetCodeSaved[w]); }
	}
	void QuarantineHostPage() {
		PioneerHostPageIO::QuarantineHostPage();
		if (outputArea>=B_OK) QuarantineOutputBuffer();
		if (wideArea>=B_OK) QuarantineWideOutput();
	}
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_PANEL
	bool MapQ38Panel() {
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT
		if (runtimeCore>=120 || (runtimeCore && !fleetEnabled) || !MapRuntime()) return false;
		uint32*& saved=runtimeCore?q38CohortSaved[runtimeCore]:q38PanelSaved;
		if (!saved) saved=(uint32*)malloc((TT_Q38_PANEL_VIEW_WORDS+1)*4);
		if (!saved) return false;
#else
		if (runtimeCore || !MapRuntime()) return false;
#endif
		if (q38PanelView) return Q38PanelMappingStable();
		if (q38PanelMapAttempted) return false;
		q38PanelMapAttempted=true;
#ifndef TT_PIONEER_EXPERIMENTAL_Q38_COHORT
		q38PanelSaved=(uint32*)malloc((TT_Q38_PANEL_VIEW_WORDS+1)*sizeof(uint32));
		if (!q38PanelSaved) return false;
#endif
		void* view=NULL;
		q38PanelArea=map_physical_memory("P100A exact local Q38 panel probe saved L1",profile.bar0_host_start+0x1906e000,
			TT_Q38_PANEL_VIEW_BYTES,B_ANY_KERNEL_ADDRESS,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&view);
		if (q38PanelArea<B_OK) return false;
		q38PanelView=(volatile uint32*)view; return true;
	}
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT
	bool Q38PanelMappingStable() { return runtimeCore<120 && (!runtimeCore || fleetEnabled) && q38PanelArea>=B_OK && q38PanelView && Q38PanelSavedWords(); }
	uint32* Q38PanelSavedWords() { return runtimeCore<120?(runtimeCore?q38CohortSaved[runtimeCore]:q38PanelSaved):NULL; }
	uint64 Q38CohortNow() { return uint64(system_time()); }
#ifdef TT_PIONEER_Q38_DISPATCH_PROFILE
	// PIONEER_Q38_HOST_PHASE_ADD_BEGIN
	void Q38DispatchProfile(unsigned type,unsigned inputs,unsigned peers,bool valid,const uint64_t* t) {
#ifdef PIONEER_Q38_HOST_PHASE_PROFILE
		q38HostPhaseProfile.Add(type,inputs,peers,valid,t); // CPU memory ONLY,NO per-projection log/card IO
#else
		// The caller has already completed ALL proofs/hold/nonce commit.
		// One diagnostic line,no card reads or additional operations.
		dprintf("P100A_Q38_PROFILE type%u K%u peers%u valid%u us authority%llu stage%llu arm%llu execute%llu proof%llu output%llu hold%llu commit%llu;CPU_WALL_NOT_PURE_GPU\n",
			type,inputs,peers,unsigned(valid),(unsigned long long)t[0],(unsigned long long)t[1],
			(unsigned long long)t[2],(unsigned long long)t[3],(unsigned long long)t[4],
			(unsigned long long)t[5],(unsigned long long)t[6],(unsigned long long)t[7]);
#endif
	}
	// PIONEER_Q38_HOST_PHASE_ADD_END
#ifdef PIONEER_Q38_HOST_PHASE_PROFILE
	// PIONEER_Q38_HOST_PHASE_REPORT_BEGIN
	void Q38HostPhaseReport() {
		for(unsigned type=0;type<3;type++)for(unsigned k=0;k<3;k++) {
			const auto& b=q38HostPhaseProfile.At(type,k);if(!b.count && b.valid)continue;
			const auto* t=b.warm;
			dprintf("P100A_Q38_HOST_PHASE type%u K%u peers119 valid%u total%llu warm%llu us authority%llu stage%llu arm%llu execute%llu proof%llu output%llu hold%llu commit%llu;CPU_WALL_SUMS_FIRST_OF_EACH_SHAPE_EXCLUDED_NOT_PURE_GPU_OR_TOKEN_RATE_NO_PER_PROJECTION_LOG\n",
				type==0?8:type==1?12:14,k==0?5120:k==1?6144:17408,unsigned(b.valid),
				(unsigned long long)b.count,(unsigned long long)(b.count?b.count-1:0),
				(unsigned long long)t[0],(unsigned long long)t[1],(unsigned long long)t[2],(unsigned long long)t[3],
				(unsigned long long)t[4],(unsigned long long)t[5],(unsigned long long)t[6],(unsigned long long)t[7]);
		}
		dprintf("P100A_Q38_HOST_PHASE_END ignored%llu;CPU_METADATA_ONLY_AFTER_EXPLICIT_CLEAN_END\n",(unsigned long long)q38HostPhaseProfile.Ignored());
	}
	// PIONEER_Q38_HOST_PHASE_REPORT_END
#endif
#endif
#else
	bool Q38PanelMappingStable() { return !runtimeCore && q38PanelArea>=B_OK && q38PanelView && q38PanelSaved; }
	uint32* Q38PanelSavedWords() { return q38PanelSaved; }
#endif
	uint32 Q38PanelRead(uint32 a) {
		return q38PanelView && !(a&3) && a>=TT_Q38_PANEL_VIEW_BASE && a-TT_Q38_PANEL_VIEW_BASE<TT_Q38_PANEL_VIEW_BYTES
			?PioneerRead32(q38PanelView+(a-TT_Q38_PANEL_VIEW_BASE)/4):UINT32_MAX;
	}
	void Q38PanelWrite(uint32 a,uint32 value) {
		if (q38PanelView && !(a&3) && a>=TT_Q38_PANEL_VIEW_BASE && a-TT_Q38_PANEL_VIEW_BASE<TT_Q38_PANEL_VIEW_BYTES)
			PioneerWrite32(q38PanelView+(a-TT_Q38_PANEL_VIEW_BASE)/4,value);
	}
#endif
	bool MapCoordinator() {
		if (runtimeCore!=0) return false;
		if (coordinatorView) return true;
		if (coordinatorMapAttempted) return false;
		coordinatorMapAttempted=true;
		coordinatorSaved=(uint32*)malloc(TT_COORDINATOR_SAVED_WORDS*4);
		if (!coordinatorSaved) return false;
		void* view=NULL;
		coordinatorArea=map_physical_memory("P100A fixed backed-up coordinator code/stack",profile.bar0_host_start+0x1906c000,
			TT_COORDINATOR_SAVED_WORDS*4,B_ANY_KERNEL_ADDRESS,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&view);
		if (coordinatorArea<B_OK) return false;
		coordinatorView=(volatile uint32*)view; return true;
	}
	bool CoordinatorMappingStable() { return runtimeCore==0 && coordinatorArea>=B_OK && coordinatorView && coordinatorSaved; }
	uint32* CoordinatorSavedWords() { return coordinatorSaved; }
	uint32 CoordinatorRead(unsigned w) { return w<TT_COORDINATOR_SAVED_WORDS && coordinatorView?PioneerRead32(coordinatorView+w):UINT32_MAX; }
	void CoordinatorWrite(unsigned w,uint32 value) { if (w<TT_COORDINATOR_SAVED_WORDS && coordinatorView) PioneerWrite32(coordinatorView+w,value); }
	bool MapLayer() {
		if (runtimeCore!=0) return false;
		if (layerView) return true;
		if (layerMapAttempted) return false;
		layerMapAttempted=true; layerSaved=(uint32*)malloc(TT_LAYER_CODE_WORDS*4);
		if (!layerSaved) return false;
		void* view=NULL;
		layerArea=map_physical_memory("P100A fixed backed-up32KiB FFN code",profile.bar0_host_start+0x19064000,
			TT_LAYER_CODE_WORDS*4,B_ANY_KERNEL_ADDRESS,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&view);
		if (layerArea<B_OK) return false;
		layerView=(volatile uint32*)view; return true;
	}
	bool LayerMappingStable() { return runtimeCore==0 && layerArea>=B_OK && layerView && layerSaved; }
	uint32* LayerSavedWords() { return layerSaved; }
	uint32 LayerCodeRead(unsigned w) { return w<TT_LAYER_CODE_WORDS && layerView?PioneerRead32(layerView+w):UINT32_MAX; }
	void LayerCodeWrite(unsigned w,uint32 value) { if (w<TT_LAYER_CODE_WORDS && layerView) PioneerWrite32(layerView+w,value); }
	bool MapDecoder() {
		if (runtimeCore!=0) return false;
		if (decoderSaved) return LayerMappingStable();
		if (decoderMapAttempted) return false;
		decoderMapAttempted=true;
		if (!MapLayer()) return false;
		decoderSaved=(uint32*)malloc(TT_DECODER_CODE_WORDS*4);
		return decoderSaved!=NULL;
	}
	bool DecoderMappingStable() { return decoderSaved && LayerMappingStable(); }
	bool DecoderAttentionMappingStable() { return runtimeCore==0 && attentionArea>=B_OK && attentionView && attentionSaved; }
	bool DecoderPeerMappingsStable() {
		if (!peerAttentionEnabled || !DecoderAttentionMappingStable()) return false;
		for (unsigned i=0;i<8;i++) if (!peerAttentionSaved[i]) return false;
		return true;
	}
	bool DecoderRopeMappingsStable() { return peerRopeEnabled && DecoderPeerMappingsStable(); }
	uint32* DecoderSavedWords() { return decoderSaved; }
	bool MapBulk() {
		if (runtimeCore!=0) return false;
		if (bulkView) return true;
		if (bulkMapAttempted) return false;
		bulkMapAttempted=true;
		bulkSaved=(uint32*)malloc(TT_BULK_UPLOAD_SAVED_WORDS*4);
		if (!bulkSaved) return false;
		void* view=NULL;
		bulkArea=map_physical_memory("P100A fixed owned bulk-upload L1",profile.bar0_host_start+0x19140000,
			TT_BULK_UPLOAD_SAVED_WORDS*4,B_ANY_KERNEL_ADDRESS,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&view);
		if (bulkArea<B_OK) return false;
		bulkView=(volatile uint32*)view; return true;
	}
	bool BulkMappingStable() { return runtimeCore==0 && bulkArea>=B_OK && bulkView && bulkSaved; }
	uint32* BulkSavedWords() { return bulkSaved; }
	uint32 BulkL1Read(unsigned w) { return w<TT_BULK_UPLOAD_SAVED_WORDS && bulkView?PioneerRead32(bulkView+w):UINT32_MAX; }
	void BulkL1Write(unsigned w,uint32 value) { if (w<TT_BULK_UPLOAD_SAVED_WORDS && bulkView) PioneerWrite32(bulkView+w,value); }
	void BulkHostFence() { PioneerIoFence(); }
	uint32 BulkHostLoad(unsigned w) {
		return w<TT_BULK_UPLOAD_WORDS && outputArea>=B_OK && outputPage?B_LENDIAN_TO_HOST_INT32(outputPage[w]):UINT32_MAX;
	}
	void BulkHostStore(unsigned w,uint32 value) {
		if (w<TT_BULK_UPLOAD_WORDS && outputArea>=B_OK && outputPage) outputPage[w]=B_HOST_TO_LENDIAN_INT32(value);
	}
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT_PAYLOAD
	uint64 BulkHostPairLoad(unsigned w) {
		typedef uint64 Alias64 __attribute__((__may_alias__));
		return !(w&1) && w<TT_BULK_UPLOAD_WORDS-1 && outputArea>=B_OK && outputPage
			?B_LENDIAN_TO_HOST_INT64(*reinterpret_cast<const volatile Alias64*>(outputPage+w)):UINT64_MAX;
	}
	void BulkHostPairStore(unsigned w,uint32 low,uint32 high) {
		typedef uint64 Alias64 __attribute__((__may_alias__));
		if (!(w&1) && w<TT_BULK_UPLOAD_WORDS-1 && outputArea>=B_OK && outputPage)
			*reinterpret_cast<volatile Alias64*>(outputPage+w)=B_HOST_TO_LENDIAN_INT64(uint64(low)|(uint64(high)<<32));
	}
#endif
	struct PayloadIO {
		volatile uint32* view;
		void PayloadFence() { PioneerIoFence(); }
		uint32 PayloadRead(unsigned w) { return B_LENDIAN_TO_HOST_INT32(view[w]); }
		void PayloadWrite(unsigned w,uint32 value) { view[w]=B_HOST_TO_LENDIAN_INT32(value); }
		uint64 PayloadReadPair(unsigned w) {
			// All mapped bases are page aligned; generic block helpers align
			// the word index and never read a pair beyond the checked span.
			typedef uint64 Alias64 __attribute__((__may_alias__));
			return B_LENDIAN_TO_HOST_INT64(*reinterpret_cast<const volatile Alias64*>(view+w));
		}
		void PayloadWritePair(unsigned w,uint32 low,uint32 high) {
			typedef uint64 Alias64 __attribute__((__may_alias__));
			*reinterpret_cast<volatile Alias64*>(view+w)=B_HOST_TO_LENDIAN_INT64(uint64(low)|(uint64(high)<<32));
		}
	};
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT_PAYLOAD
	PayloadIO Q38PayloadData(uint32 a) { return PayloadIO{q38PanelView+(a-TT_Q38_PANEL_VIEW_BASE)/4}; }
#endif
	bool RuntimePayloadRead(unsigned first,unsigned words,uint32* output) {
		if (!output || !runtimeMapped || !runtimeBuffers || !words || first!=4096 || words>4096
			|| first>=runtimeBufferWords || words>runtimeBufferWords-first) return false;
		if (payloadBlocks) { PayloadIO data={runtimeBuffers}; return PioneerPayloadReadBlock(data,first,words,runtimeBufferWords,output); }
		for (unsigned w=0;w<words;w++) output[w]=RuntimeBufferRead(first+w);
		return true;
	}
	bool CoordinatorPayloadRead(unsigned words,uint32* output) {
		if (!output || runtimeCore!=0 || !runtimeMapped || !runtimeBuffers || !payloadBlocks
			|| !words || words>8192 || runtimeBufferWords!=TT_Q8_BULK_SAVED_BUFFER_WORDS) return false;
		PayloadIO data={runtimeBuffers}; return PioneerPayloadReadBlock(data,4096,words,runtimeBufferWords,output);
	}
	bool BulkPayloadStage(const uint32* input,unsigned words) {
		if (!input || !BulkMappingStable() || !words || words>3072) return false;
		if (payloadBlocks) { PayloadIO data={bulkView}; return PioneerPayloadStageBlock(data,words,3072,input); }
		for (unsigned w=0;w<words;w++) BulkL1Write(w,input[w]);
		bool exact=true;
		for (unsigned w=0;w<words;w++) exact &= BulkL1Read(w)==input[w];
		return exact;
	}
	bool BulkPayloadRead(unsigned first,unsigned words,uint32* output) {
		if (!output || !BulkMappingStable() || first!=4096 || words!=2048) return false;
		if (payloadBlocks) { PayloadIO data={bulkView}; return PioneerPayloadReadBlock(data,first,words,8192,output); }
		for (unsigned w=0;w<words;w++) output[w]=BulkL1Read(first+w);
		return true;
	}
	bool MapRuntime(bool bulk=false) {
		unsigned words=bulk?TT_Q8_BULK_SAVED_BUFFER_WORDS:8192;
		if (runtimeMapped) return runtimeBufferWords==words;
		if (runtimeMapAttempted) return false;
		runtimeMapAttempted=true;
		runtimeBufferWords=words;
		runtimeSaved=(uint32*)malloc((1025+words)*sizeof(uint32)); runtimeStage=(uint32*)malloc(4096);
		if (!runtimeSaved || !runtimeStage) return false;
		const uint32 offsets[]={0x19006000,0x19070000,0x19080000,0x19312000,0x19313000};
		void* views[5]={};
		for (unsigned i=0;i<5;i++) {
			uint32 protection=B_KERNEL_READ_AREA|B_UNCACHED_MEMORY;
			if (i!=4) protection|=B_KERNEL_WRITE_AREA;
			runtimeAreas[i]=map_physical_memory("P100A owned runtime fixed compute view",profile.bar0_host_start+offsets[i],
				i==2?words*sizeof(uint32):B_PAGE_SIZE,B_ANY_KERNEL_ADDRESS,protection,&views[i]);
			if (runtimeAreas[i]<B_OK) return false;
		}
		runtimeEntry=(volatile uint32*)views[0]; runtimeCode=(volatile uint32*)views[1];
		runtimeBuffers=(volatile uint32*)views[2]; runtimeControl=(volatile uint32*)views[3];
		runtimePc=(const volatile uint32*)((const uint8*)views[4]+0x140);
		runtimeMapped=true; return true;
	}
	// Only these two fixed coordinates exist. Switching the owned PCIe TLB
	// views is not a core reset or NIU transaction, even during rollback.
	bool SelectRuntimeCore(unsigned core) {
		if (core>=TT_PIONEER_WORKERS || (!fleetEnabled && core>1)) return false;
		unsigned x=fleetEnabled?PioneerWorkerX(core):1,y=fleetEnabled?PioneerWorkerY(core):2+core;
		bool okay=BlackholeDramConfigure(*this,0,0,x,y,false)
			&& BlackholeDramConfigure(*this,1,0xffa00000,x,y,false);
		if (okay) runtimeCore=core;
		return okay;
	}
	bool AllocateFleetWorker(unsigned worker) {
		if (!fleetEnabled || worker>=120 || !runtimeMapped || runtimeBufferWords!=TT_Q8_BULK_SAVED_BUFFER_WORDS) return false;
		if (fleetArea<B_OK) {
			void* view=NULL;
			fleetArea=map_physical_memory("P100A owned fleet prelude page",profile.bar0_host_start+0x19071000,
				B_PAGE_SIZE,B_ANY_KERNEL_ADDRESS,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&view);
			if (fleetArea<B_OK) return false;
			fleetView=(volatile uint32*)view;
		}
		if (!fleetCodeSaved[worker]) fleetCodeSaved[worker]=(uint32*)malloc(4096);
		if (worker && !fleetRuntimeSaved[worker]) fleetRuntimeSaved[worker]=(uint32*)malloc((1025+runtimeBufferWords)*4);
		return fleetCodeSaved[worker] && (!worker || fleetRuntimeSaved[worker]);
	}
	uint32* FleetSavedWords() { return runtimeCore<120?fleetCodeSaved[runtimeCore]:NULL; }
	// Fleet polls still require the same status/PC/counter proofs. Shorter
	// bounded backoff avoids multiplying the old100us delay by120 workers.
	void Pause() { spin(fleetEnabled?1:100); }
	uint32 FleetCodeRead(unsigned word) { return word<1024 && fleetView?PioneerRead32(fleetView+word):UINT32_MAX; }
	void FleetCodeWrite(unsigned word,uint32 value) { if (word<1024 && fleetView) PioneerWrite32(fleetView+word,value); }
	bool AllocateSecondRuntime() {
		if (!runtimeMapped || runtimeBufferWords!=TT_Q8_BULK_SAVED_BUFFER_WORDS) return false;
		if (!secondRuntimeSaved) secondRuntimeSaved=(uint32*)malloc((1025+runtimeBufferWords)*sizeof(uint32));
		return secondRuntimeSaved!=NULL;
	}
	bool MapAttention(bool parallelRope=false) {
		if (parallelRope && (!peerRopeEnabled || !runtimeCore || runtimeCore>8)) return false;
		if ((runtimeCore && (!peerAttentionEnabled || runtimeCore>8)) || (!runtimeMapped && !MapRuntime())) return false;
		uint32*& saved=runtimeCore?peerAttentionSaved[runtimeCore-1]:attentionSaved;
		if (!saved) saved=(uint32*)malloc((parallelRope?TT_ATTN_TILE_ROPE_SAVED_WORDS:TT_ATTN_TILE_SAVED_WORDS)*sizeof(uint32));
		if (!saved) return false;
		if (attentionView) return attentionArea>=B_OK;
		if (attentionMapAttempted) return false;
		attentionMapAttempted=true;
		void* view=NULL;
		attentionArea=map_physical_memory("P100A fixed attention qualification L1",profile.bar0_host_start+0x19072000,
			0x42000,B_ANY_KERNEL_ADDRESS,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&view);
		if (attentionArea<B_OK) return false;
		attentionView=(volatile uint32*)view; return true;
	}
	uint32* AttentionSavedWords() { return !runtimeCore?attentionSaved:peerAttentionEnabled && runtimeCore<=8?peerAttentionSaved[runtimeCore-1]:NULL; }
	bool AttentionRopeMappingStable() { return peerRopeEnabled && runtimeCore && runtimeCore<=8 && attentionArea>=B_OK && attentionView && peerAttentionSaved[runtimeCore-1]; }
	uint32 AttentionReadWord(unsigned w) {
		uint32 address=PioneerAttentionTileWordAddress(w,peerRopeEnabled && runtimeCore && runtimeCore<=8);
		if (address==0x6000) return RuntimeCodeRead(0);
		return address!=UINT32_MAX && attentionView?PioneerRead32(attentionView+(address-0x72000)/4):UINT32_MAX;
	}
	void AttentionWriteWord(unsigned w,uint32 value) {
		uint32 address=PioneerAttentionTileWordAddress(w,peerRopeEnabled && runtimeCore && runtimeCore<=8);
		if (address==0x6000) RuntimeCodeWrite(0,value);
		else if (address!=UINT32_MAX && attentionView) PioneerWrite32(attentionView+(address-0x72000)/4,value);
	}
	uint32* HostReadBackup(unsigned words) {
		if (!runtimeCore) return PioneerHostWriteIO::HostReadBackup(words);
		if (fleetEnabled) {
			if (words!=2048 || runtimeCore>=120 || fleetLeaseSaved[runtimeCore]) return NULL;
			fleetLeaseSaved[runtimeCore]=(uint32*)malloc(8192); return fleetLeaseSaved[runtimeCore];
		}
		if (words!=2048 || secondLeaseSaved) return NULL;
		secondLeaseSaved=(uint32*)malloc(words*sizeof(uint32)); return secondLeaseSaved;
	}
	uint32* RuntimeSavedWords() { return !runtimeCore?runtimeSaved:fleetEnabled?fleetRuntimeSaved[runtimeCore]:secondRuntimeSaved; }
	unsigned RuntimeBufferWords() const { return runtimeBufferWords; }
	uint32* RuntimeStageWords() { return runtimeStage; }
	void RuntimeHostStageFence() { PioneerIoFence(); }
	void RuntimeHostStageStore(unsigned w,uint32 value) {
		if (w<1024 && PioneerHostPageIO::area>=B_OK && PioneerHostPageIO::page)
			PioneerHostPageIO::page[w]=B_HOST_TO_LENDIAN_INT32(value);
	}
	uint32 RuntimeHostStageLoad(unsigned w) {
		return w<1024 && PioneerHostPageIO::area>=B_OK && PioneerHostPageIO::page
			?B_LENDIAN_TO_HOST_INT32(PioneerHostPageIO::page[w]):UINT32_MAX;
	}
	bool RuntimeHostFillVerify(const uint32* data) {
		// Keep the old checked/Assistant paths' transport unchanged. Only
		// the separate persistent session selects boundary-fenced host RAM.
		return persistentHostStages?PioneerRuntimeHostStageBlock(*this,data):PioneerRuntimeHostStage(*this,data,0L);
	}
	void RuntimeProfileMark(unsigned slot) { runtimeProfile.Mark(uint64(system_time()),slot); }
	uint32 RuntimeCodeRead(unsigned w) { return w<=1024?PioneerRead32(w?runtimeCode+w-1:runtimeEntry):UINT32_MAX; }
	void RuntimeCodeWrite(unsigned w,uint32 value) { if (w<=1024) PioneerWrite32(w?runtimeCode+w-1:runtimeEntry,value); }
	uint32 RuntimeBufferRead(unsigned w) { return w<runtimeBufferWords?PioneerRead32(runtimeBuffers+w):UINT32_MAX; }
	void RuntimeBufferWrite(unsigned w,uint32 value) { if (w<runtimeBufferWords) PioneerWrite32(runtimeBuffers+w,value); }
	uint32 RuntimeControlRead(unsigned w) {
		const uint32 offsets[]={0x234,0xa0,0x90}; return w<3?PioneerRead32(runtimeControl+offsets[w]/4):UINT32_MAX;
	}
	void RuntimeResetWrite(uint32 value) {
		if (value==0x87ffff || value==0x46800 || value==0x47800) PioneerWrite32(runtimeControl+0x1b0/4,value);
	}
	uint32 RuntimePcRead() { return PioneerRead32(runtimePc); }
	uint32 RuntimeStatusRead(unsigned w) { return w<3?ResidentL1Read((0x1fc0/4)+w):UINT32_MAX; }
	void RuntimeStatusWrite(unsigned w,uint32 value) { if (w<3) ResidentL1Write((0x1fc0/4)+w,value); }
	bool Unmap() {
		bool okay=true;
		if (attentionArea>=B_OK) {
			if (delete_area(attentionArea)==B_OK) { attentionArea=-1; attentionView=NULL; } else okay=false;
		}
		for (unsigned i=0;i<5;i++) if (runtimeAreas[i]>=B_OK) {
			if (delete_area(runtimeAreas[i])==B_OK) runtimeAreas[i]=-1; else okay=false;
		}
		return PioneerHostWriteIO::Unmap() && okay;
	}
};

struct PioneerResidentMathIO : PioneerComputeNocIO {
	tt_resident_math_result& compute;
	const unsigned gemvInputs;
	area_id mathAreas[7]={-1,-1,-1,-1,-1,-1,-1};
	volatile uint32* entry=NULL;
	volatile uint32* code=NULL;
	volatile uint32* tail=NULL;
	volatile uint32* controls=NULL;
	volatile uint32* gddrData=NULL;
	volatile uint32* niuWrite=NULL;
	const volatile uint32* pc=NULL;
	uint32* savedWords=NULL;
	bool computeAttempted=false;
	PioneerResidentMathIO(const tt_pioneer_profile& p,tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t,tt_tensix_reset_result& c,tt_dram_samples_result& d,
		tt_compute_noc_result& n,tt_resident_math_result& r,unsigned inputs=0)
		: PioneerComputeNocIO(p,a,t,c,d,n),compute(r),gemvInputs(inputs) {}
	~PioneerResidentMathIO() {
		free(savedWords);
		for (unsigned i=0;i<7;i++) if (mathAreas[i]>=B_OK) delete_area(mathAreas[i]);
	}
	bool Map() {
		if (gemvInputs!=0 && !BlackholeResidentGemvShape(gemvInputs)) return false;
		if (!PioneerComputeNocIO::Map()) return false;
		savedWords=(uint32*)malloc((TT_RESIDENT_MATH_IMAGE_WORDS+GddrWordCount())*sizeof(uint32));
		if (savedWords==NULL) return false;
		const uint32 offsets[]={0x19006000,0x19070000,0x1917f000,0x19312000,0x19313000,0x19000000,0x19320000};
		void* pages[7]={};
		for (unsigned i=0;i<7;i++) {
			uint32 protection=B_KERNEL_READ_AREA|B_UNCACHED_MEMORY;
			if (i!=4) protection|=B_KERNEL_WRITE_AREA;
			size_t bytes=i==5?((GddrWordCount()*sizeof(uint32)+B_PAGE_SIZE-1)&~(B_PAGE_SIZE-1)):B_PAGE_SIZE;
			mathAreas[i]=map_physical_memory("P100A fixed resident-matrix experiment page",
				profile.bar0_host_start+offsets[i],bytes,B_ANY_KERNEL_ADDRESS,protection,&pages[i]);
			if (mathAreas[i]<B_OK) return false;
		}
		entry=(volatile uint32*)pages[0]; code=(volatile uint32*)pages[1]; tail=(volatile uint32*)pages[2];
		controls=(volatile uint32*)pages[3]; pc=(const volatile uint32*)((uint8*)pages[4]+0x140);
		gddrData=(volatile uint32*)pages[5]; niuWrite=(volatile uint32*)pages[6]; return true;
	}
	uint32* ResidentMathSavedWords() { return savedWords; }
	unsigned GddrWordCount() const { return (gemvInputs!=0?gemvInputs/16:1)*TT_RESIDENT_MATH_GDDR_WORDS; }
	volatile uint32* MathPointer(unsigned index) {
		if (index>=TT_RESIDENT_MATH_IMAGE_WORDS) return NULL;
		if (index==0) return entry;
		uint32 address=BlackholeResidentMathAddress(index);
		return index<=TT_RESIDENT_MATH_CODE_WORDS?code+(address-0x70000)/4:tail+(address-0x17f000)/4;
	}
	uint32 MathImageRead(unsigned index) { volatile uint32* p=MathPointer(index); return p!=NULL?PioneerRead32(p):UINT32_MAX; }
	void MathImageWrite(unsigned index,uint32 value) { volatile uint32* p=MathPointer(index); if (p!=NULL) PioneerWrite32(p,value); }
	uint32 MathControlRead(unsigned index) {
		const uint32 offsets[]={0x234,0xa0,0x90};
		return index<3?PioneerRead32(controls+offsets[index]/4):UINT32_MAX;
	}
	void MathResetWrite(uint32 value) {
		if (value==0x87ffff || value==0x46800 || value==0x47800) PioneerWrite32(controls+0x1b0/4,value);
	}
	uint32 MathPcRead() { return PioneerRead32(pc); }
	uint32 ResidentGddrRead(unsigned word) { return word<GddrWordCount()?PioneerRead32(gddrData+word):UINT32_MAX; }
	void ResidentGddrWrite(unsigned word,uint32 value) { if (word<GddrWordCount()) PioneerWrite32(gddrData+word,value); }
	uint32 ResidentNocRead(uint32 offset) { return BlackholeResidentReadRegister(offset)?PioneerRead32(nocRegisters+offset/4):UINT32_MAX; }
	void ResidentNocWrite(uint32 offset,uint32 value) {
		// This operation's HOST cannot ring any NoC doorbell. Fixed TRISC0
		// alone issues its bounded reads after the full launch proof.
		if (offset!=0x40 && BlackholeResidentWriteRegister(offset)) PioneerWrite32(niuWrite+offset/4,value);
	}
	void Pause() { spin(100); }
	bool AfterNodeId(uint32 id) {
		if (!PioneerComputeNocIO::AfterNodeId(id)) return false;
		computeAttempted=true;
		if (gemvInputs!=0) {
			dprintf("PIONEER-P100A: fixed 16-output resident GEMV K=%u, device partial sums, no host DMA/flash\n",gemvInputs);
			return BlackholeResidentMathProbe(*this,dram,noc,compute,BlackholeResidentGemvProgram(gemvInputs));
		}
		dprintf("PIONEER-P100A: fixed TRISC0 resident GDDR inputs, two device-issued reads, no host DMA/flash\n");
		return BlackholeResidentMathProbe(*this,dram,noc,compute);
	}
	bool Unmap() {
		bool okay=true;
		for (unsigned i=0;i<7;i++) if (mathAreas[i]>=B_OK) {
			if (delete_area(mathAreas[i])==B_OK) mathAreas[i]=-1; else okay=false;
		}
		if (computeAttempted && (compute.recovery_required || !compute.gddr_restored || !compute.niu_restored
			|| !compute.math.l1_restored || !compute.math.tlb_restored)) okay=false;
		return PioneerComputeNocIO::Unmap() && okay;
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
	const unsigned tileCount;
	uint32* const completedJobs;
	PioneerMatmulIO(const tt_pioneer_profile& p, tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t, tt_tensix_reset_result& c, tt_dram_samples_result& d, tt_tensix_matmul_result& m,
		bool configOnly = false, tt_arc_clock_guard_result* clockGuard = NULL, unsigned sfpuOnly = 0,
		const tt_tensix_tile_data* input = NULL, uint32* output = NULL,
		unsigned count = 1, uint32* completed = NULL)
		: PioneerDramSamplesIO(p,a,t,c,d,false), math(m), configSmoke(configOnly), sfpuSmoke(sfpuOnly),
		clockIO(p,a,t,clockGuard != NULL ? clockGuard->last : unusedQuery,clockGuard), clock(clockGuard),
		tileInput(input), tileOutput(output), tileCount(count), completedJobs(completed) {}
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
		return BlackholeTensixMatmulProbe(*this,dram,math,configSmoke,sfpuSmoke,tileInput,tileOutput,tileCount,completedJobs);
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

// The session owns the PCI reservation, mappings and high host page until
// explicit END or the final close of this FD. No firmware/core start is
// performed here. Storage/compute commands will use this same owner.
struct PioneerStoreContext {
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
	// Private heap owners:separate fixed-zero diagnostic ONLY,not model compute.
	PioneerQ38GdnBanks* q38GdnBanks=NULL;
	PioneerQ38StateZeroLease* q38StateZero=NULL;
	PioneerQ38GdnIdentity q38GdnIdentity={};
	// Only the private diagnostic guard may bind this seal under the global lock,then clear it
	// before releasing the lock. Identity/presence alone never grants IO.
	thread_id q38GdnCallThread=-1;
#endif
#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
	PioneerQ38ResidentGdnRootLease* q38ResidentGdnRoot=NULL;
#endif
#ifdef PIONEER_Q38_LOCAL_DELTA_PROBE
	PioneerQ38DeltaProbeLease q38Delta;
#endif
#ifdef PIONEER_Q38_SPAN_PRELOAD
	PioneerQ38SpanLease* q38Span=NULL;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
	PioneerQ38DispatchLease* q38Dispatch=NULL;
	bool q38DispatchRoutes=false,dispatchOwnerRetained=false;
	unsigned q38EndPhase=0;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT
	PioneerQ38CohortLease* q38Cohort=NULL;
	bool q38CohortRoutes=false;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_STREAM
	PioneerQ38StreamLease streamProbe;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_PANEL
	PioneerQ38PanelLease panelProbe;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_BANKS
	PioneerResidentBanks banks;
	bool bankStorageOnly=false;
#endif
	tt_pioneer_profile profile={};
	tt_pioneer_host_dma_audit_result baseline={};
	tt_host_write_result unused={};
	tt_host_page_result host={};
	tt_resident_session_idle_result idle={};
	tt_pioneer_mmio_transaction transaction={};
	PioneerResidentLease lease;
	PioneerRuntimeMatvecLease compute;
	PioneerRuntimeMatvecLease secondCompute;
	PioneerAttentionTileLease attention;
	PioneerAttentionTileLease peerAttention[8];
	PioneerResidentAttentionLease residentAttention;
	PioneerCoordinatorLease coordinator;
	PioneerLayerLease layer;
	PioneerDecoderLease decoder;
	PioneerDecoderJob decoderModel={};
	bool decoderReady=false;
	PioneerBulkUploadLease bulk;
	PioneerOutputBufferLease bulkHost;
	PioneerWideOutputLease wideHost;
	PioneerRuntimeActivationCache activation[2];
	PioneerResidentLease secondLease;
	tt_compute_noc_result secondNoc={};
	PioneerFleetWorker* fleet=NULL;
	bool fleetRoutes=false;
	int runtimeCoreMode=-1; // mode8 may ONE-WAY seal a fixed immutable model into decoder mode9
	uint64 bytes;
	uint8* initialized=NULL;
	uint32* hashes=NULL;
	uint32 uploadedPages=0;
	PioneerSessionIO io;
#ifdef PIONEER_Q38_NORMAL_CLOCK
	tt_arc_queue_result q38ClockMappingResult={}; // never used for protocol completion
	PioneerQ38ClockIO q38ClockIO;
	PioneerQ38ClockLease<PioneerQ38ClockIO> q38Clock;
#endif
	explicit PioneerStoreContext(uint64 capacity) : bytes(capacity),
		io(profile,baseline.baseline.baseline.baseline.control.control.control.arc,
			baseline.baseline.baseline.baseline.control.control.telemetry,
			baseline.baseline.baseline.baseline.control.tensix,
			baseline.baseline.baseline.baseline.dram,baseline.baseline.baseline.noc,
			baseline.baseline.path,baseline.audit,unused)
#ifdef PIONEER_Q38_NORMAL_CLOCK
		,q38ClockIO(profile,baseline.baseline.baseline.baseline.control.control.control.arc,
			baseline.baseline.baseline.baseline.control.control.telemetry,q38ClockMappingResult),q38Clock(q38ClockIO)
#endif
		{
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
		io.q38MutableContext=this;
#endif
		initialized=(uint8*)calloc(capacity/4096,1);
		hashes=(uint32*)calloc(capacity/4096,sizeof(uint32));
	}
	~PioneerStoreContext() {
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
		delete q38ResidentGdnRoot;
#endif
		delete q38StateZero;
		delete q38GdnBanks;
#endif
#ifdef PIONEER_Q38_SPAN_PRELOAD
		delete q38Span;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
		delete q38Dispatch;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT
		delete q38Cohort;
#endif
		delete[] fleet; free(initialized); free(hashes);
	}
	tt_pioneer_mmio_result& Pci() { return baseline.baseline.baseline.baseline.control.control.control.pci; }
	bool Unsafe(
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
		bool controlledZero=false
#endif
		) {
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
		if ((q38GdnBanks && q38GdnBanks->Unsafe())
			|| (q38StateZero && (q38StateZero->unsafe || (!controlledZero && q38StateZero->running)))) return true;
#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
		if (q38ResidentGdnRoot && (q38ResidentGdnRoot->unsafe || (!controlledZero && q38ResidentGdnRoot->running))) return true;
#endif
#endif
#ifdef PIONEER_Q38_NORMAL_CLOCK
		if (q38Clock.Unsafe()) return true;
#endif
#ifdef PIONEER_Q38_LOCAL_DELTA_PROBE
		if (q38Delta.unsafe || q38Delta.running) return true;
#endif
#ifdef PIONEER_Q38_SPAN_PRELOAD
		if (q38Span && (q38Span->unsafe || q38Span->running)) return true;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
		if (q38Dispatch && PioneerQ38DispatchUnsafe(*q38Dispatch)) return true;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT
		if (q38Cohort) {
			if (q38Cohort->unsafe) return true;
			for (unsigned w=0;w<120;w++) {
				auto& n=q38Cohort->worker[w];
				if (n.noc.recovery_required || n.image.running || n.image.unsafe || n.Store().pending || n.Store().recovery_required) return true;
			}
		}
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_STREAM
		if (streamProbe.running || streamProbe.unsafe) return true;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_PANEL
		if (panelProbe.running || panelProbe.unsafe) return true;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_BANKS
		if (banks.Quarantined()) return true;
#endif
		for (unsigned i=0;i<8;i++) if (peerAttention[i].running || peerAttention[i].unsafe) return true;
		if (fleet) for (unsigned w=0;w<TT_PIONEER_WORKERS;w++) if (fleet[w].Compute().unsafe || fleet[w].Compute().running
			|| fleet[w].Store().pending || fleet[w].Store().recovery_required || fleet[w].noc.recovery_required) return true;
		return decoder.running || decoder.unsafe || wideHost.pending || wideHost.quarantined || io.wideQuarantined || layer.running || layer.unsafe || coordinator.running || coordinator.unsafe || bulk.unsafe || bulkHost.pending || bulkHost.quarantined || attention.running || attention.unsafe || compute.running || compute.unsafe || secondCompute.running || secondCompute.unsafe
			|| secondLease.pending || secondLease.recovery_required || secondNoc.recovery_required
			|| lease.pending || lease.recovery_required || idle.recovery_required || baseline.audit.recovery_required || baseline.baseline.path.recovery_required
			|| baseline.baseline.baseline.noc.recovery_required || baseline.baseline.baseline.baseline.dram.recovery_required
			|| Pci().state==TT_PIONEER_MMIO_RESTORE_FAILED;
	}
	void Snapshot(tt_resident_session_result& r) {
		r={}; r.version=1; r.state=TT_RESIDENT_SESSION_READY; r.active=1; r.bytes=bytes;
		r.host_ready=host.state==TT_HOST_PAGE_READY; r.host_address_bits=host.address_bits;
		r.host_released=host.released; r.pci_restored=Pci().restored;
		r.route_state=baseline.baseline.path.state; r.audit_state=baseline.audit.state;
		r.capabilities=TT_PIONEER_RESIDENT_CAPABILITIES; r.uploaded_pages=uploadedPages;
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_BANKS
		if (bankStorageOnly) r.capabilities=0; // no legacy compute/storage authority
#endif
		r.cold=sColdRecoveryRequired;
	}
};

#ifdef PIONEER_Q38_NORMAL_CLOCK
// PIONEER_Q38_CLOCK_SERVICE_BEGIN
static bool
BeginQ38NormalClock(PioneerStoreContext& c)
{
	if (!c.bankStorageOnly || c.bytes!=PioneerResidentBanks::Total || c.Unsafe()
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
		|| c.q38GdnBanks || c.q38StateZero
#endif
		|| c.q38Clock.RequiresExplicitEnd() || c.q38Dispatch || c.q38Cohort || c.q38Span
		|| c.panelProbe.saved || c.streamProbe.saved || c.fleet || c.runtimeCoreMode!=-1) return false;
	auto& dram=c.baseline.baseline.baseline.baseline.dram;
	const auto& audit=c.baseline.audit;
	// Fresh BEGIN's existing physical ALL120-held/quiet-DMA audit, under
	// the SAME global lock/reservation/bank owner. No caller-supplied proof.
	if (!c.banks.Matches(dram) || dram.cores_checked!=120 || audit.version!=1
		|| audit.state!=TT_HOST_DMA_AUDIT_PASSED || audit.tlb_restored!=1 || audit.recovery_required) return false;
	bool ready=c.q38Clock.Begin(c.baseline.baseline.baseline.baseline.control.control.control.arc,
		c.baseline.baseline.baseline.baseline.control.control.telemetry);
	const auto& r=c.q38Clock.Snapshot(); // CPU metadata ONLY,even after failure
	dprintf("P100A_Q38_CLOCK begin%u state%u mhz%u/%u mode%u/%u messages%u uncertain%u;NORMAL_FIRMWARE_ONLY\n",
		unsigned(ready),r.state,r.before_mhz,r.busy_mhz,r.before_mode,r.busy_mode,r.messages,unsigned(c.q38Clock.Unsafe()));
	return ready;
}
static bool
FinishQ38NormalClock(PioneerStoreContext& c,bool knownClean)
{
	// Native END calls this ONLY AFTER explicit span END, ALL peer STOPs,
	// independent physical audits/holds and complete bulk/root restoration.
	// It is BEFORE final PCI disable and before releasing ANY host owner.
	knownClean=knownClean && !c.Unsafe() && (!c.q38Span || c.q38Span->closed)
		&& (!c.q38Dispatch || c.q38Dispatch->closed);
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
	knownClean=knownClean && (!c.q38StateZero || c.q38StateZero->closed);
#endif
	bool ended=c.q38Clock.End(c.baseline.baseline.baseline.baseline.control.control.control.arc,
		c.baseline.baseline.baseline.baseline.control.control.telemetry,knownClean);
	const auto& r=c.q38Clock.Snapshot(); // CPU metadata ONLY;not another ARC query
	dprintf("P100A_Q38_CLOCK end%u state%u mhz%u/%u/%u restored%u messages%u uncertain%u;NORMAL_FIRMWARE_ONLY\n",
		unsigned(ended),r.state,r.before_mhz,r.busy_mhz,r.after_mhz,r.restored,r.messages,unsigned(c.q38Clock.Unsafe()));
	return ended;
}
// PIONEER_Q38_CLOCK_SERVICE_END
#endif

#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
// PIONEER_Q38_DISPATCH_RETAIN_BEGIN
// Memory bookkeeping ONLY. In particular,NO legacy PCI/TLB/SRAM rollback,
// mapping/host release,unreserve,context destruction or cookie destruction.
// PIONEER_Q38_RETAIN_MEMORY_BEGIN
static void
RetainQ38DispatchOwner(PioneerStoreContext& c,tt_resident_session_result& result)
{
	c.dispatchOwnerRetained=true;
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
	if (c.q38StateZero) c.q38StateZero->unsafe=true;
	if (c.q38GdnBanks) c.q38GdnBanks->Fault(); // MEMORY ONLY
#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
	if (c.q38ResidentGdnRoot) c.q38ResidentGdnRoot->unsafe=true; // MEMORY ONLY
#endif
#endif
#ifdef PIONEER_Q38_NORMAL_CLOCK
	c.q38Clock.Quarantine(); // MEMORY ONLY; never issue IDLE after ANY unknown
#endif
	if (c.q38Dispatch) PioneerQ38DispatchQuarantine(*c.q38Dispatch,c.banks);
	c.banks.Quarantine(); c.lease.recovery_required=true; c.io.QuarantineHostPage();
	if (c.bulkHost.allocated && !c.bulkHost.released) { c.bulkHost.ready=false; c.bulkHost.quarantined=true; }
	if (c.wideHost.allocated && !c.wideHost.released) { c.wideHost.ready=false; c.wideHost.quarantined=true; }
	sColdRecoveryRequired=true; result.active=1; result.host_released=0; result.cold=1;
	result.state=TT_RESIDENT_SESSION_CLEANUP_FAILED;
#ifdef _KERNEL_MODE
	const auto* d=c.q38Dispatch;
	dprintf("PIONEER-P100A:54 RETAIN end=%u close=%u worker=%u status=%08x/%08x/%08x prepared=%u booted=%u closed=%u;NO_LATER_CARD_IO\n",
		c.q38EndPhase,d?d->closePhase:0,d?d->closeWorker:0,d?d->closeStatus[0]:0,d?d->closeStatus[1]:0,d?d->closeStatus[2]:0,
		d?d->prepared:0,unsigned(d && d->booted),unsigned(d && d->closed));
	if (d) dprintf("PIONEER-P100A:54 DETAIL %08x/%08x/%08x/%08x;ALREADY_READ_VALUES_ONLY\n",
		d->closeDetail[0],d->closeDetail[1],d->closeDetail[2],d->closeDetail[3]);
#endif
}
// PIONEER_Q38_RETAIN_MEMORY_END
static bool
EndQ38DispatchOwnership(PioneerStoreContext& c,tt_resident_session_result& result,bool& clean)
{
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
	if (!c.bankStorageOnly && (c.q38GdnBanks || c.q38StateZero
#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
		|| c.q38ResidentGdnRoot
#endif
		)) {
		clean=false; RetainQ38DispatchOwner(c,result); return false; // misbound private scope:NEVER legacy rollback
	}
#endif
	if (!c.bankStorageOnly) return true;
#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
	if (c.q38ResidentGdnRoot && (!c.q38ResidentGdnRoot->closed || c.q38ResidentGdnRoot->unsafe
		|| c.q38ResidentGdnRoot->running || c.q38ResidentGdnRoot->awaiting || !c.q38ResidentGdnRoot->saved
		|| c.q38ResidentGdnRoot->dirty || c.q38ResidentGdnRoot->ready || !c.q38GdnBanks || !c.q38StateZero
		|| !c.q38StateZero->closed || !(c.q38StateZero->identity==c.q38GdnIdentity) || !c.q38GdnIdentity.Valid()
		|| !(c.q38ResidentGdnRoot->identity==c.q38GdnIdentity) || c.q38ResidentGdnRoot->owner!=c.q38GdnBanks
		|| c.q38ResidentGdnRoot->banks!=&c.banks || c.q38ResidentGdnRoot->proof!=&c.baseline.baseline.baseline.baseline.dram)) {
		clean=false; RetainQ38DispatchOwner(c,result); return false; // NO implicit debug ACCEPT/CLOSE
	}
#endif
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
	// Explicit state-zero CLOSE must precede BANK_END. Refuse BEFORE
	// delta/STOP/ARC/restore paths, even if the last device job passed.
	if ((c.q38GdnBanks && c.q38GdnBanks->Unsafe())
		|| (c.q38StateZero && (!c.q38StateZero->closed || c.q38StateZero->unsafe || c.q38StateZero->running))) {
		clean=false; RetainQ38DispatchOwner(c,result); return false;
	}
#endif
#ifdef PIONEER_Q38_LOCAL_DELTA_PROBE
	if (c.q38Delta.saved) clean=clean && PioneerQ38DeltaProbeClose(c.io,
		c.baseline.baseline.baseline.baseline.dram,c.baseline.baseline.baseline.noc,c.lease,c.banks,c.q38Delta,true);
#endif
#ifdef PIONEER_Q38_SPAN_PRELOAD
	// SPAN_END is an explicit prerequisite, never an implicit cleanup path.
	// A caller-only failure may be invisible to transport completion metadata.
	if (c.q38Span && !c.q38Span->closed) clean=false;
#endif
	if (clean && c.q38Dispatch) clean=PioneerQ38DispatchClose(c.io,c.baseline.baseline.baseline.baseline.dram,c.banks,*c.q38Dispatch,true);
	if (clean) return true;
	RetainQ38DispatchOwner(c,result); return false;
}
// PIONEER_Q38_DISPATCH_RETAIN_END
#endif

#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
// PIONEER_Q38_STATE_ZERO_NATIVE_ADAPTER_BEGIN
// Private native adapter,selected ONLY by the separate fixed zero diagnostic
// flag. Ordinary mutable/model compute remains REFUSED. No production call
// binds this owner/identity/seal. Existing owned mappings are BORROWED,not copied
// or allocated;no new reset address/host page/BME or public data capability.
// PIONEER_Q38_STATE_ZERO_NATIVE_SCOPE_BEGIN
bool PioneerSessionIO::StateZeroBeforeCohort()
{
	PioneerStoreContext* c=q38MutableContext;
	if (!c || &c->io!=this || sColdRecoveryRequired || sReloadQuiesced || !sRuntimeOwner
		|| sRuntimeOwner->magic!=0x50313030 || sRuntimeOwner->context!=c || !sBankStorageOnly || !c->bankStorageOnly
		|| c->bytes!=PioneerResidentBanks::Total || c->runtimeCoreMode!=1 || runtimeCore || fleetEnabled
		|| q38SpanAllowed || c->q38GdnCallThread<0 || c->q38GdnCallThread!=find_thread(NULL)
		|| !c->q38GdnBanks || !c->q38StateZero || !c->q38GdnIdentity.Valid()
		|| c->q38GdnIdentity.driverGeneration!=sReloadGeneration
		|| c->q38Dispatch || c->q38Cohort || c->fleet || sQ38LocalDeltaOwner
		|| c->q38Delta.saved || c->q38Delta.dirty || c->q38Delta.ready || c->q38Delta.running || c->q38Delta.awaiting
		|| c->panelProbe.saved || c->panelProbe.dirty || c->panelProbe.ready
		|| c->streamProbe.saved || c->streamProbe.dirty || c->streamProbe.ready) return false;
	if (c->q38Span && (!c->q38Span->closed || c->q38Span->dirty || c->q38Span->ready
		|| c->q38Span->running || c->q38Span->unsafe)) return false;
	const auto& zero=*c->q38StateZero;
#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
	if (c->q38ResidentGdnRoot) return false; // mutually exclusive root role,even after its clean CLOSE
#endif
	if (zero.identity.Valid()) { if (!(zero.identity==c->q38GdnIdentity)) return false; }
	else if (zero.identity.session || zero.identity.sequence || zero.identity.modelEpoch || zero.identity.driverGeneration
		|| zero.saved || zero.dirty || zero.ready || zero.running || zero.nonce) return false;
	const auto& clock=c->q38Clock.Snapshot();
	if (!c->q38Clock.RequiresExplicitEnd() || clock.state!=TT_ARC_CLOCK_BUSY_VERIFIED
		|| !clock.busy_requested || !clock.busy_acknowledged || clock.restored) return false;
	PioneerQ38GdnBankExtent probe;
	// Exempt ONLY this controlled root's running bit after proving the
	// exact context/identity/thread/clock and absence of competing owners.
	// ALL other uncertainty remains fatal;ordinary Unsafe() still rejects
	// this running bit for every cleanup/status/lost-caller path.
	return !c->Unsafe(true) && c->q38GdnBanks->MatchesInitializationOwner(c->banks,dram,c->q38GdnIdentity)
		&& c->q38GdnBanks->InitializeSpan(c->q38GdnIdentity,0,0,1,probe);
}
#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
// PIONEER_Q38_RGDN_ROOT_NATIVE_SCOPE_BEGIN
// CPU-only exact context/owner gate. Never a model/peer compute capability.
bool PioneerSessionIO::ResidentGdnBeforeRoot()
{
	PioneerStoreContext* c=q38MutableContext;
	if (!c || &c->io!=this || sColdRecoveryRequired || sReloadQuiesced || !sRuntimeOwner
		|| sRuntimeOwner->magic!=0x50313030 || sRuntimeOwner->context!=c || !sBankStorageOnly || !c->bankStorageOnly
		|| c->bytes!=PioneerResidentBanks::Total || c->runtimeCoreMode!=1 || runtimeCore || fleetEnabled || q38SpanAllowed
		|| c->q38GdnCallThread<0 || c->q38GdnCallThread!=find_thread(NULL) || !c->q38GdnBanks || !c->q38StateZero
		|| !c->q38ResidentGdnRoot || !c->q38GdnIdentity.Valid() || c->q38GdnIdentity.driverGeneration!=sReloadGeneration
		|| c->q38Dispatch || c->q38Cohort || c->fleet || sQ38LocalDeltaOwner
		|| c->q38Delta.saved || c->q38Delta.dirty || c->q38Delta.ready || c->q38Delta.running || c->q38Delta.awaiting
		|| c->panelProbe.saved || c->panelProbe.dirty || c->panelProbe.ready
		|| c->streamProbe.saved || c->streamProbe.dirty || c->streamProbe.ready) return false;
	if (c->q38Span && (!c->q38Span->closed || c->q38Span->dirty || c->q38Span->ready || c->q38Span->running || c->q38Span->unsafe)) return false;
	const auto& zero=*c->q38StateZero;
	if (!zero.closed || zero.running || zero.unsafe || zero.dirty || zero.ready || !(zero.identity==c->q38GdnIdentity)) return false;
	const auto& root=*c->q38ResidentGdnRoot;
	if (root.identity.Valid()) {
		if (!(root.identity==c->q38GdnIdentity) || root.owner!=c->q38GdnBanks || root.banks!=&c->banks || root.proof!=&dram) return false;
		if ((root.running && (!root.saved || !root.dirty || !root.ready || root.closed || root.awaiting))
			|| (root.awaiting && (!root.saved || !root.dirty || !root.ready || root.closed || root.running))
			|| (root.closed && (root.running || root.awaiting || root.dirty || root.ready))) return false;
	} else if (root.identity.session || root.identity.sequence || root.identity.modelEpoch || root.identity.driverGeneration
		|| root.saved || root.dirty || root.ready || root.running || root.unsafe || root.awaiting || root.closed || root.nonce
		|| root.readSlot || root.position || root.owner || root.banks || root.proof) return false;
	const auto& clock=c->q38Clock.Snapshot();
	if (!c->q38Clock.RequiresExplicitEnd() || clock.state!=TT_ARC_CLOCK_BUSY_VERIFIED
		|| !clock.busy_requested || !clock.busy_acknowledged || clock.restored) return false;
	// Exempt ONLY this exact controlled root's running bit. Every other
	// uncertainty is still fatal;ordinary Unsafe() blocks cleanup/END.
	return !c->Unsafe(true) && c->q38GdnBanks->MatchesInitializationOwner(c->banks,dram,c->q38GdnIdentity)
		&& c->q38GdnBanks->InitializationPublished(c->q38GdnIdentity);
}
// PIONEER_Q38_RGDN_ROOT_NATIVE_SCOPE_END
#endif
// PIONEER_Q38_NONROOT_NATIVE_OWNER_BEGIN
bool PioneerSessionIO::NonRootHeldOwner() {
	return StateZeroBeforeCohort()
#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
		|| ResidentGdnBeforeRoot()
#endif
		;
}
bool PioneerSessionIO::NonRootHeldMappingStable()
{
	return NonRootHeldOwner() && PioneerMmioIO::area>=B_OK && nodeID && tlbArea>=B_OK && tlb
		&& resetArea>=B_OK && reset;
}
// PIONEER_Q38_NONROOT_NATIVE_OWNER_END
bool PioneerSessionIO::StateZeroOtherCoresHeld()
{
	if (!StateZeroBeforeCohort() || !NonRootHeldMappingStable()) return false; // CPU-only refusal
	if (PioneerQ38NonRootHeld(*this,dram)) return true;
	// First physical readback mismatch:full MEMORY-ONLY context quarantine,
	// including clock/cookie/PCI/ALL host leases,BEFORE returning to caller.
	tt_resident_session_result result={}; RetainQ38DispatchOwner(*q38MutableContext,result); return false;
}
#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
// PIONEER_Q38_RGDN_ROOT_NATIVE_HELD_BEGIN
bool PioneerSessionIO::ResidentGdnOtherCoresHeld()
{
	if (!ResidentGdnBeforeRoot() || !NonRootHeldMappingStable()) return false;
	if (PioneerQ38NonRootHeld(*this,dram)) return true;
	tt_resident_session_result result={}; RetainQ38DispatchOwner(*q38MutableContext,result); return false; // MEMORY ONLY
}
// PIONEER_Q38_RGDN_ROOT_NATIVE_HELD_END
#endif
// PIONEER_Q38_STATE_ZERO_NATIVE_SCOPE_END
// PIONEER_Q38_GDN_CALL_SCOPE_BEGIN
// PRIVATE fixed zero diagnostic ONLY. Exactly ONE
// non-reentrant stack scope belongs at its control entry,not inside
// a controller/helper. FD lifetime keeps the cookie alive until return;
// context is captured/rechecked ONLY after acquiring the global lock.
// Destruction clears a CPU seal and releases that lock,NEVER END/STOP/
// restore/reset/ARC/unmap/free. All owners survive for explicit protocol END.
class PioneerQ38GdnCallScope {
	PioneerProbeCookie* cookie=NULL;
	PioneerStoreContext* context=NULL;
	thread_id thread=-1;
	status_t status=B_BAD_VALUE;
	bool held=false;
public:
	explicit PioneerQ38GdnCallScope(PioneerProbeCookie* owner) {
		if (!owner || owner->magic!=0x50313030) return;
		thread_info info={}; thread=find_thread(NULL);
		if (thread<0 || get_thread_info(thread,&info)!=B_OK || info.team!=owner->team) { status=B_NOT_ALLOWED; return; }
		status=AcquireQ38GdnSessionLock(owner); if (status!=B_OK) return;
		held=true; context=owner->context;
		if (owner->magic!=0x50313030 || sRuntimeOwner!=owner || !context || !sBankStorageOnly
			|| !context->bankStorageOnly || context->bytes!=PioneerResidentBanks::Total
			|| context->io.q38MutableContext!=context || context->q38GdnCallThread!=-1) {
			context=NULL; status=B_NOT_ALLOWED; held=false; release_sem(sLock); return;
		}
		if (context->Unsafe()) {
			tt_resident_session_result result={}; RetainQ38DispatchOwner(*context,result);
			context=NULL; status=B_DEV_NOT_READY; held=false; release_sem(sLock); return;
		}
		cookie=owner; context->q38GdnCallThread=thread;
	}
	PioneerQ38GdnCallScope(const PioneerQ38GdnCallScope&)=delete;
	PioneerQ38GdnCallScope& operator=(const PioneerQ38GdnCallScope&)=delete;
	~PioneerQ38GdnCallScope() {
		if (!held) return;
		if (!context || !cookie || cookie->context!=context || cookie->magic!=0x50313030
			|| sRuntimeOwner!=cookie || context->q38GdnCallThread!=thread || find_thread(NULL)!=thread) {
			if (context) { tt_resident_session_result result={}; RetainQ38DispatchOwner(*context,result); }
		} else context->q38GdnCallThread=-1;
		release_sem(sLock);
	}
	status_t Status() const { return status; }
	PioneerStoreContext* Context() const { return held && status==B_OK?context:NULL; }
};
// PIONEER_Q38_GDN_CALL_SCOPE_END
// PIONEER_Q38_GDN_BIND_OWNER_BEGIN
// Private CPU-only transition,fixed zero diagnostic ONLY. Caller
// holds the call-scope seal/global lock;no user coordinates or addresses.
// Fresh placement is a proposal;real Bind independently validates/claims
// every page. No initialization/model/compute readiness is published here.
[[maybe_unused]] static bool BindQ38MutableOwner(PioneerStoreContext& c,const PioneerQ38GdnIdentity& identity)
{
	if (sColdRecoveryRequired || sReloadQuiesced || !sRuntimeOwner || sRuntimeOwner->magic!=0x50313030
		|| sRuntimeOwner->context!=&c || !sBankStorageOnly || !c.bankStorageOnly || c.bytes!=PioneerResidentBanks::Total
		|| c.io.q38MutableContext!=&c || c.q38GdnCallThread<0 || c.q38GdnCallThread!=find_thread(NULL)
		|| c.runtimeCoreMode!=1 || c.io.runtimeCore || c.io.fleetEnabled || !c.initialized || c.Unsafe()
		|| !identity.Valid() || identity.driverGeneration!=sReloadGeneration || c.q38GdnBanks || c.q38StateZero
		|| c.q38GdnIdentity.session || c.q38GdnIdentity.sequence || c.q38GdnIdentity.modelEpoch || c.q38GdnIdentity.driverGeneration
		|| c.q38Dispatch || c.q38Cohort || c.fleet || sQ38LocalDeltaOwner
		|| c.q38Delta.saved || c.q38Delta.dirty || c.q38Delta.ready || c.q38Delta.running || c.q38Delta.awaiting
		|| c.panelProbe.saved || c.panelProbe.dirty || c.panelProbe.ready || c.streamProbe.saved || c.streamProbe.dirty || c.streamProbe.ready) return false;
	if (c.q38Span && (!c.q38Span->closed || c.q38Span->dirty || c.q38Span->ready || c.q38Span->running || c.q38Span->unsafe)) return false;
	const auto& clock=c.q38Clock.Snapshot();
	if (!c.q38Clock.RequiresExplicitEnd() || clock.state!=TT_ARC_CLOCK_BUSY_VERIFIED
		|| !clock.busy_requested || !clock.busy_acknowledged || clock.restored) return false;
	auto* plan=new(std::nothrow) PioneerQ38GdnFreshPlan;
	if (!plan) return false;
	if (!plan->Build(c.initialized,PioneerResidentBanks::Pages)) { delete plan; return false; }
	auto* owner=new(std::nothrow) PioneerQ38GdnBanks;
	auto* zero=new(std::nothrow) PioneerQ38StateZeroLease;
	if (!owner || !zero) { delete owner; delete zero; delete plan; return false; } // no claim/IO yet
	// Publish ALL heap owners before a claim can succeed. A failed/partial
	// claim then retains the same context rather than freeing a referenced
	// owner. Only temporary CPU proposal storage is disposable afterward.
	c.q38GdnBanks=owner; c.q38StateZero=zero; c.q38GdnIdentity=identity; c.io.q38SpanAllowed=false;
	bool okay=owner->Bind(c.banks,c.baseline.baseline.baseline.baseline.dram,identity,c.initialized,
		PioneerResidentBanks::Pages,plan->Regions(),plan->Count());
	delete plan;
	if (!okay) { tt_resident_session_result r={}; RetainQ38DispatchOwner(c,r); }
	return okay;
}
// PIONEER_Q38_GDN_BIND_OWNER_END
[[maybe_unused]] static bool InitializeQ38MutableState(PioneerStoreContext& c,PioneerQ38InitializeOwnedResult& result)
{
	if (!c.io.StateZeroBeforeCohort()) {
		if (c.Unsafe()) { tt_resident_session_result r={}; RetainQ38DispatchOwner(c,r); }
		return false;
	}
	bool okay=PioneerQ38InitializeOwnedState(c.io,c.baseline.baseline.baseline.baseline.dram,
		c.baseline.baseline.baseline.noc,*c.q38GdnBanks,c.banks,c.q38GdnIdentity,c.host,
		c.lease,c.bulk,c.bulkHost,c.wideHost,*c.q38StateZero,result);
	if (!okay && c.Unsafe()) { tt_resident_session_result r={}; RetainQ38DispatchOwner(c,r); }
	return okay;
}
#ifdef PIONEER_Q38_GDN_ZERO_DIAGNOSTIC
static bool QualifyQ38MutableState(PioneerStoreContext& c,PioneerQ38InitializeOwnedResult& result)
{
	if (!c.io.StateZeroBeforeCohort()) {
		if (c.Unsafe()) { tt_resident_session_result r={}; RetainQ38DispatchOwner(c,r); }
		return false;
	}
	bool okay=PioneerQ38QualifyOwnedState(c.io,c.baseline.baseline.baseline.baseline.dram,
		c.baseline.baseline.baseline.noc,*c.q38GdnBanks,c.banks,c.q38GdnIdentity,c.host,
		c.lease,c.bulk,c.bulkHost,c.wideHost,*c.q38StateZero,result);
	if (!okay && c.Unsafe()) { tt_resident_session_result r={}; RetainQ38DispatchOwner(c,r); }
	return okay;
}
// PIONEER_Q38_GDN_ZERO_CONTROL_BEGIN
// Separately GATED fixed zero QUALIFICATION,not ordinary mutable inference.
// The exact closed-span handoff supplies the existing ready owners/views;
// no generic setup/cleanup is attempted if a prerequisite is absent.
static uint64_t sQ38GdnDiagnosticEpoch=0; // global lock;never wrap/reuse
static status_t Q38GdnZeroControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	if (!cookie || cookie->magic!=0x50313030 || !data) return B_BAD_VALUE;
	thread_info info={}; status_t status=get_thread_info(find_thread(NULL),&info);
	if (status!=B_OK) return status;
	if (info.team!=cookie->team) return B_NOT_ALLOWED;
	if (operation==TT_PCI_PROBE_Q38_GDN_ZERO_SUPPORT) {
		if (length!=sizeof(tt_q38_gdn_zero_support)) return B_BAD_VALUE;
		status=AcquireQ38GdnSessionLock(cookie); if (status!=B_OK) return status;
		const tt_q38_gdn_zero_support support={1,55,1,73728,672,1,0,1};
		status=user_memcpy(data,&support,sizeof(support)); release_sem(sLock); return status;
	}
	if (operation!=TT_PCI_PROBE_Q38_GDN_ZERO_INITIALIZE || length!=sizeof(tt_q38_gdn_zero_request)) return B_BAD_VALUE;
	tt_q38_gdn_zero_request request={}; status=user_memcpy(&request,data,sizeof(request));
	if (status!=B_OK || request.version!=1 || (request.mode!=1 && request.mode!=2) || request.reserved[0] || request.reserved[1])
		return status!=B_OK?status:B_BAD_VALUE;
	PioneerQ38GdnCallScope scope(cookie); if (scope.Status()!=B_OK) return scope.Status();
	auto* c=scope.Context();
	if (!c || c->runtimeCoreMode!=-1 || c->q38GdnBanks || c->q38StateZero || !c->q38Span || !c->q38Span->closed
		|| c->q38Span->dirty || c->q38Span->ready || c->q38Span->running || c->q38Span->unsafe
		|| !c->io.q38SpanAllowed || !c->lease.saved || !c->lease.ready || c->lease.pending || c->lease.recovery_required
		|| !c->bulk.saved || !c->bulk.ready || c->bulk.dirty || c->bulk.unsafe
		|| !c->bulkHost.allocated || !c->bulkHost.ready || c->bulkHost.pending || c->bulkHost.quarantined || c->bulkHost.released
		|| !c->wideHost.ready || !c->wideHost.memory_type_set || c->wideHost.pending || c->wideHost.quarantined || c->wideHost.released
		|| sQ38GdnDiagnosticEpoch==UINT64_MAX) return B_NOT_ALLOWED; // CPU-only,NO partial setup
	// Private epochs are minted under the lock,not accepted from the user.
	// This fixed diagnostic epoch is NEVER a real-model binding/capability.
	PioneerQ38GdnIdentity identity={++sQ38GdnDiagnosticEpoch,1,UINT64_C(0x5147385a45524f31),sReloadGeneration};
	// SPAN_END leaves mode==-1. This is the ONLY one-way CPU handoff to
	// the private root-only mode1;do not assume SPAN_END selected it. After
	// this point even a CPU allocation refusal retains the full owner/clock.
	c->runtimeCoreMode=1; c->io.q38SpanAllowed=false;
	if (!BindQ38MutableOwner(*c,identity)) { tt_resident_session_result r={}; RetainQ38DispatchOwner(*c,r); return B_DEV_NOT_READY; }
	PioneerQ38InitializeOwnedResult result;
	unsigned mode=request.mode;
	bool okay=mode==1?QualifyQ38MutableState(*c,result):InitializeQ38MutableState(*c,result);
	okay=okay && result.regions==(mode==1?1U:672U) && result.jobs==(mode==1?1U:672U) && result.pages==(mode==1?1U:73728U)
		&& result.closed==1 && result.published==(mode==1?0U:1U) && !result.recovery_required && !c->Unsafe();
	request={}; request.version=1; request.mode=mode; request.state=okay?TT_RUNTIME_PASSED:TT_RUNTIME_COMPLETION_FAILED;
	request.regions=result.regions; request.jobs=result.jobs; request.pages=result.pages; request.closed=result.closed;
	request.initialization_published=okay && result.published; request.recovery_required=!okay || c->Unsafe();
	// Any failure AFTER binding,including failed reply delivery,retains ALL
	// owners. Never implicit CLOSE/END/STOP/ARC/restore/reset/retry/free here.
	status=user_memcpy(data,&request,sizeof(request));
	if (!okay || status!=B_OK || c->Unsafe()) { tt_resident_session_result r={}; RetainQ38DispatchOwner(*c,r); }
	return status;
}
// PIONEER_Q38_GDN_ZERO_CONTROL_END
#endif
[[maybe_unused]] static bool PrepareQ38MutableZero(PioneerStoreContext& c)
{
	if (!c.io.StateZeroBeforeCohort()) {
		if (c.Unsafe()) { tt_resident_session_result r={}; RetainQ38DispatchOwner(c,r); }
		return false;
	}
	bool okay=PioneerQ38StateZeroPrepare(c.io,c.baseline.baseline.baseline.baseline.dram,
		c.baseline.baseline.baseline.noc,*c.q38GdnBanks,c.banks,c.q38GdnIdentity,c.host,
		c.lease,c.bulk,c.bulkHost,c.wideHost,*c.q38StateZero);
	if (!okay && c.Unsafe()) { tt_resident_session_result r={}; RetainQ38DispatchOwner(c,r); }
	return okay;
}
[[maybe_unused]] static bool RunQ38MutableZero(PioneerStoreContext& c,unsigned region,unsigned first,unsigned pages,
	PioneerQ38StateZeroResult& result)
{
	if (!c.io.StateZeroBeforeCohort()) {
		if (c.Unsafe()) { tt_resident_session_result r={}; RetainQ38DispatchOwner(c,r); }
		return false;
	}
	bool okay=PioneerQ38StateZeroRun(c.io,c.baseline.baseline.baseline.baseline.dram,
		c.baseline.baseline.baseline.noc,*c.q38GdnBanks,c.banks,c.host,c.lease,c.bulk,
		c.bulkHost,c.wideHost,*c.q38StateZero,c.q38GdnIdentity,region,first,pages,result);
	if (!okay && c.Unsafe()) { tt_resident_session_result r={}; RetainQ38DispatchOwner(c,r); }
	return okay;
}
[[maybe_unused]] static bool CloseQ38MutableZero(PioneerStoreContext& c,bool knownClean)
{
	if (!c.io.StateZeroBeforeCohort()) {
		if (c.Unsafe()) { tt_resident_session_result r={}; RetainQ38DispatchOwner(c,r); }
		return false;
	}
	bool okay=PioneerQ38StateZeroClose(c.io,c.baseline.baseline.baseline.baseline.dram,
		c.baseline.baseline.baseline.noc,*c.q38GdnBanks,c.banks,c.host,c.lease,c.bulk,
		c.bulkHost,c.wideHost,*c.q38StateZero,knownClean);
	if (!okay && c.Unsafe()) { tt_resident_session_result r={}; RetainQ38DispatchOwner(c,r); }
	return okay;
}
// PIONEER_Q38_STATE_ZERO_NATIVE_ADAPTER_END
#endif

#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
// PIONEER_Q38_RGDN_ROOT_CONTROL_BEGIN
// Separate ROOT0 QUALIFICATION control entry,not ordinary model/Work.
// No arbitrary placement/state inputs. Same fully initialized owner only.
static bool Q38RgdnRootParent(PioneerStoreContext& c)
{
	return c.q38GdnBanks && c.q38StateZero && c.q38StateZero->closed && !c.q38StateZero->unsafe
		&& !c.q38StateZero->running && !c.q38StateZero->dirty && !c.q38StateZero->ready
		&& c.q38GdnIdentity.Valid() && c.q38GdnIdentity.driverGeneration==sReloadGeneration
		&& c.q38StateZero->identity==c.q38GdnIdentity && c.runtimeCoreMode==1 && c.io.q38MutableContext==&c
		&& c.q38GdnBanks->MatchesInitializationOwner(c.banks,c.baseline.baseline.baseline.baseline.dram,c.q38GdnIdentity)
		&& c.q38GdnBanks->InitializationPublished(c.q38GdnIdentity);
}
static status_t Q38RgdnRootControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	if (!cookie || cookie->magic!=0x50313030 || !data) return B_BAD_VALUE;
	thread_info info={}; status_t status=get_thread_info(find_thread(NULL),&info);
	if (status!=B_OK) return status;
	if (info.team!=cookie->team) return B_NOT_ALLOWED;
	if (operation==TT_PCI_PROBE_Q38_RGDN_ROOT_SUPPORT) {
		if (length!=sizeof(tt_q38_rgdn_root_support)) return B_BAD_VALUE;
		status=AcquireQ38GdnSessionLock(cookie); if (status!=B_OK) return status;
		const tt_q38_rgdn_root_support support={1,55,0,0,128,64,1,2,1,0};
		status=user_memcpy(data,&support,sizeof(support)); release_sem(sLock); return status;
	}
	if (operation==TT_PCI_PROBE_Q38_RGDN_ROOT_ACCEPT) {
		if (length!=sizeof(tt_q38_rgdn_root_accept)) return B_BAD_VALUE;
		tt_q38_rgdn_root_accept a={}; status=user_memcpy(&a,data,sizeof(a));
		if (status!=B_OK || a.version!=1 || !a.nonce || a.reserved[0] || a.reserved[1]) return status!=B_OK?status:B_BAD_VALUE;
		PioneerQ38GdnCallScope scope(cookie); if (scope.Status()!=B_OK) return scope.Status();
		auto* c=scope.Context();
		return c && Q38RgdnRootParent(*c) && c->q38ResidentGdnRoot && c->io.ResidentGdnBeforeRoot()
			&& PioneerQ38RgdnRootAccept(*c->q38ResidentGdnRoot,a)?B_OK:B_NOT_ALLOWED; // CPU-only ACK
	}
	if (operation==TT_PCI_PROBE_Q38_RGDN_ROOT_END) {
		if (length!=sizeof(tt_q38_rgdn_root_end)) return B_BAD_VALUE;
		tt_q38_rgdn_root_end r={}; status=user_memcpy(&r,data,sizeof(r));
		if (status!=B_OK || r.version!=1 || r.reserved) return status!=B_OK?status:B_BAD_VALUE;
		PioneerQ38GdnCallScope scope(cookie); if (scope.Status()!=B_OK) return scope.Status();
		auto* c=scope.Context(); if (!c || !Q38RgdnRootParent(*c)) return B_NOT_ALLOWED;
		bool okay=!c->q38ResidentGdnRoot || (c->io.ResidentGdnBeforeRoot() && PioneerQ38RgdnRootClose(c->io,c->baseline.baseline.baseline.baseline.dram,
			c->baseline.baseline.baseline.noc,*c->q38GdnBanks,c->banks,c->host,c->lease,c->bulk,c->bulkHost,c->wideHost,*c->q38ResidentGdnRoot,true));
		r={1,0,unsigned(okay),unsigned(!okay || c->Unsafe())};
		status=user_memcpy(data,&r,sizeof(r));
		if (!okay || status!=B_OK || c->Unsafe()) { tt_resident_session_result retained={}; RetainQ38DispatchOwner(*c,retained); }
		return status;
	}
	if (operation!=TT_PCI_PROBE_Q38_RGDN_ROOT_RUN || length!=sizeof(tt_q38_rgdn_root_request)) return B_BAD_VALUE;
	auto* job=(tt_q38_rgdn_root_request*)malloc(sizeof(tt_q38_rgdn_root_request)); if (!job) return B_NO_MEMORY;
	status=user_memcpy(job,data,sizeof(*job));
	if (status!=B_OK || !PioneerQ38RgdnRootValid(*job)) { free(job); return status!=B_OK?status:B_BAD_VALUE; }
	{
		PioneerQ38GdnCallScope scope(cookie); auto* c=scope.Context();
		if (scope.Status()!=B_OK) status=scope.Status();
		else if (!c || !Q38RgdnRootParent(*c) || (c->q38ResidentGdnRoot && (c->q38ResidentGdnRoot->closed
			|| c->q38ResidentGdnRoot->running || c->q38ResidentGdnRoot->unsafe || c->q38ResidentGdnRoot->awaiting))) status=B_NOT_ALLOWED;
		else {
			if (!c->q38ResidentGdnRoot) c->q38ResidentGdnRoot=new(std::nothrow) PioneerQ38ResidentGdnRootLease;
			if (!c->q38ResidentGdnRoot) status=B_NO_MEMORY; // no partial owner or physical work
			else {
				job->result={}; job->result.version=1; job->result.state=TT_RUNTIME_CONFIG_FAILED;
				for (auto& w:job->next) w=0;
				for (auto& w:job->output) w=0;
				auto& root=*c->q38ResidentGdnRoot;
				bool ready=root.ready;
				if (!ready && !root.saved) ready=PioneerQ38RgdnRootPrepare(c->io,c->baseline.baseline.baseline.baseline.dram,
					c->baseline.baseline.baseline.noc,*c->q38GdnBanks,c->banks,c->q38GdnIdentity,c->host,c->lease,c->bulk,c->bulkHost,c->wideHost,root);
				bool passed=ready && PioneerQ38RgdnRootRun(c->io,c->baseline.baseline.baseline.baseline.dram,
					c->baseline.baseline.baseline.noc,*c->q38GdnBanks,c->banks,c->host,c->lease,c->bulk,c->bulkHost,c->wideHost,root,*job);
				if (!passed || c->Unsafe()) job->result.recovery_required=1;
				status=user_memcpy(data,job,sizeof(*job));
				if (!passed || status!=B_OK || c->Unsafe()) { tt_resident_session_result retained={}; RetainQ38DispatchOwner(*c,retained); }
			}
		}
	} // clears ONLY thread seal/releases semaphore,NEVER END/cleanup
	free(job); return status;
}
// PIONEER_Q38_RGDN_ROOT_CONTROL_END
#endif

// sLock held. On uncertainty only the established PCI/TLB rollback is used;
// a wired system-team page is retained, never freed through a destructor.
static bool
CloseBulkScratch(PioneerStoreContext& context,bool safe)
{
	if (!context.bulk.saved) return safe;
	if (safe) safe=context.io.SelectRuntimeCore(0);
	if (!safe) context.bulk.unsafe=true;
	return PioneerBulkUploadClose(context.io,context.baseline.baseline.baseline.baseline.dram,
		context.baseline.baseline.baseline.noc,context.host,context.lease,context.bulkHost,context.bulk) && safe;
}

static bool
FleetHeld(PioneerStoreContext& context)
{
	if (!context.fleet) return true;
	auto& dram=context.baseline.baseline.baseline.baseline.dram;
	if (!PioneerFleetGlobalSources(context.io,dram)) return false;
	for (unsigned w=0;w<TT_PIONEER_WORKERS;w++) {
		auto& f=context.fleet[w]; if (!f.Store().ready) continue;
		if (!context.io.SelectRuntimeCore(w) || context.io.ResetRead()!=0x47800 || context.io.ResetRead()!=0x47800
			|| !BlackholeResidentIdle(context.io) || !PioneerFleetLocalSources(context.io,f.noc,f.Store())) return false;
		if (f.Compute().ready && f.preludeDirty) {
			uint32 entry=context.io.RuntimeCodeRead(0);
			if (entry!=0x0006b06f && (w || !context.attention.ready || entry!=TT_ATTENTION_TILE_BOOT)) return false;
		}
	}
	return context.io.SelectRuntimeCore(0) && PioneerFleetGlobalSources(context.io,dram);
}

static bool
SelectFleetEntry(PioneerStoreContext& context,bool attention,bool alreadyHeld=false)
{
	// alreadyHeld is private to one locked ioctl: between the audit and
	// this switch, only held-core code upload/readback has been performed.
	bool okay=(alreadyHeld || FleetHeld(context)) && (!attention || context.attention.ready);
	if (okay) {
		uint32 selected=attention?uint32(TT_ATTENTION_TILE_BOOT):0x0006b06f;
		context.io.RuntimeCodeWrite(0,selected); okay=context.io.RuntimeCodeRead(0)==selected;
	}
	if (!okay) {
		context.compute.unsafe=true; context.lease.recovery_required=true;
		context.attention.unsafe=true; context.io.QuarantineHostPage();
	}
	return okay;
}

static bool
SelectResidentFleetEntry(PioneerStoreContext& context,bool attention)
{
	bool okay=context.residentAttention.reserved && (!attention || context.attention.ready)
		&& (context.runtimeCoreMode==8
			?PioneerCoordinatedMasterBoundary(context.io,context.baseline.baseline.baseline.baseline.dram,context.fleet,context.attention.ready)
			:PioneerFleetMasterBoundary(context.io,context.baseline.baseline.baseline.baseline.dram,context.fleet,context.attention.ready));
	if (okay) {
		uint32 selected=attention?uint32(TT_ATTENTION_TILE_BOOT):0x0006b06f;
		context.io.RuntimeCodeWrite(0,selected); okay=context.io.RuntimeCodeRead(0)==selected;
	}
	if (!okay) {
		context.compute.unsafe=true; context.lease.recovery_required=true;
		context.attention.unsafe=true; context.io.QuarantineHostPage();
	}
	return okay;
}

static bool
CloseFleet(PioneerStoreContext& context,bool clean)
{
	if (!context.fleet) return clean;
	if (clean && (context.runtimeCoreMode==6 || context.runtimeCoreMode==7 || context.runtimeCoreMode==8 || context.runtimeCoreMode==9)) clean=PioneerPersistentStop(context.io,
		context.baseline.baseline.baseline.baseline.dram,context.fleet);
	if (clean) clean=FleetHeld(context);
	// Establish the global held/idle proof BEFORE restoring any one peer.
	if (!clean) for (unsigned w=0;w<TT_PIONEER_WORKERS;w++) {
		context.fleet[w].Compute().unsafe=true; context.fleet[w].Store().recovery_required=true;
	}
	for (unsigned end=TT_PIONEER_WORKERS;end;end--) {
		unsigned w=end-1; auto& f=context.fleet[w];
		if (!f.Compute().saved && !f.Store().controls_saved && !f.preludeSaved) continue;
		if (!context.io.SelectRuntimeCore(w)) clean=false;
		if (w && w<=8 && context.peerAttention[w-1].saved) {
			if (!clean) context.peerAttention[w-1].unsafe=true;
			clean=PioneerAttentionTileClose(context.io,context.baseline.baseline.baseline.baseline.dram,
				f.noc,f.Store(),context.peerAttention[w-1]) && clean;
		}
		clean=PioneerFleetPreludeClose(context.io,f,clean) && clean;
		if (w) {
			if (!clean) { f.Compute().unsafe=true; f.Store().recovery_required=true; }
			clean=PioneerRuntimeMatvecClose(context.io,context.baseline.baseline.baseline.baseline.dram,
				f.noc,f.Store(),f.Compute()) && clean;
			clean=PioneerResidentLeaseEnd(context.io,f.noc,f.Store()) && clean;
		}
	}
	return context.io.SelectRuntimeCore(0) && clean;
}

static void
EndResidentSession(PioneerProbeCookie* cookie,tt_resident_session_result& result,bool recheck)
{
	PioneerStoreContext* context=cookie->context;
	context->Snapshot(result);
	bool clean=!sColdRecoveryRequired && !context->Unsafe();
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
	if (!context->dispatchOwnerRetained) context->q38EndPhase=10;
	if (!EndQ38DispatchOwnership(*context,result,clean)) return;
	if (context->bankStorageOnly) {
		// Dedicated54 clean path:single-attempt bulk/root restoration,never
		// fall through to legacy root-lease retry/rollback after a failure.
		context->q38EndPhase=20;
		if (!CloseBulkScratch(*context,true)) { RetainQ38DispatchOwner(*context,result); return; }
		context->q38EndPhase=30;
		if (context->lease.saved && !PioneerQ38DispatchPeerLeaseEnd(context->io,
			context->baseline.baseline.baseline.noc,context->lease)) { RetainQ38DispatchOwner(*context,result); return; }
		goto q38_dispatch_owned_restored;
	}
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT
	if (context->q38Cohort) clean=PioneerQ38CohortClose(context->io,context->baseline.baseline.baseline.baseline.dram,
		context->banks,*context->q38Cohort,clean) && clean;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_STREAM
	clean=PioneerQ38PanelClose(context->io,context->baseline.baseline.baseline.baseline.dram,
		context->baseline.baseline.baseline.noc,context->lease,context->banks,context->streamProbe,clean) && clean;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_PANEL
	clean=PioneerQ38PanelClose(context->io,context->baseline.baseline.baseline.baseline.dram,
		context->baseline.baseline.baseline.noc,context->lease,context->banks,context->panelProbe,clean) && clean;
#endif
	clean=CloseFleet(*context,clean);
	clean=PioneerDecoderClose(context->io,context->baseline.baseline.baseline.baseline.dram,
		context->baseline.baseline.baseline.noc,context->lease,context->decoder,clean) && clean;
	clean=PioneerLayerClose(context->io,context->baseline.baseline.baseline.baseline.dram,
		context->baseline.baseline.baseline.noc,context->lease,context->layer,clean) && clean;
	clean=PioneerCoordinatorClose(context->io,context->baseline.baseline.baseline.baseline.dram,
		context->baseline.baseline.baseline.noc,context->lease,context->coordinator,clean) && clean;
	if (!context->secondCompute.saved && !context->secondLease.saved) clean=CloseBulkScratch(*context,clean);
	if (context->attention.saved && !context->secondCompute.saved && !context->secondLease.saved) {
		if (!clean) context->attention.unsafe=true;
		clean=PioneerAttentionTileClose(context->io,context->baseline.baseline.baseline.baseline.dram,
			context->baseline.baseline.baseline.noc,context->lease,context->attention) && clean;
	}
	if (context->secondCompute.saved || context->secondLease.saved) {
		// Global uncertainty forbids restoring even a known-finished peer.
		if (clean && context->lease.ready && context->secondLease.ready) {
			clean=context->io.SelectRuntimeCore(0) && context->io.ResetRead()==0x47800
				&& BlackholeResidentIdle(context->io) && PioneerRuntimeSources(context->io,
					context->baseline.baseline.baseline.baseline.dram,context->baseline.baseline.baseline.noc,context->lease);
			clean=context->io.SelectRuntimeCore(1) && clean && context->io.ResetRead()==0x47800
				&& BlackholeResidentIdle(context->io) && PioneerRuntimeSources(context->io,
					context->baseline.baseline.baseline.baseline.dram,context->secondNoc,context->secondLease);
		}
		// All workers must be known held before restoring either program's
		// shared boot word. Attention saved the matrix entry; matrix close
		// later restores the original firmware entry in the opposite order.
		clean=CloseBulkScratch(*context,clean);
		if (context->attention.saved) {
			if (!context->io.SelectRuntimeCore(0)) clean=false;
			if (!clean) context->attention.unsafe=true;
			clean=PioneerAttentionTileClose(context->io,context->baseline.baseline.baseline.baseline.dram,
				context->baseline.baseline.baseline.noc,context->lease,context->attention) && clean;
		}
		if (!context->io.SelectRuntimeCore(1)) clean=false;
		if (!clean) { context->secondCompute.unsafe=true; context->secondLease.recovery_required=true; }
		clean=PioneerRuntimeMatvecClose(context->io,context->baseline.baseline.baseline.baseline.dram,
			context->secondNoc,context->secondLease,context->secondCompute) && clean;
		clean=PioneerResidentLeaseEnd(context->io,context->secondNoc,context->secondLease) && clean;
		if (!context->io.SelectRuntimeCore(0)) clean=false;
	}
	if (context->compute.saved) {
		if (!clean) context->compute.unsafe=true;
		clean=PioneerRuntimeMatvecClose(context->io,context->baseline.baseline.baseline.baseline.dram,
			context->baseline.baseline.baseline.noc,context->lease,context->compute) && clean;
	}
	if (context->lease.saved!=NULL) {
		if (!clean) context->lease.recovery_required=true;
		clean=PioneerResidentLeaseEnd(context->io,context->baseline.baseline.baseline.noc,context->lease) && clean;
	}
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
q38_dispatch_owned_restored:
	context->q38EndPhase=40;
#endif
#ifdef PIONEER_Q38_NORMAL_CLOCK
	if (context->bankStorageOnly && !FinishQ38NormalClock(*context,clean)) {
		RetainQ38DispatchOwner(*context,result); return;
	}
#endif
	if (recheck && clean) {
		clean=context->io.PioneerHostDmaAuditIO::AfterNodeId(context->Pci().node_id[0])
			&& BlackholeHostReadPciQuiet(context->io,0x402)
			&& PioneerResidentSessionIdle(context->io,context->baseline.baseline.baseline.noc,context->idle);
	}
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
	if (context->bankStorageOnly) {
		context->q38EndPhase=50;
		if (!clean || !PioneerQ38DispatchMmioEnd(context->io,context->Pci(),context->transaction)) { RetainQ38DispatchOwner(*context,result); return; }
	} else
#endif
	BlackholePioneerMmioEnd(context->io,context->Pci(),context->transaction);
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
	context->q38EndPhase=60;
#endif
	clean &= !context->Unsafe() && context->Pci().restored;
	result.active=0; result.pci_restored=context->Pci().restored;
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
	context->q38EndPhase=70;
#endif
	if (context->wideHost.allocated) {
		if (!clean) PioneerQuarantineWideOutput(context->io,context->wideHost);
		else clean=PioneerReleaseWideOutput(context->io,context->wideHost,true) && clean;
	}
	if (context->bulkHost.allocated) {
		if (!clean) PioneerQuarantineOutputBuffer(context->io,context->bulkHost);
		else clean=PioneerReleaseOutputBuffer(context->io,context->bulkHost,true) && clean;
	}
	if (context->host.allocated) {
		if (!clean) context->io.QuarantineHostPage();
		else {
			context->host.release_status=context->io.ReleaseHostPage();
			context->host.released=context->host.release_status==B_OK;
		}
	}
	result.host_released=(!context->host.allocated || context->host.released)
		&& (!context->bulkHost.allocated || context->bulkHost.released) && (!context->wideHost.allocated || context->wideHost.released);
	if (!clean || !result.host_released) sColdRecoveryRequired=true;
	result.cold=sColdRecoveryRequired;
	result.state=clean && result.host_released?TT_RESIDENT_SESSION_CLOSED:TT_RESIDENT_SESSION_CLEANUP_FAILED;
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
	if (context->bankStorageOnly && (!clean || !result.host_released)) { RetainQ38DispatchOwner(*context,result); return; }
#endif
#ifdef PIONEER_Q38_HOST_PHASE_PROFILE
	// PIONEER_Q38_HOST_PHASE_PUBLICATION_BEGIN
	if (context->io.q38HostPhaseReportAllowed && context->bankStorageOnly && clean && result.host_released && !result.cold) context->io.Q38HostPhaseReport();
	// PIONEER_Q38_HOST_PHASE_PUBLICATION_END
#endif
	sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
	cookie->context=NULL; sRuntimeOwner=NULL;
#ifdef PIONEER_Q38_LOCAL_DELTA_PROBE
	sQ38LocalDeltaOwner=NULL;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_BANKS
	sBankStorageOnly=false;
#endif
	delete context;
}

// PIONEER_Q38_DISPATCH_COOKIE_RELEASE_BEGIN
static status_t
ReleaseResidentCookie(PioneerProbeCookie* cookie)
{
	if (cookie==NULL || cookie->magic!=0x50313030) return B_BAD_VALUE;
	if (cookie->context!=NULL) {
		// Closing must also work after the cold latch: do not use the
		// ordinary gate, which deliberately refuses all further card work.
		status_t status=acquire_sem(sLock);
		if (status!=B_OK) return status; // retain ownership, never silently free a possibly live page
		tt_resident_session_result result={};
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
		// Scope presence alone requires explicit END,even if a future
		// allocation bug misbound the context as a legacy/non-bank session.
		if (cookie->context->q38GdnBanks || cookie->context->q38StateZero)
			RetainQ38DispatchOwner(*cookie->context,result);
		else
#endif
#ifdef TT_PIONEER_Q38_WORK_MODE
		// After a persistent RUN,ONLY explicit BANK_END authorizes normal
		// cleanup. A userspace CPU-reference/graph failure can be invisible
		// to the kernel:close WITHOUT END must retain ALL owners with NO
		// device IO/reset/STOP,even when the last RUN's metadata passed.
		if (cookie->context->bankStorageOnly && ((cookie->context->q38Dispatch && !cookie->context->q38Dispatch->closed)
#ifdef PIONEER_Q38_NORMAL_CLOCK
			|| cookie->context->q38Clock.RequiresExplicitEnd()
#endif
#ifdef PIONEER_Q38_LOCAL_DELTA_PROBE
			|| cookie->context->q38Delta.saved
#endif
#ifdef PIONEER_Q38_SPAN_PRELOAD
			|| cookie->context->q38Span
#endif
			))
			RetainQ38DispatchOwner(*cookie->context,result);
		else
#endif
		EndResidentSession(cookie,result,!sColdRecoveryRequired);
		release_sem(sLock);
	}
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
	if (cookie->context && cookie->context->dispatchOwnerRetained) return B_OK; // retain cookie/owner too until cold recovery
#endif
	cookie->magic=0; delete cookie; return B_OK;
}
// PIONEER_Q38_DISPATCH_COOKIE_RELEASE_END

// sLock held; exact capacity/policy has already been selected by its ABI.
static status_t
BeginResidentSession(PioneerProbeCookie* cookie,uint64 capacity,tt_resident_session_result& result,bool bankOnly=false)
{
	if (cookie->context!=NULL) return B_BUSY;
	PioneerStoreContext* context=new(std::nothrow) PioneerStoreContext(capacity);
	if (context==NULL) return B_NO_MEMORY;
	if (context->initialized==NULL || context->hashes==NULL) { delete context; return B_NO_MEMORY; }
	status_t status=sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
	if (status!=B_OK) { delete context; return status; }
	if (sDeviceManager==NULL) status=get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
	if (status==B_OK) status=GetPioneerPciProfile(sDeviceManager,sCard,context->profile);
	context->profile.status=status;
	bool ready=status==B_OK && BlackholePioneerMmioBegin(context->io,context->Pci(),context->transaction)
		&& context->io.PioneerHostDmaAuditIO::AfterNodeId(context->Pci().node_id[0])
		&& PioneerResidentSessionIdle(context->io,context->baseline.baseline.baseline.noc,context->idle);
	unsigned failure=TT_RESIDENT_SESSION_PREFLIGHT_FAILED;
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_BANKS
	context->bankStorageOnly=bankOnly;
	sBankStorageOnly=bankOnly;
	if (ready && bankOnly) ready=context->banks.Bind(context->baseline.baseline.baseline.baseline.dram);
#ifdef PIONEER_Q38_NORMAL_CLOCK
	if (ready && bankOnly) ready=BeginQ38NormalClock(*context);
#endif
#else
	(void)bankOnly;
#endif
	if (ready) { failure=TT_RESIDENT_SESSION_HOST_FAILED; ready=PioneerPrepareHostPage(context->io,context->host); }
	cookie->context=context; sRuntimeOwner=cookie;
	context->Snapshot(result);
	if (!ready) {
		if (!context->transaction.config_attempted && context->Pci().state!=TT_PIONEER_MMIO_READ_OK)
			context->Pci().restored=1;
		EndResidentSession(cookie,result,false);
		if (!result.cold && result.host_released) result.state=failure;
	}
	return B_OK;
}

static status_t
ResidentSessionControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	if (data==NULL || length!=sizeof(tt_resident_session_result)) return B_BAD_VALUE;
	thread_info thread={};
	status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	tt_resident_session_result result={};
	if (operation==TT_PCI_PROBE_PIONEER_SESSION_BEGIN) {
		status=user_memcpy(&result,data,sizeof(result)); if (status!=B_OK) return status;
		if (result.version!=1 || !PioneerResidentCapacity(result.bytes)) return B_BAD_VALUE;
	}
	status=AcquireProbeLock(cookie); if (status!=B_OK) return status;
	if (operation==TT_PCI_PROBE_PIONEER_SESSION_SUPPORT) {
		result={}; result.version=1; result.capabilities=TT_PIONEER_RESIDENT_CAPABILITIES;
	} else if (operation==TT_PCI_PROBE_PIONEER_SESSION_BEGIN) {
		status=BeginResidentSession(cookie,result.bytes,result);
		if (status!=B_OK) { release_sem(sLock); return status; }
	} else if (cookie->context==NULL || sRuntimeOwner!=cookie) {
		release_sem(sLock); return B_DEV_NOT_READY;
	} else if (operation==TT_PCI_PROBE_PIONEER_SESSION_END) {
		EndResidentSession(cookie,result,true);
	} else {
		// CPU metadata only. It does not revalidate hardware health or
		// authorize freeing a page; END performs its own live audit.
		cookie->context->Snapshot(result);
	}
	status=user_memcpy(data,&result,sizeof(result));
	if (status!=B_OK && operation==TT_PCI_PROBE_PIONEER_SESSION_BEGIN && cookie->context!=NULL) {
		tt_resident_session_result cleanup={}; EndResidentSession(cookie,cleanup,true);
	}
	release_sem(sLock); return status;
}

static status_t
ResidentStoreControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	if (data==NULL || length!=sizeof(tt_pioneer_resident_page)) return B_BAD_VALUE;
	thread_info thread={};
	status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	tt_pioneer_resident_page* request=(tt_pioneer_resident_page*)malloc(sizeof(tt_pioneer_resident_page));
	if (request==NULL) return B_NO_MEMORY;
	status=user_memcpy(request,data,sizeof(*request));
	if (status!=B_OK || request->version!=1 || request->reserved!=0) {
		free(request); return status!=B_OK?status:B_BAD_VALUE;
	}
	status=AcquireProbeLock(cookie);
	if (status!=B_OK) { free(request); return status; }
	PioneerStoreContext* context=cookie->context;
	if (context==NULL || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if (context->runtimeCoreMode==4 || context->runtimeCoreMode==6 || context->runtimeCoreMode==7 || context->runtimeCoreMode==8 || context->runtimeCoreMode==9) status=B_NOT_ALLOWED;
	else if (!PioneerResidentOffset(context->bytes,request->offset)
		|| (context->residentAttention.reserved && request->offset>=context->bytes-TT_RESIDENT_KV_BYTES)) status=B_BAD_VALUE;
	else {
		bool upload=operation==TT_PCI_PROBE_PIONEER_STORE_UPLOAD;
		unsigned page=unsigned(request->offset/4096);
		if (!upload && !context->initialized[page]) status=B_ENTRY_NOT_FOUND;
		else {
			request->result={}; request->result.version=1; request->result.state=TT_STORE_CONFIG_FAILED;
			bool ready=context->lease.ready;
			if (!ready && !context->lease.saved) ready=PioneerResidentLeaseBegin(context->io,context->baseline.baseline.baseline.noc,context->lease);
			bool passed=ready && PioneerResidentStorePage(context->io,context->baseline.baseline.baseline.baseline.dram,
				context->baseline.baseline.baseline.noc,context->host,context->lease,context->bytes,request->offset,
				upload,request->data,request->result);
			if (upload && request->result.wrote_device && context->initialized[page]) {
				context->initialized[page]=0; context->uploadedPages--;
			}
			if (passed && upload) {
				context->hashes[page]=PioneerResidentPageHash(request->data);
				context->initialized[page]=1; context->uploadedPages++;
			} else if (passed && PioneerResidentPageHash(request->data)!=context->hashes[page]) {
				request->result.state=TT_STORE_MISMATCH; request->result.mismatches++;
			}
			if (context->lease.recovery_required || context->lease.pending || request->result.recovery_required) {
				context->lease.recovery_required=true; context->io.QuarantineHostPage();
				request->result.recovery_required=1; sColdRecoveryRequired=true;
			}
			status=user_memcpy(data,request,sizeof(*request));
		}
	}
	release_sem(sLock); free(request); return status;
}

#ifdef TT_PIONEER_EXPERIMENTAL_Q38_BANKS
// PIONEER_Q38_BANK_CONTROL_BEGIN
// Experimental45: storage-only, disjoint ABI, no code image/core release.
// An arena consumes the SAME global owner as legacy inference. There is no
// bank-local recovery/retry: uncertainty quarantines the entire card owner.
static status_t
Q38BankBulkUpload(PioneerProbeCookie* cookie,void* data,size_t length)
{
	if (length!=sizeof(tt_q38_bank_bulk)) return B_BAD_VALUE;
	auto* request=(tt_q38_bank_bulk*)malloc(sizeof(tt_q38_bank_bulk));
	if (!request) return B_NO_MEMORY;
	status_t status=user_memcpy(request,data,sizeof(*request));
	if (status!=B_OK || request->version!=TT_Q38_BANK_PROTOCOL || request->bank>=7 || request->reserved
		|| !PioneerQ38BankBulkRange(request->bank,request->offset,request->pages)) {
		free(request); return status!=B_OK?status:B_BAD_VALUE;
	}
	status=AcquireProbeLock(cookie,true); if (status!=B_OK) { free(request); return status; }
	auto* c=cookie->context;
	if (!c || sRuntimeOwner!=cookie || !c->bankStorageOnly) status=B_NOT_ALLOWED;
#ifdef PIONEER_Q38_SPAN_PRELOAD
	else if (c->q38Span && !c->q38Span->closed) status=B_NOT_ALLOWED;
#endif
	else {
		auto& dram=c->baseline.baseline.baseline.baseline.dram;
		auto& noc=c->baseline.baseline.baseline.noc;
		uint32 address=0,route=0,index=0;
		if (!c->banks.Range(dram,request->bank,request->offset,request->pages)
			|| !c->banks.Page(dram,request->bank,request->offset,address,route,index)) status=B_DEV_NOT_READY;
		else {
			request->result={}; request->result.transfer.version=1; request->result.transfer.state=TT_STORE_CONFIG_FAILED;
			bool ready=c->lease.ready;
			if (!ready && !c->lease.saved) ready=PioneerResidentLeaseBegin(c->io,noc,c->lease);
			if (ready) ready=c->io.SelectRuntimeCore(0) && c->io.MapBulk();
			if (ready && !c->bulkHost.allocated) ready=PioneerPrepareOutputBuffer(c->io,c->host,c->bulkHost);
			ready=ready && c->bulkHost.ready && PioneerBulkUploadPrepare(c->io,dram,noc,c->host,c->lease,c->bulkHost,c->bulk);
			bool passed=ready && PioneerBulkUpload(c->io,dram,noc,c->host,c->lease,c->bulkHost,c->bulk,
				PioneerResidentBanks::Capacity,request->offset,request->pages,request->data,request->result,&c->banks,request->bank);
			for (unsigned p=0;p<request->pages;p++) {
				if (request->result.transfer.wrote_device && c->initialized[index+p]) {
					c->initialized[index+p]=0; c->uploadedPages--;
				}
				if (passed) {
					c->hashes[index+p]=PioneerResidentPageHash(request->data+p*1024);
					c->initialized[index+p]=1; c->uploadedPages++;
				}
			}
			if (c->Unsafe() || request->result.transfer.recovery_required) {
				c->banks.Quarantine(); c->bulk.unsafe=true; c->lease.recovery_required=true;
				c->io.QuarantineHostPage(); PioneerQuarantineOutputBuffer(c->io,c->bulkHost);
				request->result.transfer.recovery_required=1; sColdRecoveryRequired=true;
			}
			status=user_memcpy(data,request,sizeof(*request));
		}
	}
	release_sem(sLock); free(request); return status;
}

static status_t
Q38BankControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	if (!data) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	if (operation==TT_PCI_PROBE_Q38_BANK_BULK_UPLOAD) return Q38BankBulkUpload(cookie,data,length);
	if (operation==TT_PCI_PROBE_Q38_BANK_SUPPORT) {
		if (length!=sizeof(tt_q38_bank_support)) return B_BAD_VALUE;
		status=AcquireProbeLock(cookie,true); if (status!=B_OK) return status;
		const tt_q38_bank_support support={TT_Q38_BANK_DRIVER,TT_Q38_BANK_PROTOCOL,7,4096,4,1,
			PioneerResidentBanks::Capacity};
		status=user_memcpy(data,&support,sizeof(support)); release_sem(sLock); return status;
	}
	bool page=operation==TT_PCI_PROBE_Q38_BANK_UPLOAD || operation==TT_PCI_PROBE_Q38_BANK_READ;
	if (!page) {
		if (length!=sizeof(tt_q38_bank_session)) return B_BAD_VALUE;
		tt_q38_bank_session request={}; status=user_memcpy(&request,data,sizeof(request));
		if (status!=B_OK || request.version!=TT_Q38_BANK_PROTOCOL || request.reserved)
			return status!=B_OK?status:B_BAD_VALUE;
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
		status=(operation==TT_PCI_PROBE_Q38_BANK_END || operation==TT_PCI_PROBE_Q38_BANK_STATUS)?AcquireQ38GdnSessionLock(cookie):AcquireProbeLock(cookie,true);
#elif defined(PIONEER_Q38_LOCAL_DELTA_PROBE)
		status=(operation==TT_PCI_PROBE_Q38_BANK_END || operation==TT_PCI_PROBE_Q38_BANK_STATUS)?AcquireQ38DeltaLock(cookie):AcquireProbeLock(cookie,true);
#else
		status=AcquireProbeLock(cookie,true); if (status!=B_OK) return status;
#endif
		if (status!=B_OK) return status;
		bool began=false;
		if (operation==TT_PCI_PROBE_Q38_BANK_BEGIN) {
			bool empty=cookie->context==NULL;
			status=BeginResidentSession(cookie,PioneerResidentBanks::Total,request.result,true);
			began=empty && status==B_OK && cookie->context!=NULL;
		} else if (!cookie->context || sRuntimeOwner!=cookie || !cookie->context->bankStorageOnly) {
			status=B_NOT_ALLOWED;
		} else if (operation==TT_PCI_PROBE_Q38_BANK_END) {
#ifdef PIONEER_Q38_HOST_PHASE_PROFILE
			cookie->context->io.q38HostPhaseReportAllowed=true; // CPU diagnostic permission ONLY,not cleanup authority
#endif
			EndResidentSession(cookie,request.result,true);
		} else if (operation==TT_PCI_PROBE_Q38_BANK_STATUS) {
			cookie->context->Snapshot(request.result);
		} else status=B_BAD_VALUE;
		if (status==B_OK) status=user_memcpy(data,&request,sizeof(request));
		if (status!=B_OK && began) {
			tt_resident_session_result cleanup={};
#ifdef PIONEER_Q38_NORMAL_CLOCK
			// A lost BEGIN reply must not remove a known busy request behind
			// the caller:it sees only transport uncertainty and closes ONLY.
			if (cookie->context && cookie->context->q38Clock.RequiresExplicitEnd())
				RetainQ38DispatchOwner(*cookie->context,cleanup);
			else
#endif
			EndResidentSession(cookie,cleanup,true);
		}
		release_sem(sLock); return status;
	}
	if (length!=sizeof(tt_q38_bank_page)) return B_BAD_VALUE;
	auto* request=(tt_q38_bank_page*)malloc(sizeof(tt_q38_bank_page));
	if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,sizeof(*request));
	if (status!=B_OK || request->version!=TT_Q38_BANK_PROTOCOL || request->bank>=7
		|| (request->offset&4095) || request->offset>=PioneerResidentBanks::Capacity) {
		free(request); return status!=B_OK?status:B_BAD_VALUE;
	}
	status=AcquireProbeLock(cookie,true); if (status!=B_OK) { free(request); return status; }
	auto* c=cookie->context;
	if (!c || sRuntimeOwner!=cookie || !c->bankStorageOnly) status=B_NOT_ALLOWED;
#ifdef PIONEER_Q38_SPAN_PRELOAD
	else if (c->q38Span && !c->q38Span->closed) status=B_NOT_ALLOWED;
#endif
	else {
		auto& dram=c->baseline.baseline.baseline.baseline.dram;
		auto& noc=c->baseline.baseline.baseline.noc;
		uint32 address=0,route=0,index=0;
		bool upload=operation==TT_PCI_PROBE_Q38_BANK_UPLOAD;
		if (!c->banks.Page(dram,request->bank,request->offset,address,route,index)) status=B_DEV_NOT_READY;
		else if (!upload && !c->initialized[index]) status=B_ENTRY_NOT_FOUND;
		else {
			request->result={}; request->result.version=1; request->result.state=TT_STORE_CONFIG_FAILED;
			bool ready=c->lease.ready;
			if (!ready && !c->lease.saved) ready=PioneerResidentLeaseBegin(c->io,noc,c->lease);
			bool passed=ready && PioneerResidentStorePage(c->io,dram,noc,c->host,c->lease,
				PioneerResidentBanks::Capacity,request->offset,upload,request->data,request->result,
				&c->banks,request->bank);
			if (upload && request->result.wrote_device && c->initialized[index]) {
				c->initialized[index]=0; c->uploadedPages--;
			}
			if (passed && upload) {
				c->hashes[index]=PioneerResidentPageHash(request->data);
				c->initialized[index]=1; c->uploadedPages++;
			} else if (passed && PioneerResidentPageHash(request->data)!=c->hashes[index]) {
				request->result.state=TT_STORE_MISMATCH; request->result.mismatches++;
				c->initialized[index]=0; c->uploadedPages--;
			}
			if (c->Unsafe()) {
				c->banks.Quarantine(); c->lease.recovery_required=true;
				c->io.QuarantineHostPage(); request->result.recovery_required=1; sColdRecoveryRequired=true;
			}
			status=user_memcpy(data,request,sizeof(*request));
		}
	}
	release_sem(sLock); free(request); return status;
}
// PIONEER_Q38_BANK_CONTROL_END
#endif

#ifdef PIONEER_Q38_SPAN_PRELOAD
// PIONEER_Q38_SPAN_CONTROL_BEGIN
static status_t
Q38SpanControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	if (!data) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	if (operation==TT_PCI_PROBE_Q38_SPAN_SUPPORT
#ifdef PIONEER_Q38_SPAN_FAST
		|| operation==TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT
#endif
		) {
		if (length!=sizeof(tt_q38_span_support)) return B_BAD_VALUE;
		status=AcquireProbeLock(cookie,true); if (status!=B_OK) return status;
		const tt_q38_span_support support={55,1,7,4096,TT_Q38_SPAN_PUBLIC_BYTES,1,1,0};
		status=user_memcpy(data,&support,sizeof(support)); release_sem(sLock); return status;
	}
	if (operation==TT_PCI_PROBE_Q38_SPAN_END) {
		if (length!=sizeof(tt_q38_span_end)) return B_BAD_VALUE;
		tt_q38_span_end request={}; status=user_memcpy(&request,data,sizeof(request));
		if (status!=B_OK || request.version!=1 || request.reserved) return status!=B_OK?status:B_BAD_VALUE;
		status=AcquireProbeLock(cookie,true); if (status!=B_OK) return status;
		auto* c=cookie->context;
		if (!c || sRuntimeOwner!=cookie || !PioneerQ38SpanOwner(*c) || !c->q38Span) status=B_NOT_ALLOWED;
		else {
			request.closed=unsigned(PioneerQ38SpanEndService(*c)); request.recovery_required=0;
			if (!request.closed) {
				c->q38Span->unsafe=true; tt_resident_session_result retained={}; RetainQ38DispatchOwner(*c,retained);
				request.recovery_required=1;
			}
			status=user_memcpy(data,&request,sizeof(request));
		}
		release_sem(sLock); return status;
	}
	if ((operation!=TT_PCI_PROBE_Q38_SPAN_UPLOAD
#ifdef PIONEER_Q38_SPAN_FAST
		&& operation!=TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD
#endif
		) || length!=sizeof(tt_q38_span_upload)) return B_BAD_VALUE;
	auto* request=(tt_q38_span_upload*)malloc(sizeof(tt_q38_span_upload)); if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,sizeof(*request));
	if (status!=B_OK || !PioneerQ38SpanPublicShape(request->version,request->bank,request->offset,request->bytes,request->reserved)) {
		free(request); return status!=B_OK?status:B_BAD_VALUE;
	}
	status=AcquireProbeLock(cookie,true); if (status!=B_OK) { free(request); return status; }
	auto* c=cookie->context;
	if (!c || sRuntimeOwner!=cookie || !PioneerQ38SpanOwner(*c) || (c->q38Span && c->q38Span->closed)) status=B_NOT_ALLOWED;
	else {
		PioneerQ38SpanExtent extent;
		if (!PioneerQ38SpanAuthorize(c->banks,c->baseline.baseline.baseline.baseline.dram,c->initialized,
			uint32(c->bytes/4096),request->bank,request->offset,request->bytes,extent)) status=B_ENTRY_NOT_FOUND;
		else {
			if (!c->q38Span) c->q38Span=new(std::nothrow) PioneerQ38SpanLease;
			if (!c->q38Span) status=B_NO_MEMORY;
			else {
				bool passed;
#ifdef PIONEER_Q38_SPAN_FAST
				if (operation==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD) passed=PioneerQ38SpanUploadService<PioneerStoreContext,true>(*c,*request);
				else
#endif
				passed=PioneerQ38SpanUploadService(*c,*request);
				if (!passed || c->Unsafe()) {
					c->q38Span->unsafe=true; tt_resident_session_result retained={}; RetainQ38DispatchOwner(*c,retained);
					request->result.state=TT_RUNTIME_COMPLETION_FAILED; request->result.pages=0;
					request->result.host_exact=0; request->result.recovery_required=1;
				}
				status=user_memcpy(data,request,sizeof(*request));
			}
		}
	}
	release_sem(sLock); free(request); return status;
}
// PIONEER_Q38_SPAN_CONTROL_END
#endif

#ifdef PIONEER_Q38_LOCAL_DELTA_PROBE
// PIONEER_Q38_DELTA_CONTROL_BEGIN
static status_t
Q38DeltaControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	if (!data) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	if (operation==TT_PCI_PROBE_Q38_DELTA_SUPPORT) {
		if (length!=sizeof(tt_q38_delta_support)) return B_BAD_VALUE;
		status=AcquireQ38DeltaLock(cookie); if (status!=B_OK) return status;
		const tt_q38_delta_support support={TT_Q38_DELTA_PROBE_VERSION,55,1,128,64,8192,1};
		status=user_memcpy(data,&support,sizeof(support)); release_sem(sLock); return status;
	}
	if (operation==TT_PCI_PROBE_Q38_DELTA_ACCEPT) {
		if (length!=sizeof(tt_q38_delta_accept)) return B_BAD_VALUE;
		tt_q38_delta_accept a={}; status=user_memcpy(&a,data,sizeof(a)); if (status!=B_OK) return status;
		status=AcquireQ38DeltaLock(cookie); if (status!=B_OK) return status;
		auto* c=cookie->context;
		status=c && sRuntimeOwner==cookie && c->bankStorageOnly && PioneerQ38DeltaProbeAccept(c->q38Delta,a)?B_OK:B_NOT_ALLOWED;
		release_sem(sLock); return status;
	}
	if (operation!=TT_PCI_PROBE_Q38_DELTA_RUN || length!=sizeof(tt_q38_delta_probe)) return B_BAD_VALUE;
	auto* job=(tt_q38_delta_probe*)malloc(sizeof(tt_q38_delta_probe)); if (!job) return B_NO_MEMORY;
	status=user_memcpy(job,data,sizeof(*job));
	if (status!=B_OK || !PioneerQ38DeltaProbeValid(*job)) { free(job); return status!=B_OK?status:B_BAD_VALUE; }
	status=AcquireQ38DeltaLock(cookie); if (status!=B_OK) { free(job); return status; }
	auto* c=cookie->context;
	if (!c || sRuntimeOwner!=cookie || !c->bankStorageOnly || c->q38Dispatch || c->q38Cohort
		|| c->panelProbe.saved || c->streamProbe.saved || c->q38Delta.awaiting || c->q38Delta.unsafe || c->q38Delta.running
		|| c->q38Delta.nonce==UINT32_MAX) status=B_NOT_ALLOWED;
#ifdef PIONEER_Q38_SPAN_PRELOAD
	else if (c->q38Span) status=B_NOT_ALLOWED;
#endif
	else {
		sQ38LocalDeltaOwner=cookie; // exclusive intent BEFORE any prepare/mapping/reset
		auto& dram=c->baseline.baseline.baseline.baseline.dram; auto& noc=c->baseline.baseline.baseline.noc;
		job->result={}; job->result.version=TT_Q38_DELTA_PROBE_VERSION; job->result.state=TT_RUNTIME_CONFIG_FAILED;
		for (auto& v:job->next) v=0;
		for (auto& v:job->output) v=0;
		bool ready=c->lease.ready;
		if (!ready && !c->lease.saved) ready=PioneerResidentLeaseBegin(c->io,noc,c->lease);
		ready=ready && c->io.SelectRuntimeCore(0) && c->io.MapQ38Panel()
			&& PioneerQ38DeltaProbePrepare(c->io,dram,noc,c->lease,c->banks,c->q38Delta);
		bool passed=ready && PioneerQ38DeltaProbeRun(c->io,dram,noc,c->lease,c->banks,c->q38Delta,*job);
		if (!passed || c->Unsafe()) job->result.recovery_required=1;
		status=user_memcpy(data,job,sizeof(*job));
		if (!passed || status!=B_OK || c->Unsafe()) {
			// MEMORY ONLY. On UNKNOWN or lost reply,retain ALL owners.
			c->q38Delta.unsafe=true; c->banks.Quarantine(); c->lease.recovery_required=true;
			c->io.QuarantineHostPage(); sColdRecoveryRequired=true;
		}
	}
	release_sem(sLock); free(job); return status;
}
// PIONEER_Q38_DELTA_CONTROL_END
#endif

#ifdef TT_PIONEER_EXPERIMENTAL_Q38_PANEL
// PIONEER_Q38_PANEL_CONTROL_BEGIN
static status_t
Q38PanelControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	if (!data) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	if (operation==TT_PCI_PROBE_Q38_PANEL_SUPPORT) {
		if (length!=sizeof(tt_q38_panel_support)) return B_BAD_VALUE;
		status=AcquireProbeLock(cookie,true); if (status!=B_OK) return status;
		const tt_q38_panel_support support={
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT
			TT_Q38_COHORT_DRIVER,
#else
			TT_Q38_PANEL_CURRENT_DRIVER,
#endif
			TT_Q38_PANEL_ABI_VERSION,1,16,TT_Q38_PANEL_COLUMNS,0x5100};
		status=user_memcpy(data,&support,sizeof(support)); release_sem(sLock); return status;
	}
	if (operation!=TT_PCI_PROBE_Q38_PANEL_RUN || length!=sizeof(tt_q38_panel_probe)) return B_BAD_VALUE;
	auto* request=(tt_q38_panel_probe*)malloc(sizeof(tt_q38_panel_probe));
	if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,sizeof(*request));
	if (status!=B_OK || !PioneerQ38PanelProbeValid(*request)) { free(request); return status!=B_OK?status:B_BAD_VALUE; }
	status=AcquireProbeLock(cookie,true); if (status!=B_OK) { free(request); return status; }
	auto* c=cookie->context;
	if (!c || sRuntimeOwner!=cookie || !c->bankStorageOnly) status=B_NOT_ALLOWED;
#ifdef PIONEER_Q38_SPAN_PRELOAD
	else if (c->q38Span && !c->q38Span->closed) status=B_NOT_ALLOWED;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT
	else if (c->q38Cohort) status=B_NOT_ALLOWED; // never overlap local and cohort L1 owners
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_STREAM
	else if (c->streamProbe.saved) status=B_NOT_ALLOWED; // overlapping L1 leases NEVER coexist
#endif
	else {
		auto& dram=c->baseline.baseline.baseline.baseline.dram;
		auto& noc=c->baseline.baseline.baseline.noc;
		request->result={}; request->result.version=1; request->result.state=TT_RUNTIME_CONFIG_FAILED;
		for (unsigned row=0;row<16;row++) request->dot[row]=request->coefficient[row]=request->minimum[row]=0;
		bool ready=c->lease.ready;
		if (!ready && !c->lease.saved) ready=PioneerResidentLeaseBegin(c->io,noc,c->lease);
		ready=ready && c->io.SelectRuntimeCore(0) && c->io.MapQ38Panel()
			&& PioneerQ38PanelPrepare(c->io,dram,noc,c->lease,c->banks,c->panelProbe);
		if (ready) PioneerQ38PanelRun(c->io,dram,noc,c->lease,c->banks,c->panelProbe,*request);
		if (c->Unsafe() || request->result.recovery_required) {
			c->banks.Quarantine(); c->panelProbe.unsafe=true; c->lease.recovery_required=true;
			c->io.QuarantineHostPage(); request->result.recovery_required=1; sColdRecoveryRequired=true;
		}
		status=user_memcpy(data,request,sizeof(*request));
	}
	release_sem(sLock); free(request); return status;
}
// PIONEER_Q38_PANEL_CONTROL_END
#endif

#ifdef TT_PIONEER_EXPERIMENTAL_Q38_STREAM
// PIONEER_Q38_STREAM_CONTROL_BEGIN
static status_t
Q38StreamControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	if (!data) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	if (operation==TT_PCI_PROBE_Q38_STREAM_SUPPORT) {
		if (length!=sizeof(tt_q38_stream_support)) return B_BAD_VALUE;
		status=AcquireProbeLock(cookie,true); if (status!=B_OK) return status;
		const tt_q38_stream_support support={
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT
			TT_Q38_COHORT_DRIVER,
#else
			TT_Q38_STREAM_DRIVER,
#endif
			TT_Q38_STREAM_ABI,TT_Q38_STREAM_OUTPUT_ROWS,17408,7,0};
		status=user_memcpy(data,&support,sizeof(support)); release_sem(sLock); return status;
	}
	if (operation!=TT_PCI_PROBE_Q38_STREAM_RUN || length!=sizeof(tt_q38_stream_probe)) return B_BAD_VALUE;
	auto* request=(tt_q38_stream_probe*)malloc(sizeof(tt_q38_stream_probe));
	if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,sizeof(*request));
	if (status!=B_OK || !PioneerQ38StreamValid(*request)) { free(request); return status!=B_OK?status:B_BAD_VALUE; }
	status=AcquireProbeLock(cookie,true); if (status!=B_OK) { free(request); return status; }
	auto* c=cookie->context;
	if (!c || sRuntimeOwner!=cookie || !c->bankStorageOnly || c->panelProbe.saved) status=B_NOT_ALLOWED;
#ifdef PIONEER_Q38_SPAN_PRELOAD
	else if (c->q38Span && !c->q38Span->closed) status=B_NOT_ALLOWED;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT
	else if (c->q38Cohort) status=B_NOT_ALLOWED;
#endif
	else {
		auto& dram=c->baseline.baseline.baseline.baseline.dram; auto& noc=c->baseline.baseline.baseline.noc;
		PioneerQ38StreamExtent extent;
		if (c->bytes!=PioneerResidentBanks::Total || !PioneerQ38StreamAuthorize(c->banks,dram,c->initialized,
			uint32(c->bytes/4096),*request,extent)) status=B_ENTRY_NOT_FOUND;
		else {
			request->result={}; request->result.version=1; request->result.state=TT_RUNTIME_CONFIG_FAILED;
			for (unsigned row=0;row<TT_Q38_STREAM_OUTPUT_ROWS;row++) request->output[row]=0;
			bool ready=c->lease.ready;
			if (!ready && !c->lease.saved) ready=PioneerResidentLeaseBegin(c->io,noc,c->lease);
			ready=ready && c->io.SelectRuntimeCore(0) && c->io.MapQ38Panel()
				&& PioneerQ38StreamPrepare(c->io,dram,noc,c->lease,c->banks,c->streamProbe);
			if (ready) PioneerQ38StreamRun(c->io,dram,noc,c->lease,c->banks,c->streamProbe,c->initialized,uint32(c->bytes/4096),*request);
			if (c->Unsafe() || request->result.recovery_required) {
				c->banks.Quarantine(); c->streamProbe.unsafe=true; c->lease.recovery_required=true;
				c->io.QuarantineHostPage(); request->result.recovery_required=1; sColdRecoveryRequired=true;
			}
			status=user_memcpy(data,request,sizeof(*request));
		}
	}
	release_sem(sLock); free(request); return status;
}
// PIONEER_Q38_STREAM_CONTROL_END
#endif

#if defined(TT_PIONEER_EXPERIMENTAL_Q38_COHORT) && !defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH)
// PIONEER_Q38_COHORT_PREPARE_BEGIN
static bool
PrepareQ38Cohort(PioneerStoreContext& c,unsigned workers)
{
	auto& dram=c.baseline.baseline.baseline.baseline.dram; auto& noc=c.baseline.baseline.baseline.noc;
	if (!workers || workers>120 || !c.io.SelectRuntimeCore(0)) return false;
	bool ready=c.lease.ready;
	if (!ready && !c.lease.saved) ready=PioneerResidentLeaseBegin(c.io,noc,c.lease);
	if (!ready || !c.io.MapRuntime()) return false;
	if (!c.q38Cohort) {
		c.q38Cohort=new(std::nothrow) PioneerQ38CohortLease;
		if (!c.q38Cohort) return false;
		c.io.fleetEnabled=true; c.q38Cohort->worker[0].master=&c.lease; c.q38Cohort->worker[0].noc=noc;
	}
	if (!c.q38CohortRoutes) {
		ready=BlackholeDramAllCoresHeld(c.io,dram);
		for (unsigned w=1;w<120 && ready;w++) ready=PioneerWorkerNocProbe(c.io,dram,w,c.q38Cohort->worker[w].noc);
		c.q38CohortRoutes=ready;
	}
	for (unsigned w=0;w<workers && ready;w++) {
		auto& node=c.q38Cohort->worker[w];
		ready=c.io.SelectRuntimeCore(w) && c.io.MapQ38Panel();
		if (!ready) break;
		if (c.q38Cohort->prepared<w+1) c.q38Cohort->prepared=w+1;
		if (w && !node.Store().ready) ready=!node.Store().saved && PioneerQ38CohortPeerLeaseBegin(c.io,dram,w,node);
		if (ready) ready=PioneerQ38CohortPrepareWorker(c.io,dram,c.banks,node,w);
	}
	return ready;
}
// PIONEER_Q38_COHORT_PREPARE_END
// PIONEER_Q38_COHORT_CONTROL_BEGIN
static status_t
Q38CohortControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	if (!data) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	if (operation==TT_PCI_PROBE_Q38_COHORT_SUPPORT) {
		if (length!=sizeof(tt_q38_cohort_support)) return B_BAD_VALUE;
		status=AcquireProbeLock(cookie,true); if (status!=B_OK) return status;
		const tt_q38_cohort_support support={TT_Q38_COHORT_DRIVER,TT_Q38_COHORT_ABI,120,256,17408,7,64,0};
		status=user_memcpy(data,&support,sizeof(support)); release_sem(sLock); return status;
	}
	if (operation!=TT_PCI_PROBE_Q38_COHORT_RUN || length!=sizeof(tt_q38_cohort_probe)) return B_BAD_VALUE;
	auto* request=(tt_q38_cohort_probe*)malloc(sizeof(tt_q38_cohort_probe)); if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,sizeof(*request));
	if (status!=B_OK || !PioneerQ38CohortValid(*request)) { free(request); return status!=B_OK?status:B_BAD_VALUE; }
	status=AcquireProbeLock(cookie,true); if (status!=B_OK) { free(request); return status; }
	auto* c=cookie->context;
	if (!c || sRuntimeOwner!=cookie || !c->bankStorageOnly || c->panelProbe.saved || c->streamProbe.saved) status=B_NOT_ALLOWED;
#ifdef PIONEER_Q38_SPAN_PRELOAD
	else if (c->q38Span && !c->q38Span->closed) status=B_NOT_ALLOWED;
#endif
	else {
		auto& dram=c->baseline.baseline.baseline.baseline.dram;
		if (c->bytes!=PioneerResidentBanks::Total || !PioneerQ38CohortAuthorize(c->banks,dram,c->initialized,
			uint32(c->bytes/4096),*request,nullptr)) status=B_ENTRY_NOT_FOUND;
		else {
			request->result={}; request->result.version=1; request->result.state=TT_RUNTIME_CONFIG_FAILED;
			for (auto& row:request->output) for (auto& value:row) value=0;
			if (PrepareQ38Cohort(*c,request->workers)) PioneerQ38CohortRun(c->io,dram,c->banks,*c->q38Cohort,
				c->initialized,uint32(c->bytes/4096),*request);
			bool unsafe=c->Unsafe() || request->result.recovery_required;
			if (!unsafe && !c->io.SelectRuntimeCore(0)) unsafe=true;
			if (unsafe) {
				if (c->q38Cohort) PioneerQ38CohortQuarantine(*c->q38Cohort,c->banks);
				c->banks.Quarantine(); c->lease.recovery_required=true; c->io.QuarantineHostPage();
				request->result.recovery_required=1; sColdRecoveryRequired=true;
				for (auto& row:request->output) for (auto& value:row) value=0;
			}
			status=user_memcpy(data,request,sizeof(*request));
		}
	}
	release_sem(sLock); free(request); return status;
}
// PIONEER_Q38_COHORT_CONTROL_END
#endif

#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
// PIONEER_Q38_DISPATCH_PREPARE_BEGIN
static bool
PrepareQ38Dispatch(PioneerStoreContext& c)
{
#ifdef PIONEER_Q38_SPAN_PRELOAD
	if (c.q38Span && !c.q38Span->closed) return false;
	c.io.q38SpanAllowed=false; // one-way ownership handoff, even if prepare fails
#endif
	auto& dram=c.baseline.baseline.baseline.baseline.dram; auto& noc=c.baseline.baseline.baseline.noc;
	if (c.q38Dispatch && c.q38Dispatch->booted) return !PioneerQ38DispatchUnsafe(*c.q38Dispatch) && !c.q38Dispatch->closed;
	if (c.q38Dispatch) return false; // no second prepare/retry after any attempt
	if (!c.io.SelectRuntimeCore(0)) return false;
	bool ready=c.lease.ready;
	if (!ready && !c.lease.saved) ready=PioneerResidentLeaseBegin(c.io,noc,c.lease);
	if (!ready || !c.io.MapRuntime()) return false;
	c.q38Dispatch=new(std::nothrow) PioneerQ38DispatchLease;
	if (!c.q38Dispatch) return false;
#ifdef PIONEER_Q38_WORK_OUTPUT_DMA
	if (!c.wideHost.ready && (c.wideHost.allocated || !PioneerPrepareWideOutput(c.io,c.host,c.bulkHost,c.wideHost))) return false;
	if (!PioneerQ38OutputDmaOwner(c.io,c.host,c.bulkHost,c.wideHost,false)) return false;
#endif
	auto& lease=*c.q38Dispatch; c.io.fleetEnabled=true; lease.worker[0].master=&c.lease; lease.worker[0].noc=noc;
	ready=BlackholeDramAllCoresHeld(c.io,dram);
	for (unsigned w=1;w<120 && ready;w++) ready=PioneerWorkerNocProbe(c.io,dram,w,lease.worker[w].noc);
	c.q38DispatchRoutes=ready;
	for (unsigned w=0;w<120 && ready;w++) {
		auto& node=lease.worker[w];
		ready=c.io.SelectRuntimeCore(w) && c.io.MapQ38Panel();
		if (ready && w) ready=PioneerQ38CohortPeerLeaseBegin(c.io,dram,w,node);
		if (ready) ready=PioneerQ38DispatchPrepareRole(c.io,dram,c.banks,lease,w);
		if (!ready) lease.prepared=w+1; // include any just-touched peer NIU
	}
	if (!ready) return PioneerQ38DispatchQuarantine(lease,c.banks);
	return PioneerQ38DispatchBootstrap(c.io,dram,c.banks,lease);
}
// PIONEER_Q38_DISPATCH_PREPARE_END
// PIONEER_Q38_DISPATCH_CONTROL_BEGIN
static status_t
Q38DispatchControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	if (!data) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	// PIONEER_Q38_WORK_SUPPORT_BEGIN
#ifdef TT_PIONEER_Q38_WORK_MODE
	if (operation==TT_PCI_PROBE_Q38_WORK_SUPPORT) {
		if (length!=sizeof(tt_q38_dispatch_support)) return B_BAD_VALUE;
		status=AcquireProbeLock(cookie,true); if (status!=B_OK) return status;
		const tt_q38_dispatch_support support={TT_Q38_DISPATCH_DRIVER,TT_Q38_WORK_SUPPORT_VERSION,119,256,17408,7,64,1,0};
		status=user_memcpy(data,&support,sizeof(support)); release_sem(sLock); return status;
	}
#endif
	// PIONEER_Q38_WORK_SUPPORT_END
	if (operation==TT_PCI_PROBE_Q38_DISPATCH_SUPPORT) {
		if (length!=sizeof(tt_q38_dispatch_support)) return B_BAD_VALUE;
		status=AcquireProbeLock(cookie,true); if (status!=B_OK) return status;
		const tt_q38_dispatch_support support={TT_Q38_DISPATCH_DRIVER,1,119,256,17408,7,64,1,0};
		status=user_memcpy(data,&support,sizeof(support)); release_sem(sLock); return status;
	}
	if ((operation!=TT_PCI_PROBE_Q38_DISPATCH_RUN
#ifdef TT_PIONEER_Q38_WORK_MODE
		&& operation!=TT_PCI_PROBE_Q38_WORK_RUN
#endif
		) || length!=sizeof(tt_q38_dispatch_probe)) return B_BAD_VALUE;
	auto* request=(tt_q38_dispatch_probe*)malloc(sizeof(tt_q38_dispatch_probe)); if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,sizeof(*request));
	if (status!=B_OK || !PioneerQ38DispatchValid(*request)) { free(request); return status!=B_OK?status:B_BAD_VALUE; }
	status=AcquireProbeLock(cookie,true
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
		,true
#endif
	); if (status!=B_OK) { free(request); return status; }
	auto* c=cookie->context;
	if (!c || sRuntimeOwner!=cookie || !c->bankStorageOnly || c->bytes!=PioneerResidentBanks::Total
		|| c->q38Cohort || c->panelProbe.saved || c->streamProbe.saved) status=B_NOT_ALLOWED;
#ifdef PIONEER_Q38_SPAN_PRELOAD
	else if (c->q38Span && !c->q38Span->closed) status=B_NOT_ALLOWED;
#endif
	else {
		auto& dram=c->baseline.baseline.baseline.baseline.dram; bool authorized=true;
		// Complete ALL logical initialized extents BEFORE prepare/boot/IO.
		for (unsigned w=0;w<request->peers;w++) {
			PioneerQ38CohortExtent extent;
			authorized &= PioneerQ38CohortAuthorizePart(c->banks,dram,c->initialized,uint32(c->bytes/4096),
				request->type,request->inputs,request->part[w],w+1,extent);
		}
		if (!authorized) status=B_ENTRY_NOT_FOUND;
		else {
			request->result={}; request->result.version=1; request->result.state=TT_RUNTIME_CONFIG_FAILED;
			for (auto& row:request->output) for (auto& value:row) value=0;
			bool prepared=PrepareQ38Dispatch(*c);
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
			if (prepared) c->q38Dispatch->inputEpoch=sQ38InputEpoch;
#endif
			if (prepared) {
#ifdef TT_PIONEER_Q38_WORK_MODE
				if (operation==TT_PCI_PROBE_Q38_WORK_RUN) PioneerQ38DispatchRun<decltype(c->io),true>(c->io,dram,c->banks,*c->q38Dispatch,c->initialized,uint32(c->bytes/4096),*request
#ifdef PIONEER_Q38_WORK_OUTPUT_DMA
					,&c->host,&c->bulkHost,&c->wideHost
#endif
					);
				else
#endif
				PioneerQ38DispatchRun(c->io,dram,c->banks,*c->q38Dispatch,c->initialized,uint32(c->bytes/4096),*request);
			}
			bool unsafe=!prepared || c->Unsafe() || request->result.recovery_required;
			if (!unsafe && !c->io.SelectRuntimeCore(0)) unsafe=true;
			if (unsafe) {
				if (c->q38Dispatch) PioneerQ38DispatchQuarantine(*c->q38Dispatch,c->banks);
				c->banks.Quarantine(); c->lease.recovery_required=true; c->io.QuarantineHostPage(); sColdRecoveryRequired=true;
				request->result.state=TT_RUNTIME_COMPLETION_FAILED; request->result.recovery_required=1;
				for (auto& row:request->output) for (auto& value:row) value=0;
			}
			status=user_memcpy(data,request,sizeof(*request));
		}
	}
	release_sem(sLock); free(request); return status;
}
// PIONEER_Q38_DISPATCH_CONTROL_END
#endif

static status_t
ResidentStreamControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	if (data==NULL || length!=sizeof(tt_pioneer_resident_stream)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	tt_pioneer_resident_stream* request=(tt_pioneer_resident_stream*)malloc(sizeof(tt_pioneer_resident_stream));
	if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,sizeof(*request));
	if (status!=B_OK || request->version!=1 || request->reserved) { free(request); return status!=B_OK?status:B_BAD_VALUE; }
	status=AcquireProbeLock(cookie); if (status!=B_OK) { free(request); return status; }
	PioneerStoreContext* context=cookie->context;
	if (!context || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if (context->runtimeCoreMode==4 || context->runtimeCoreMode==6 || context->runtimeCoreMode==7 || context->runtimeCoreMode==8 || context->runtimeCoreMode==9) status=B_NOT_ALLOWED;
	else if (!PioneerResidentOffset(context->bytes,request->offset)
		|| (context->residentAttention.reserved && request->offset>=context->bytes-TT_RESIDENT_KV_BYTES)) status=B_BAD_VALUE;
	else {
		request->result={}; request->result.version=1; request->result.state=TT_STORE_CONFIG_FAILED;
		bool ready=context->lease.ready;
		if (!ready && !context->lease.saved) ready=PioneerResidentLeaseBegin(context->io,context->baseline.baseline.baseline.noc,context->lease);
		bool passed=ready && PioneerResidentStreamPage(context->io,context->baseline.baseline.baseline.baseline.dram,
			context->baseline.baseline.baseline.noc,context->host,context->lease,context->bytes,request->offset,request->data,request->result);
		unsigned page=unsigned(request->offset/4096);
		if (request->result.wrote_device && context->initialized[page]) { context->initialized[page]=0; context->uploadedPages--; }
		if (passed) { context->hashes[page]=PioneerResidentPageHash(request->data); context->initialized[page]=1; context->uploadedPages++; }
		if (context->lease.recovery_required || context->lease.pending || request->result.recovery_required) {
			context->lease.recovery_required=true; context->io.QuarantineHostPage(); request->result.recovery_required=1; sColdRecoveryRequired=true;
		}
		status=user_memcpy(data,request,sizeof(*request));
	}
	release_sem(sLock); free(request); return status;
}

static status_t
BulkUploadControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	if (!data || length!=sizeof(tt_pioneer_bulk_upload)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	auto* request=(tt_pioneer_bulk_upload*)malloc(sizeof(tt_pioneer_bulk_upload));
	if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,sizeof(*request));
	if (status!=B_OK || request->version!=1 || request->reserved[0] || request->reserved[1]
		|| request->pages<1 || request->pages>4) { free(request); return status!=B_OK?status:B_BAD_VALUE; }
	status=AcquireProbeLock(cookie); if (status!=B_OK) { free(request); return status; }
	PioneerStoreContext* context=cookie->context;
	if (!context || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if (context->runtimeCoreMode!=-1 && context->runtimeCoreMode!=3 && context->runtimeCoreMode!=5 && context->runtimeCoreMode!=8) status=B_NOT_ALLOWED;
	else if (!PioneerBulkUploadRange(context->bytes,request->offset,request->pages)
		|| (context->residentAttention.reserved && request->offset+request->pages*4096>context->bytes-TT_RESIDENT_KV_BYTES)) status=B_BAD_VALUE;
	else {
		request->result={}; request->result.transfer.version=1; request->result.transfer.state=TT_STORE_CONFIG_FAILED;
		bool ready=context->lease.ready;
		if (!ready && !context->lease.saved) ready=PioneerResidentLeaseBegin(context->io,context->baseline.baseline.baseline.noc,context->lease);
		if (ready && context->runtimeCoreMode==3) ready=PioneerRuntimeAttentionPeersHeld(context->io,
			context->baseline.baseline.baseline.baseline.dram,context->baseline.baseline.baseline.noc,context->secondNoc,
			context->lease,context->secondLease,context->compute,context->secondCompute,context->attention);
		if (ready && context->runtimeCoreMode==5) ready=context->residentAttention.reserved
			?PioneerFleetMasterBoundary(context->io,context->baseline.baseline.baseline.baseline.dram,context->fleet,context->attention.ready)
			:FleetHeld(*context);
		if (ready && context->runtimeCoreMode==8) {
			ready=PioneerCoordinatedMasterBoundary(context->io,context->baseline.baseline.baseline.baseline.dram,context->fleet,context->attention.ready);
			for (unsigned p=0;p<request->pages;p++) ready &= !context->initialized[request->offset/4096+p];
			ready &= request->offset+request->pages*4096<=context->bytes-TT_RESIDENT_KV_BYTES;
		}
		if (ready) ready=context->io.SelectRuntimeCore(0) && context->io.MapBulk();
		if (ready && !context->bulkHost.allocated) ready=PioneerPrepareOutputBuffer(context->io,context->host,context->bulkHost);
		ready=ready && context->bulkHost.ready && PioneerBulkUploadPrepare(context->io,
			context->baseline.baseline.baseline.baseline.dram,context->baseline.baseline.baseline.noc,
			context->host,context->lease,context->bulkHost,context->bulk);
		bool passed=ready && PioneerBulkUpload(context->io,context->baseline.baseline.baseline.baseline.dram,
			context->baseline.baseline.baseline.noc,context->host,context->lease,context->bulkHost,context->bulk,
			context->bytes,request->offset,request->pages,request->data,request->result);
		unsigned first=unsigned(request->offset/4096);
		for (unsigned p=0;p<request->pages;p++) {
			if (request->result.transfer.wrote_device && context->initialized[first+p]) {
				context->initialized[first+p]=0; context->uploadedPages--;
			}
			if (passed) {
				context->hashes[first+p]=PioneerResidentPageHash(request->data+p*1024);
				context->initialized[first+p]=1; context->uploadedPages++;
			}
		}
		if (context->Unsafe() || request->result.transfer.recovery_required) {
			context->bulk.unsafe=true; context->lease.recovery_required=true;
			context->io.QuarantineHostPage(); PioneerQuarantineOutputBuffer(context->io,context->bulkHost);
			request->result.transfer.recovery_required=1; sColdRecoveryRequired=true;
		}
		status=user_memcpy(data,request,sizeof(*request));
	}
	release_sem(sLock); free(request); return status;
}

static status_t
RuntimeMatvecControl(PioneerProbeCookie* cookie,void* data,size_t length,bool dmaOutput,bool fp16,bool int8,bool q8=false,bool compact=false,bool native=false,bool f32Scales=false,bool bulk=false,bool second=false,bool dual=false,bool reuse=false)
{
	size_t bytes=dual?sizeof(tt_pioneer_runtime_dual_matvec):q8?sizeof(tt_pioneer_runtime_q8_matvec):sizeof(tt_pioneer_runtime_matvec);
	if (data==NULL || length!=bytes) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	tt_pioneer_runtime_matvec* request=(tt_pioneer_runtime_matvec*)malloc(bytes);
	if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,bytes);
	const uint32_t* scales=q8?((tt_pioneer_runtime_q8_matvec*)request)->scale:NULL;
	if (status!=B_OK || request->version!=1 || request->reserved || !(dual?PioneerRuntimeDualShape(request->inputs,request->outputs):compact?PioneerQ8CompactShape(request->inputs,request->outputs):PioneerMatvecShape(request->inputs,request->outputs))) {
		free(request); return status!=B_OK?status:B_BAD_VALUE;
	}
	status=AcquireProbeLock(cookie); if (status!=B_OK) { free(request); return status; }
	PioneerStoreContext* context=cookie->context;
	if (!context || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if (!(compact?PioneerQ8CompactWeightRange(context->bytes,request->weight_offset,request->inputs,request->outputs):PioneerMatvecWeightRange(context->bytes,request->weight_offset,request->inputs,request->outputs))) status=B_BAD_VALUE;
	else {
		uint64 weightBytes=compact?PioneerQ8CompactWeightBytes(request->inputs,request->outputs):PioneerMatvecWeightBytes(request->inputs,request->outputs);
		bool initialized=true;
		for (uint64 page=request->weight_offset/4096;page<(request->weight_offset+weightBytes+4095)/4096;page++)
			initialized &= context->initialized[page]!=0;
		if (!initialized) status=B_ENTRY_NOT_FOUND;
		else {
			request->result={}; request->result.version=1; request->result.state=TT_RUNTIME_CONFIG_FAILED;
			if (dual) memset(((tt_pioneer_runtime_dual_matvec*)request)->core,0,sizeof(((tt_pioneer_runtime_dual_matvec*)request)->core));
			memset(request->output,0,sizeof(request->output));
			context->io.runtimeProfile.Begin(uint64(system_time()),reuse?11:dual?10:second?9:bulk?8:f32Scales?7:native?6:compact?5:q8?4:int8?3:fp16?2:dmaOutput?1:0);
			int coreMode=reuse?3:dual?2:int(second);
			bool ready=context->runtimeCoreMode<0 || context->runtimeCoreMode==coreMode;
			ready=ready && (context->lease.ready || (!context->lease.saved
				&& PioneerResidentLeaseBegin(context->io,context->baseline.baseline.baseline.noc,context->lease)));
			if (ready) ready=context->io.MapRuntime(bulk);
			if (ready && second && !context->secondNoc.version)
				ready=BlackholeComputeNocProbe(context->io,context->baseline.baseline.baseline.baseline.dram,context->secondNoc,3);
			if (ready && second) ready=context->secondNoc.state==TT_COMPUTE_NOC_PASSED
				&& !context->secondNoc.recovery_required && context->secondNoc.tlb_restored
				&& context->io.AllocateSecondRuntime() && context->io.SelectRuntimeCore(1);
			auto& compute=second?context->secondCompute:context->compute;
			auto& lease=second?context->secondLease:context->lease;
			auto& noc=second?context->secondNoc:context->baseline.baseline.baseline.noc;
			if (ready && second && !lease.ready) ready=!lease.saved && PioneerResidentLeaseBegin(context->io,noc,lease,3);
			if (ready) context->runtimeCoreMode=coreMode;
			if (ready) ready=PioneerRuntimeMatvecPrepare(context->io,context->baseline.baseline.baseline.baseline.dram,
				noc,lease,compute,fp16,int8,q8,compact,native,f32Scales,bulk);
			if (ready && dual) ready=context->io.SelectRuntimeCore(0)
				&& PioneerRuntimeMatvecPrepare(context->io,context->baseline.baseline.baseline.baseline.dram,
					context->baseline.baseline.baseline.noc,context->lease,context->compute,false,true,true,true,true,true,true);
			if (ready && context->attention.saved) ready=reuse
				&& PioneerRuntimeAttentionSelect(context->io,context->baseline.baseline.baseline.baseline.dram,
					context->baseline.baseline.baseline.noc,context->secondNoc,context->lease,context->secondLease,
					context->compute,context->secondCompute,context->attention,false);
			if (ready) context->io.RuntimeProfileMark(TT_PROFILE_PREPARED);
			if (ready && dual) {
				auto& parts=((tt_pioneer_runtime_dual_matvec*)request)->core;
				PioneerRuntimeDualMatvec(context->io,context->baseline.baseline.baseline.baseline.dram,
					context->baseline.baseline.baseline.noc,context->secondNoc,context->host,context->lease,context->secondLease,
					context->compute,context->secondCompute,context->bytes,request->weight_offset,request->inputs,request->outputs,
					request->input,scales,request->output,parts,reuse?context->activation:NULL);
				auto& total=request->result; total=parts[0];
				total.state=parts[0].state==TT_RUNTIME_PASSED?parts[1].state:parts[0].state;
				total.prepared=parts[0].prepared && parts[1].prepared;
				total.started=parts[0].started && parts[1].started; total.completed=parts[0].completed && parts[1].completed;
				total.stopped=parts[0].stopped && parts[1].stopped; total.recovery_required=parts[0].recovery_required || parts[1].recovery_required;
				total.staged_pages+=parts[1].staged_pages; total.tiles+=parts[1].tiles; total.polls+=parts[1].polls; total.nonfinite+=parts[1].nonfinite;
				for (unsigned w=0;w<4;w++) total.after[w]+=parts[1].after[w]-parts[1].before[w];
			} else if (ready) PioneerRuntimeMatvec(context->io,context->baseline.baseline.baseline.baseline.dram,
				noc,context->host,lease,compute,context->bytes,
				request->weight_offset,request->inputs,request->outputs,request->input,request->output,request->result,dmaOutput,scales,second?3:2);
			if (second && !context->io.SelectRuntimeCore(0)) context->secondLease.recovery_required=true;
			if (context->Unsafe()) {
				context->lease.recovery_required=true; context->io.QuarantineHostPage();
				request->result.recovery_required=1; sColdRecoveryRequired=true;
			}
			const auto& r=request->result;
			context->io.runtimeProfile.End(uint64(system_time()),r.state,r.recovery_required,r.started,r.completed,r.stopped);
			status=user_memcpy(data,request,dual?bytes:sizeof(*request));
		}
	}
	release_sem(sLock); free(request); return status;
}

static status_t
CoordinatedSupportControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	if (!data || length!=sizeof(tt_coordinated_support)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	status=AcquireProbeLock(cookie); if (status!=B_OK) return status;
	// CPU metadata only. No map/read/write/probe/start/reset/DMA operation.
	tt_coordinated_support support={43,119,3,1,8192}; // v43 exact unpack source Wait Gate;SAME math bytes except ONE scheduling word/owners/proofs
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_BANKS
	support.version=TT_Q38_BANK_DRIVER; // separate experimental build; NEVER masquerade as qualified43
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_PANEL
	support.version=46; // local panel arithmetic probe,NOT full model support
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_AFFINE
	support.version=47; // separate scaled-local-panel probe,NOT full model support
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_BLOCK
	support.version=48; // complete LOCAL quantization block,NOT fullK/model support
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_STREAM
	support.version=TT_Q38_STREAM_DRIVER; // guarded fullK primitive,NOT model/Assistant support
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT
	support.version=TT_Q38_COHORT_DRIVER; // separate shared-input cohort qualification,NOT inference
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
	support.version=TT_Q38_DISPATCH_DRIVER; // separate persistent dispatch,NOT model qualification
#endif
	status=user_memcpy(data,&support,sizeof(support)); release_sem(sLock); return status;
}

static bool
PrepareCoordinatedFleet(PioneerStoreContext& c,unsigned workers,bool debug,bool parallel=false,bool glu=false,bool inlineMath=false,bool partialMath=false,bool cachedQuery=false,bool inputPull=false,bool phasePipeline=false,bool descriptorTable=false,bool accumPipeline=false,bool smallRows=false,bool parallelRope=false,const PioneerDecoderJob* model=NULL,bool fusedGather=false,bool vectorGlu=false,bool roundedProduct=false,bool inlineAttention=false,bool optimizedAttention=false,bool finiteHalf=false,bool unpackWaitGate=false)
{
	if (unpackWaitGate && (!finiteHalf || roundedProduct)) return false;
	if (finiteHalf && !optimizedAttention) return false;
	if (optimizedAttention && !inlineAttention) return false;
	if (inlineAttention && (!vectorGlu || roundedProduct)) return false;
	if (roundedProduct && (!vectorGlu || !accumPipeline || partialMath)) return false;
	if (vectorGlu && (!parallelRope || !glu)) return false;
	if (fusedGather && (!parallelRope || smallRows)) return false;
	if (parallelRope && (!accumPipeline || smallRows || !c.io.peerRopeEnabled)) return false;
	if ((smallRows && !accumPipeline) || (accumPipeline && !descriptorTable)) return false;
	if (descriptorTable && (!model || !phasePipeline || !inputPull || !cachedQuery || !inlineMath || partialMath || !glu || !parallel)) return false;
	bool ready=c.lease.ready && c.io.MapRuntime(true);
	if (ready && !c.fleet) {
		c.fleet=new(std::nothrow) PioneerFleetWorker[TT_PIONEER_WORKERS]; ready=c.fleet!=NULL;
		if (ready) {
			c.runtimeCoreMode=8; c.io.fleetEnabled=true; c.io.persistentHostStages=true; c.io.payloadBlocks=true;
			c.fleet[0].masterStore=&c.lease; c.fleet[0].masterCompute=&c.compute;
			c.fleet[0].noc=c.baseline.baseline.baseline.noc;
		}
	}
	auto& dram=c.baseline.baseline.baseline.baseline.dram;
	if (ready && !c.fleetRoutes) {
		ready=BlackholeDramAllCoresHeld(c.io,dram);
		for (unsigned w=1;w<TT_PIONEER_WORKERS && ready;w++) ready=PioneerWorkerNocProbe(c.io,dram,w,c.fleet[w].noc);
		c.fleetRoutes=ready;
	}
	if (ready) {
		auto& f=c.fleet[0];
		ready=c.io.AllocateFleetWorker(0) && c.io.SelectRuntimeCore(0)
			&& PioneerRuntimeMatvecPrepare(c.io,dram,f.noc,f.Store(),f.Compute(),false,true,true,true,true,true,true)
			&& PioneerFleetPreludePrepare(c.io,f) && c.io.MapCoordinator()
			&& PioneerCoordinatorPrepare(c.io,dram,f.noc,f.Store(),c.coordinator,true,debug,!debug,!debug);
	}
	if (ready) ready=PioneerCoordinatedMasterBoundary(c.io,dram,c.fleet,c.attention.ready);
	for (unsigned w=1;w<=workers && ready;w++) {
		auto& f=c.fleet[w];
		if (f.persistentActive) { ready=f.coordinated && f.persistentIdle && !f.Compute().running && f.attentionDispatch==(parallel && w<=8) && f.gluDispatch==(glu && w<=8) && f.inlineMath==inlineMath && f.coordinatedInline==inlineMath && f.partialMath==partialMath && f.phasePipeline==phasePipeline && f.descriptorTable==descriptorTable && f.accumPipeline==accumPipeline && f.roundedProduct==roundedProduct && f.unpackWaitGate==unpackWaitGate && f.smallRows==smallRows && f.fusedGather==fusedGather && (!descriptorTable || f.descriptorTablePrepared) && (w>8 || (c.peerAttention[w-1].cachedQuery==cachedQuery && c.peerAttention[w-1].inputPull==inputPull && c.peerAttention[w-1].parallelRope==parallelRope && c.peerAttention[w-1].vectorGlu==vectorGlu && c.peerAttention[w-1].inlineAttention==inlineAttention && c.peerAttention[w-1].optimizedAttention==optimizedAttention && c.peerAttention[w-1].finiteHalf==finiteHalf)); continue; }
		ready=c.io.AllocateFleetWorker(w) && c.io.SelectRuntimeCore(w);
		if (ready && !f.Store().ready) ready=!f.Store().saved && PioneerFleetLeaseBegin(c.io,dram,w,f);
		if (ready) ready=PioneerRuntimeMatvecPrepare(c.io,dram,f.noc,f.Store(),f.Compute(),false,true,true,true,true,true,true)
			&& PioneerFleetPreludePrepare(c.io,f);
		if (ready && parallel && w<=8) {
			ready=c.io.MapAttention(parallelRope) && PioneerAttentionTilePrepare(c.io,dram,f.noc,f.Store(),c.peerAttention[w-1],false,true,glu,cachedQuery,inputPull,parallelRope,vectorGlu,inlineAttention,optimizedAttention,finiteHalf);
			// The group entry returns to the persistent prelude;boot still
			// starts at that prelude,never directly into an attention job.
			if (!PioneerAttentionTileHeld(c.io,dram,f.noc,f.Store())) { c.peerAttention[w-1].unsafe=true; ready=false; }
			else { c.io.RuntimeCodeWrite(0,0x0006b06f); ready &= c.io.RuntimeCodeRead(0)==0x0006b06f; }
		}
		if (ready) ready=PioneerPersistentPrepare(c.io,f,inlineMath,true,parallel && w<=8,glu && w<=8,inlineMath,partialMath,phasePipeline,descriptorTable,accumPipeline,fusedGather,roundedProduct,unpackWaitGate)
			&& (!descriptorTable || PioneerPersistentTablePrepare(c.io,dram,f,*model,w,smallRows)) && PioneerPersistentBoot(c.io,f);
	}
	return ready && c.io.SelectRuntimeCore(0);
}

static status_t
DecoderModelControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	if (!data || length!=sizeof(tt_pioneer_decoder_model)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	auto* request=(tt_pioneer_decoder_model*)malloc(length); if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,length);
	if (status!=B_OK || request->version<1 || request->version>27 || (request->version<17 && (request->reserved[0] || request->reserved[1] || request->reserved[2]))) { free(request); return status!=B_OK?status:B_BAD_VALUE; }
	status=AcquireProbeLock(cookie); if (status!=B_OK) { free(request); return status; }
	auto* c=cookie->context;
	if (!c || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if (c->runtimeCoreMode!=8 || c->decoder.saved || c->decoderReady || c->bytes!=UINT64_C(0x80000000) || !c->fleet
		|| !c->bulk.ready || !c->bulkHost.ready || !c->wideHost.ready || c->residentAttention.reserved) status=B_NOT_ALLOWED;
	else if (request->version>=17?!PioneerDecoderBatchModelRegister(request->offsets,request->reserved[0],request->reserved[1],request->reserved[2],c->initialized,c->bytes,c->decoderModel)
		:!PioneerDecoderModelRegister(request->offsets,c->initialized,c->bytes,c->decoderModel)) status=B_ENTRY_NOT_FOUND;
	else {
		request->state=TT_RUNTIME_CONFIG_FAILED; request->ready=request->extents=request->recovery_required=0;
		auto& dram=c->baseline.baseline.baseline.baseline.dram;
		bool parallel=request->version>=2,glu=request->version>=3,greedy=request->version>=4,paired=request->version>=5,inlineMath=request->version>=6,partialMath=request->version==7,cachedQuery=request->version>=8,inputPull=request->version==9 || request->version==11 || request->version>=12,phasePipeline=request->version==10 || request->version>=12,diagnostic=request->version==11,descriptorTable=request->version>=13,accumPipeline=request->version>=14,smallRows=request->version==15,parallelRope=request->version>=16,batch=request->version>=17,fusedGather=request->version>=18,directQuant=request->version>=19,vectorGlu=request->version>=20,roundedProduct=request->version==21,inlineAttention=request->version>=22,optimizedMaster=request->version>=23,optimizedAttention=request->version>=24,finiteHalf=request->version>=25,fixedRoutes=request->version>=26,unpackWaitGate=request->version==27,ready=true;
		// The parallel registry must precede ANY peer preparation. Weight
		// spans prepare only master0. Never replace an executing/idle peer's
		// immutable code to upgrade an existing matrix session.
		if (parallel) for (unsigned w=1;w<120;w++) ready &= !c->fleet[w].Compute().saved && !c->fleet[w].persistentPrepared && !c->fleet[w].persistentActive;
		if (ready) { c->io.peerAttentionEnabled=parallel; c->io.peerRopeEnabled=parallelRope; ready=PrepareCoordinatedFleet(*c,119,false,parallel,glu,inlineMath,partialMath,cachedQuery,inputPull,phasePipeline,descriptorTable,accumPipeline,smallRows,parallelRope,descriptorTable?&c->decoderModel:NULL,fusedGather,vectorGlu,roundedProduct,inlineAttention,optimizedAttention,finiteHalf,unpackWaitGate); }
		ready=ready && PioneerCoordinatedMasterBoundary(c->io,dram,c->fleet,c->attention.ready);
		// One-way seal:all legacy mutable/runtime operations reject mode9,
		// including both legacy page/stream paths. END still live-stops ALL
		// persistent workers before restoring either separate code owner.
		c->runtimeCoreMode=9;
		ready=ready && PioneerLayerClose(c->io,dram,c->fleet[0].noc,c->lease,c->layer,true)
			&& c->io.MapAttention() && PioneerAttentionTilePrepare(c->io,dram,c->fleet[0].noc,c->lease,c->attention,true)
			&& c->io.MapDecoder() && PioneerDecoderPrepare(c->io,dram,c->fleet,c->coordinator,c->layer,c->attention,c->decoder,parallel,parallel?c->peerAttention:NULL,parallel,glu,greedy,paired,inlineMath,partialMath,cachedQuery,inputPull,phasePipeline,diagnostic,descriptorTable,accumPipeline,smallRows,parallelRope,batch,fusedGather,directQuant,vectorGlu,roundedProduct,inlineAttention,optimizedMaster,optimizedAttention,finiteHalf,fixedRoutes,unpackWaitGate);
		if (ready && (!PioneerCoordinatedMasterBoundary(c->io,dram,c->fleet,true)
			|| !PioneerWideOutputMapStable(c->io,c->wideHost,c->host,c->bulkHost))) { c->decoder.unsafe=true; ready=false; }
		if (ready) {
			c->residentAttention.reserved=c->residentAttention.payloadBlocks=c->residentAttention.coordinated=true;
			c->decoderReady=true; request->state=TT_RUNTIME_PASSED; request->ready=1; request->extents=batch?311:310;
		}
		if (c->Unsafe()) { c->lease.recovery_required=true; c->io.QuarantineHostPage(); sColdRecoveryRequired=true; request->recovery_required=1; }
		status=user_memcpy(data,request,length);
	}
	release_sem(sLock); free(request); return status;
}

template<class Run> static status_t
DecoderRunControlImpl(PioneerProbeCookie* cookie,void* data,size_t length,bool greedy,bool batch=false)
{
	if (!data || length!=sizeof(Run)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	auto* request=(Run*)malloc(length); if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,offsetof(Run,result));
	if (status!=B_OK || request->version!=1 || !request->tokens || request->tokens>256 || (batch?(!request->reserved[0] || request->reserved[0]>2 || request->reserved[0]>257-request->tokens):request->reserved[0]!=0) || request->reserved[1]) { free(request); return status!=B_OK?status:B_BAD_VALUE; }
	if (batch) for (unsigned i=0;i<128;i++) if (request->rope[i]) { free(request); return B_BAD_VALUE; }
	status=AcquireProbeLock(cookie); if (status!=B_OK) { free(request); return status; }
	auto* c=cookie->context;
	if (!c || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if (c->runtimeCoreMode!=9 || !c->decoderReady || !c->decoder.ready || !c->fleet || (greedy && !c->decoder.greedy) || (batch && !c->decoder.batch)) status=B_NOT_ALLOWED;
	else {
		request->result={}; memset(request->output,0,sizeof(request->output)); c->io.runtimeProfile.Begin(uint64(system_time()),21);
		c->io.RuntimeProfileMark(TT_PROFILE_PREPARED);
		PioneerDecoderRun(c->io,c->baseline.baseline.baseline.baseline.dram,c->host,c->bulkHost,c->wideHost,c->fleet,c->bulk,
			c->coordinator,c->decoder,c->attention,c->residentAttention,c->decoderModel,request->tokens,request->embedding,request->rope,request->output,request->result,greedy,batch?request->reserved[0]:0);
		if (c->Unsafe()) { c->lease.recovery_required=true; c->io.QuarantineHostPage(); sColdRecoveryRequired=true; request->result.master.recovery_required=1; }
		auto& r=request->result.master; c->io.runtimeProfile.End(uint64(system_time()),r.state,r.recovery_required,r.started,r.completed,r.stopped);
		if (batch?!PioneerDecoderBatchProof(request->result,request->tokens,request->output[3],c->coordinator.nonce,c->decoder)
			:!PioneerDecoderProof(request->result,request->tokens,c->coordinator.nonce,c->decoder,greedy)) memset(request->output,0,sizeof(request->output));
		status=user_memcpy(data,request,length);
	}
	release_sem(sLock); free(request); return status;
}
static status_t DecoderRunControl(PioneerProbeCookie* cookie,void* data,size_t length)
{ return DecoderRunControlImpl<tt_pioneer_decoder_run>(cookie,data,length,false); }
static status_t DecoderGreedyControl(PioneerProbeCookie* cookie,void* data,size_t length)
{ return DecoderRunControlImpl<tt_pioneer_decoder_greedy>(cookie,data,length,true); }
static status_t DecoderBatchControl(PioneerProbeCookie* cookie,void* data,size_t length)
{ return DecoderRunControlImpl<tt_pioneer_decoder_batch>(cookie,data,length,false,true); }

static status_t
DecoderStageProfileControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	if (!data || length!=sizeof(tt_decoder_stage_profile)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	status=AcquireProbeLock(cookie); if (status!=B_OK) return status;
	auto* c=cookie->context;
	// PRIVATE CPU snapshot only. No map,MMIO,DMA,probe,launch or reset.
	if (!c || sRuntimeOwner!=cookie || c->runtimeCoreMode!=9 || !c->decoderReady || !c->decoder.profiling
		|| c->decoder.running || c->decoder.unsafe || c->decoder.stages.magic!=0x31525044) status=B_DEV_NOT_READY;
	else status=user_memcpy(data,&c->decoder.stages,sizeof(c->decoder.stages));
	release_sem(sLock); return status;
}

static status_t
LayerSpanControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	const size_t prefix=offsetof(tt_pioneer_layer_span,data);
	if (!data || length<prefix || length>sizeof(tt_pioneer_layer_span)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	auto* request=(tt_pioneer_layer_span*)malloc(length); if (!request) return B_NO_MEMORY;
	// Freeze the complete request in ONE user copy; no double-fetch size or
	// later caller mutation can change the kernel-private upload/comparison.
	status=user_memcpy(request,data,length);
	if (status!=B_OK || request->version!=1 || request->reserved[0] || request->reserved[1]
		|| request->payload_reserved || length!=PioneerSpanRequestBytes(request->bytes)) { free(request); return status!=B_OK?status:B_BAD_VALUE; }
	status=AcquireProbeLock(cookie); if (status!=B_OK) { free(request); return status; }
	auto* c=cookie->context;
	if (!c || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if ((c->runtimeCoreMode!=-1 && c->runtimeCoreMode!=8) || c->bytes!=UINT64_C(0x80000000)
		|| (c->coordinator.ready && (!c->coordinator.fused || !c->coordinator.compact))) status=B_NOT_ALLOWED;
	else if (!PioneerResidentOffset(c->bytes,request->offset) || request->offset>=c->bytes-TT_RESIDENT_KV_BYTES
		|| request->bytes>c->bytes-TT_RESIDENT_KV_BYTES-request->offset) status=B_BAD_VALUE;
	else {
		unsigned first=unsigned(request->offset/4096),pages=request->bytes/4096; bool fresh=true;
		for (unsigned p=0;p<pages;p++) fresh &= !c->initialized[first+p];
		if (!fresh) status=B_NOT_ALLOWED;
		else {
			request->result={}; request->result.master.version=1; request->result.master.state=TT_RUNTIME_CONFIG_FAILED;
			c->io.runtimeProfile.Begin(uint64(system_time()),19); auto& dram=c->baseline.baseline.baseline.baseline.dram;
			bool ready=c->lease.ready || (!c->lease.saved && PioneerResidentLeaseBegin(c->io,c->baseline.baseline.baseline.noc,c->lease));
			if (ready && c->runtimeCoreMode==8) ready=PioneerCoordinatedMasterBoundary(c->io,dram,c->fleet,c->attention.ready);
			if (ready) ready=c->io.SelectRuntimeCore(0) && c->io.MapBulk();
			if (ready && !c->bulkHost.allocated) ready=PioneerPrepareOutputBuffer(c->io,c->host,c->bulkHost);
			ready=ready && c->bulkHost.ready && PioneerBulkUploadPrepare(c->io,dram,c->baseline.baseline.baseline.noc,
				c->host,c->lease,c->bulkHost,c->bulk);
			ready=ready && PrepareCoordinatedFleet(*c,0,false) && PioneerCoordinatedMasterBoundary(c->io,dram,c->fleet,c->attention.ready)
				&& c->io.MapLayer() && PioneerLayerPrepare(c->io,dram,c->fleet[0].noc,c->lease,c->coordinator,c->layer);
			if (ready && !c->wideHost.ready) ready=!c->wideHost.allocated && PioneerPrepareWideOutput(c->io,c->host,c->bulkHost,c->wideHost);
			if (ready) {
				c->io.RuntimeProfileMark(TT_PROFILE_PREPARED);
				PioneerSpanUpload(c->io,dram,c->host,c->bulkHost,c->wideHost,c->fleet,c->bulk,c->coordinator,c->layer,c->attention.ready,
					c->bytes,request->offset,request->bytes,request->data,request->result);
			}
			if (c->Unsafe()) {
				c->layer.unsafe=c->coordinator.unsafe=c->bulk.unsafe=true; c->lease.recovery_required=true; c->io.QuarantineHostPage();
				PioneerQuarantineOutputBuffer(c->io,c->bulkHost); PioneerQuarantineWideOutput(c->io,c->wideHost);
				sColdRecoveryRequired=true; request->result.master.recovery_required=1;
			}
			auto& r=request->result.master;
			c->io.runtimeProfile.End(uint64(system_time()),r.state,r.recovery_required,r.started,r.completed,r.stopped);
			// Only the COMPLETE transport proof and independent frozen CPU
			// echo permit ALL pages to become initialized. Never partial bits.
			if (PioneerSpanProof(request->result,request->bytes,c->coordinator.nonce)) for (unsigned p=0;p<pages;p++) {
				c->hashes[first+p]=PioneerResidentPageHash(request->data+p*1024); c->initialized[first+p]=1; c->uploadedPages++;
			}
			// Return metadata only,not a redundant1MiB payload copy.
			status=user_memcpy(data,request,prefix);
		}
	}
	release_sem(sLock); free(request); return status;
}

static status_t
LayerWideControl(PioneerProbeCookie* cookie,void* data,size_t length,bool narrow=false)
{
	const size_t prefix=offsetof(tt_pioneer_coordinated_fleet,output);
	if (!data || length<prefix || length>sizeof(tt_pioneer_coordinated_fleet)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	auto* request=(tt_pioneer_coordinated_fleet*)malloc(length); if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,prefix); PioneerFleetPlan plan;
	if (status!=B_OK || request->version!=1 || !request->workers || request->workers>119
		|| length!=PioneerCoordinatedRequestBytes(request->outputs)
		|| (narrow && request->outputs>4096) || !PioneerFleetPartition(request->inputs,request->outputs,request->workers,plan)) {
		free(request); return status!=B_OK?status:B_BAD_VALUE;
	}
	status=AcquireProbeLock(cookie); if (status!=B_OK) { free(request); return status; }
	auto* c=cookie->context; uint64 bytes=PioneerQ8CompactWeightBytes(request->inputs,request->outputs);
	if (!c || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if ((c->runtimeCoreMode!=-1 && c->runtimeCoreMode!=8) || c->bytes!=UINT64_C(0x80000000)
		|| !c->bulk.ready || !c->bulkHost.ready || (c->coordinator.ready && (!c->coordinator.fused || !c->coordinator.compact))) status=B_NOT_ALLOWED;
	else if (!PioneerResidentOffset(c->bytes,request->weight_offset) || !bytes || request->weight_offset>=c->bytes-TT_RESIDENT_KV_BYTES
		|| bytes>c->bytes-TT_RESIDENT_KV_BYTES-request->weight_offset) status=B_BAD_VALUE;
	else {
		bool initialized=true;
		for (uint64 page=request->weight_offset/4096;page<(request->weight_offset+bytes+4095)/4096;page++) initialized &= c->initialized[page]!=0;
		if (!initialized) status=B_ENTRY_NOT_FOUND;
		else {
			request->result={}; request->result.master.version=1; request->result.master.state=TT_RUNTIME_CONFIG_FAILED;
			memset(request->output,0,request->outputs*4); c->io.runtimeProfile.Begin(uint64(system_time()),narrow?20:18);
			auto& dram=c->baseline.baseline.baseline.baseline.dram;
			bool ready=PrepareCoordinatedFleet(*c,plan.workers,false)
				&& PioneerCoordinatedMasterBoundary(c->io,dram,c->fleet,c->attention.ready)
				&& c->io.MapLayer() && PioneerLayerPrepare(c->io,dram,c->fleet[0].noc,c->lease,c->coordinator,c->layer);
			if (ready && !narrow && !c->wideHost.ready) ready=!c->wideHost.allocated && PioneerPrepareWideOutput(c->io,c->host,c->bulkHost,c->wideHost);
			if (ready) {
				c->io.RuntimeProfileMark(TT_PROFILE_PREPARED);
				PioneerLayerJob job(&c->layer,0,0,narrow?3:6,narrow?NULL:&c->wideHost);
				PioneerCoordinatedMatvec(c->io,dram,c->host,c->bulkHost,c->fleet,c->bulk,c->coordinator,c->attention.ready,c->bytes,
					request->weight_offset,request->inputs,request->outputs,request->workers,request->input,request->scale,request->output,request->result,&job);
			}
			if (c->Unsafe()) {
				c->layer.unsafe=true; c->coordinator.unsafe=true; c->lease.recovery_required=true; c->io.QuarantineHostPage();
				PioneerQuarantineOutputBuffer(c->io,c->bulkHost); PioneerQuarantineWideOutput(c->io,c->wideHost);
				sColdRecoveryRequired=true; request->result.master.recovery_required=1;
			}
			auto& r=request->result.master;
			c->io.runtimeProfile.End(uint64(system_time()),r.state,r.recovery_required,r.started,r.completed,r.stopped);
			if (!(narrow?PioneerLayerNarrowProof(request->result,request->inputs,request->outputs,request->workers,c->coordinator.nonce)
				:PioneerLayerWideProof(request->result,request->inputs,request->outputs,request->workers,c->coordinator.nonce))) memset(request->output,0,request->outputs*4);
			status=user_memcpy(data,request,length);
		}
	}
	release_sem(sLock); free(request); return status;
}

static status_t
LayerGluControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	if (!data || length!=sizeof(tt_pioneer_layer_glu)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	auto* request=(tt_pioneer_layer_glu*)malloc(length); if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,offsetof(tt_pioneer_layer_glu,result));
	if (status!=B_OK || request->version!=1 || !request->outputs || request->outputs>3072 || (request->outputs&31)
		|| request->reserved[0] || request->reserved[1]) { free(request); return status!=B_OK?status:B_BAD_VALUE; }
	status=AcquireProbeLock(cookie); if (status!=B_OK) { free(request); return status; }
	auto* c=cookie->context;
	if (!c || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if ((c->runtimeCoreMode!=-1 && c->runtimeCoreMode!=8) || c->bytes!=UINT64_C(0x80000000)
		|| !c->bulk.ready || !c->bulkHost.ready || (c->coordinator.ready && (!c->coordinator.fused || !c->coordinator.compact))) status=B_NOT_ALLOWED;
	else {
		request->result={}; request->result.master.version=1; request->result.master.state=TT_RUNTIME_CONFIG_FAILED;
		memset(request->output,0,sizeof(request->output)); c->io.runtimeProfile.Begin(uint64(system_time()),17);
		auto& dram=c->baseline.baseline.baseline.baseline.dram;
		bool ready=PrepareCoordinatedFleet(*c,0,false) && PioneerCoordinatedMasterBoundary(c->io,dram,c->fleet,c->attention.ready)
			&& c->io.MapLayer() && PioneerLayerPrepare(c->io,dram,c->fleet[0].noc,c->lease,c->coordinator,c->layer);
		if (ready) PioneerLayerGlu(c->io,dram,c->host,c->bulkHost,c->fleet,c->bulk,c->coordinator,c->layer,c->attention.ready,
			request->outputs,request->gate,request->up,request->output,request->result);
		if (c->Unsafe()) {
			c->layer.unsafe=true; c->coordinator.unsafe=true; c->lease.recovery_required=true; c->io.QuarantineHostPage();
			PioneerQuarantineOutputBuffer(c->io,c->bulkHost); sColdRecoveryRequired=true; request->result.master.recovery_required=1;
		}
		auto& r=request->result.master;
		c->io.runtimeProfile.End(uint64(system_time()),r.state,r.recovery_required,r.started,r.completed,r.stopped);
		if (!PioneerLayerGluProof(request->result,request->outputs,c->coordinator.nonce)) memset(request->output,0,sizeof(request->output));
		status=user_memcpy(data,request,length);
	}
	release_sem(sLock); free(request); return status;
}

static status_t
LayerFfnControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	if (!data || length!=sizeof(tt_pioneer_layer_ffn)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	auto* request=(tt_pioneer_layer_ffn*)malloc(length); if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,offsetof(tt_pioneer_layer_ffn,result));
	if (status!=B_OK || request->version!=1 || !request->workers || request->workers>119 || request->reserved[0] || request->reserved[1]) {
		free(request); return status!=B_OK?status:B_BAD_VALUE;
	}
	status=AcquireProbeLock(cookie); if (status!=B_OK) { free(request); return status; }
	auto* c=cookie->context;
	if (!c || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if ((c->runtimeCoreMode!=-1 && c->runtimeCoreMode!=8) || c->bytes!=UINT64_C(0x80000000)
		|| !c->bulk.ready || !c->bulkHost.ready || (c->coordinator.ready && (!c->coordinator.fused || !c->coordinator.compact))) status=B_NOT_ALLOWED;
	else {
		const uint64 offsets[]={request->gate_offset,request->up_offset,request->down_offset};
		const uint64 bytes=PioneerQ8CompactWeightBytes(1024,3072);
		bool bounded=true,initialized=true;
		for (unsigned i=0;i<3 && bounded;i++) {
			uint64 at=offsets[i];
			bounded=PioneerResidentOffset(c->bytes,at) && at<c->bytes-TT_RESIDENT_KV_BYTES && bytes<=c->bytes-TT_RESIDENT_KV_BYTES-at;
			if (bounded) for (uint64 page=at/4096;page<(at+bytes+4095)/4096;page++) initialized &= c->initialized[page]!=0;
		}
		if (!bounded) status=B_BAD_VALUE;
		else if (!initialized) status=B_ENTRY_NOT_FOUND;
		else {
			request->result={}; request->result.master.version=1; request->result.master.state=TT_RUNTIME_CONFIG_FAILED;
			memset(request->output,0,sizeof(request->output));
			c->io.runtimeProfile.Begin(uint64(system_time()),16);
			auto& dram=c->baseline.baseline.baseline.baseline.dram;
			unsigned workers=request->workers<96?request->workers:96;
			bool ready=PrepareCoordinatedFleet(*c,workers,false)
				&& PioneerCoordinatedMasterBoundary(c->io,dram,c->fleet,c->attention.ready)
				&& c->io.MapLayer() && PioneerLayerPrepare(c->io,dram,c->fleet[0].noc,c->lease,c->coordinator,c->layer);
			if (ready) {
				c->io.RuntimeProfileMark(TT_PROFILE_PREPARED);
				PioneerLayerJob job={&c->layer,request->up_offset,request->down_offset};
				PioneerCoordinatedMatvec(c->io,dram,c->host,c->bulkHost,c->fleet,c->bulk,c->coordinator,c->attention.ready,
					c->bytes,request->gate_offset,1024,1024,request->workers,request->input,request->scale,request->output,request->result,&job);
			}
			if (c->Unsafe()) {
				c->layer.unsafe=true; c->coordinator.unsafe=true; c->lease.recovery_required=true; c->io.QuarantineHostPage();
				PioneerQuarantineOutputBuffer(c->io,c->bulkHost); sColdRecoveryRequired=true; request->result.master.recovery_required=1;
			}
			auto& r=request->result.master;
			c->io.runtimeProfile.End(uint64(system_time()),r.state,r.recovery_required,r.started,r.completed,r.stopped);
			if (!PioneerCoordinatedLayerProof(request->result,request->workers,c->coordinator.nonce)) memset(request->output,0,sizeof(request->output));
			status=user_memcpy(data,request,length);
		}
	}
	release_sem(sLock); free(request); return status;
}

static status_t
CoordinatedFleetControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	const size_t prefix=offsetof(tt_pioneer_coordinated_fleet,output);
	if (!data || length<prefix || length>sizeof(tt_pioneer_coordinated_fleet)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	auto* request=(tt_pioneer_coordinated_fleet*)malloc(length); if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,prefix);
	PioneerFleetPlan plan;
	if (status!=B_OK || (request->version!=1 && request->version!=2) || !request->workers || request->workers>119
		|| length!=PioneerCoordinatedRequestBytes(request->outputs)
		|| !PioneerFleetPartition(request->inputs,request->outputs,request->workers,plan)) {
		free(request); return status!=B_OK?status:B_BAD_VALUE;
	}
	status=AcquireProbeLock(cookie); if (status!=B_OK) { free(request); return status; }
	auto* context=cookie->context;
	uint64 bytes=PioneerQ8CompactWeightBytes(request->inputs,request->outputs);
	if (!context || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if ((context->runtimeCoreMode!=-1 && context->runtimeCoreMode!=8) || context->bytes!=UINT64_C(0x80000000)
		|| !context->bulk.ready || !context->bulkHost.ready) status=B_NOT_ALLOWED;
	else if (context->coordinator.ready && context->coordinator.fused!=(request->version==1)) status=B_NOT_ALLOWED;
	else if (!PioneerResidentOffset(context->bytes,request->weight_offset) || !bytes
		|| request->weight_offset>=context->bytes-TT_RESIDENT_KV_BYTES
		|| bytes>context->bytes-TT_RESIDENT_KV_BYTES-request->weight_offset) status=B_BAD_VALUE;
	else {
		bool initialized=true;
		for (uint64 page=request->weight_offset/4096;page<(request->weight_offset+bytes+4095)/4096;page++) initialized &= context->initialized[page]!=0;
		if (!initialized) status=B_ENTRY_NOT_FOUND;
		else {
			request->result={}; request->result.master.version=1; request->result.master.state=TT_RUNTIME_CONFIG_FAILED;
			memset(request->output,0,request->outputs*4);
			context->io.runtimeProfile.Begin(uint64(system_time()),15);
			bool ready=context->lease.ready && context->io.MapRuntime(true);
			if (ready && !context->fleet) {
				context->fleet=new(std::nothrow) PioneerFleetWorker[TT_PIONEER_WORKERS]; ready=context->fleet!=NULL;
				if (ready) {
					context->runtimeCoreMode=8; context->io.fleetEnabled=true;
					context->io.persistentHostStages=true; context->io.payloadBlocks=true;
					context->fleet[0].masterStore=&context->lease; context->fleet[0].masterCompute=&context->compute;
					context->fleet[0].noc=context->baseline.baseline.baseline.noc;
				}
			}
			auto& dram=context->baseline.baseline.baseline.baseline.dram;
			if (ready && !context->fleetRoutes) {
				ready=BlackholeDramAllCoresHeld(context->io,dram);
				for (unsigned w=1;w<TT_PIONEER_WORKERS && ready;w++) ready=PioneerWorkerNocProbe(context->io,dram,w,context->fleet[w].noc);
				context->fleetRoutes=ready;
			}
			if (ready) {
				auto& f=context->fleet[0];
				ready=context->io.AllocateFleetWorker(0) && context->io.SelectRuntimeCore(0)
					&& PioneerRuntimeMatvecPrepare(context->io,dram,f.noc,f.Store(),f.Compute(),false,true,true,true,true,true,true)
					&& PioneerFleetPreludePrepare(context->io,f) && context->io.MapCoordinator()
					&& PioneerCoordinatorPrepare(context->io,dram,f.noc,f.Store(),context->coordinator,true,request->version==2,request->version==1,request->version==1);
			}
			if (ready) ready=PioneerCoordinatedMasterBoundary(context->io,dram,context->fleet,context->attention.ready);
			for (unsigned w=1;w<=plan.workers && ready;w++) {
				auto& f=context->fleet[w];
				if (f.persistentActive) { ready=f.coordinated && f.persistentIdle && !f.Compute().running; continue; }
				ready=context->io.AllocateFleetWorker(w) && context->io.SelectRuntimeCore(w);
				if (ready && !f.Store().ready) ready=!f.Store().saved && PioneerFleetLeaseBegin(context->io,dram,w,f);
				if (ready) ready=PioneerRuntimeMatvecPrepare(context->io,dram,f.noc,f.Store(),f.Compute(),false,true,true,true,true,true,true)
					&& PioneerFleetPreludePrepare(context->io,f) && PioneerPersistentPrepare(context->io,f,false,true)
					&& PioneerPersistentBoot(context->io,f);
			}
			if (ready) {
				context->io.RuntimeProfileMark(TT_PROFILE_PREPARED);
				PioneerCoordinatedMatvec(context->io,dram,context->host,context->bulkHost,context->fleet,context->bulk,context->coordinator,
					context->attention.ready,context->bytes,request->weight_offset,request->inputs,request->outputs,request->workers,
					request->input,request->scale,request->output,request->result);
			}
			if (context->Unsafe()) {
				context->coordinator.unsafe=true; context->lease.recovery_required=true; context->io.QuarantineHostPage();
				PioneerQuarantineOutputBuffer(context->io,context->bulkHost); sColdRecoveryRequired=true;
				request->result.master.recovery_required=1;
			}
			auto& r=request->result.master;
			context->io.runtimeProfile.End(uint64(system_time()),r.state,r.recovery_required,r.started,r.completed,r.stopped);
			if (!PioneerCoordinatedProof(request->result,request->inputs,request->outputs,request->workers,context->coordinator.nonce,request->version==2))
				memset(request->output,0,request->outputs*4);
			status=user_memcpy(data,request,length);
		}
	}
	release_sem(sLock); free(request); return status;
}

static status_t
RuntimeFleetControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	const size_t prefix=sizeof(tt_pioneer_runtime_fleet)-TT_FLEET_OUTPUTS_MAX*sizeof(uint32_t);
	if (!data || length<prefix || length>sizeof(tt_pioneer_runtime_fleet)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	// Allocate the checked inline message, not a fresh1MiB maximum-sized
	// heap block for every tiny projection. All later accesses are bounded
	// by the immutable copied header and the exact length equality below.
	auto* request=(tt_pioneer_runtime_fleet*)malloc(length);
	if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,prefix);
	PioneerFleetPlan plan;
	if (status!=B_OK || request->version!=2 || length!=PioneerFleetRequestBytes(request->outputs)
		|| !PioneerFleetPartition(request->inputs,request->outputs,request->workers,plan)) {
		free(request); return status!=B_OK?status:B_BAD_VALUE;
	}
	status=AcquireProbeLock(cookie); if (status!=B_OK) { free(request); return status; }
	PioneerStoreContext* context=cookie->context;
	uint64 bytes=PioneerQ8CompactWeightBytes(request->inputs,request->outputs);
	if (!context || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if (!PioneerResidentOffset(context->bytes,request->weight_offset) || bytes>context->bytes-request->weight_offset) status=B_BAD_VALUE;
	else {
		bool initialized=true;
		for (uint64 page=request->weight_offset/4096;page<(request->weight_offset+bytes+4095)/4096;page++) initialized &= context->initialized[page]!=0;
		if (!initialized) status=B_ENTRY_NOT_FOUND;
		else {
			memset(request->output,0,request->outputs*4); request->result={}; request->result.version=1;
			request->result.state=TT_RUNTIME_CONFIG_FAILED;
			context->io.runtimeProfile.Begin(uint64(system_time()),12);
			bool ready=context->runtimeCoreMode==-1 || context->runtimeCoreMode==5;
			ready=ready && (context->lease.ready || (!context->lease.saved
				&& PioneerResidentLeaseBegin(context->io,context->baseline.baseline.baseline.noc,context->lease)));
			if (ready) ready=context->io.MapRuntime(true);
			if (ready && !context->fleet) {
				context->fleet=new(std::nothrow) PioneerFleetWorker[TT_PIONEER_WORKERS];
				ready=context->fleet!=NULL;
				if (ready) {
					context->runtimeCoreMode=5; context->io.fleetEnabled=true;
					context->fleet[0].masterStore=&context->lease; context->fleet[0].masterCompute=&context->compute;
					context->fleet[0].noc=context->baseline.baseline.baseline.noc;
				}
			}
			auto& dram=context->baseline.baseline.baseline.baseline.dram;
			if (ready && !context->fleetRoutes) {
				ready=BlackholeDramAllCoresHeld(context->io,dram);
				for (unsigned w=1;w<TT_PIONEER_WORKERS && ready;w++)
					ready=PioneerWorkerNocProbe(context->io,dram,w,context->fleet[w].noc);
				context->fleetRoutes=ready;
			}
			for (unsigned w=0;w<plan.workers && ready;w++) {
				auto& f=context->fleet[w];
				ready=context->io.AllocateFleetWorker(w) && context->io.SelectRuntimeCore(w);
				if (ready && !f.Store().ready) ready=!f.Store().saved && PioneerFleetLeaseBegin(context->io,dram,w,f);
				if (ready) ready=PioneerRuntimeMatvecPrepare(context->io,dram,f.noc,f.Store(),f.Compute(),false,true,true,true,true,true,true);
				if (ready) ready=PioneerFleetPreludePrepare(context->io,f);
			}
			if (ready && context->attention.saved) {
				ready=context->io.SelectRuntimeCore(0);
				if (ready && context->io.RuntimeCodeRead(0)!=0x0006b06f) ready=context->residentAttention.reserved
					?SelectResidentFleetEntry(*context,false):SelectFleetEntry(*context,false);
			}
			if (ready) {
				context->io.RuntimeProfileMark(TT_PROFILE_PREPARED);
				PioneerFleetMatvec(context->io,dram,context->host,context->fleet,context->bytes,request->weight_offset,
					request->inputs,request->outputs,request->workers,request->input,request->scale,request->output,request->result);
			}
			if (!context->io.SelectRuntimeCore(0)) { context->lease.recovery_required=true; request->result.recovery_required=1; }
			if (context->Unsafe()) { context->io.QuarantineHostPage(); request->result.recovery_required=1; sColdRecoveryRequired=true; }
			auto& r=request->result;
			context->io.runtimeProfile.End(uint64(system_time()),r.state,r.recovery_required,r.started,r.completed,r.stopped);
			// Never expose even a partial output vector after failed coverage.
			if (!PioneerFleetProof(r,request->inputs,request->outputs,request->workers)) memset(request->output,0,request->outputs*4);
			status=user_memcpy(data,request,length);
		}
	}
	release_sem(sLock); free(request); return status;
}

static status_t
RuntimePersistentFleetControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	const size_t prefix=offsetof(tt_pioneer_runtime_persistent_fleet,output);
	if (!data || length<prefix || length>sizeof(tt_pioneer_runtime_persistent_fleet)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	auto* request=(tt_pioneer_runtime_persistent_fleet*)malloc(length);
	if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,prefix); PioneerFleetPlan plan;
	if (status!=B_OK || (request->version!=1 && request->version!=2) || length!=PioneerPersistentRequestBytes(request->outputs)
		|| !PioneerFleetPartition(request->inputs,request->outputs,request->workers,plan)) {
		free(request); return status!=B_OK?status:B_BAD_VALUE;
	}
	status=AcquireProbeLock(cookie); if (status!=B_OK) { free(request); return status; }
	PioneerStoreContext* context=cookie->context;
	uint64 bytes=PioneerQ8CompactWeightBytes(request->inputs,request->outputs);
	bool inlineMath=request->version==2; int mode=inlineMath?7:6;
	if (!context || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if (context->runtimeCoreMode!=-1 && context->runtimeCoreMode!=mode) status=B_NOT_ALLOWED;
	else if (!PioneerResidentOffset(context->bytes,request->weight_offset) || bytes>context->bytes-request->weight_offset) status=B_BAD_VALUE;
	else {
		bool initialized=true;
		for (uint64 page=request->weight_offset/4096;page<(request->weight_offset+bytes+4095)/4096;page++) initialized &= context->initialized[page]!=0;
		if (!initialized) status=B_ENTRY_NOT_FOUND;
		else {
			memset(request->output,0,request->outputs*4); request->result={}; request->result.version=1;
			request->result.state=TT_RUNTIME_CONFIG_FAILED; context->io.runtimeProfile.Begin(uint64(system_time()),13);
			bool ready=context->lease.ready || (!context->lease.saved
				&& PioneerResidentLeaseBegin(context->io,context->baseline.baseline.baseline.noc,context->lease));
			if (ready) ready=context->io.MapRuntime(true);
			if (ready && !context->fleet) {
				context->fleet=new(std::nothrow) PioneerFleetWorker[TT_PIONEER_WORKERS]; ready=context->fleet!=NULL;
				if (ready) {
					context->runtimeCoreMode=mode; context->io.fleetEnabled=true; context->io.persistentHostStages=true;
					context->fleet[0].masterStore=&context->lease; context->fleet[0].masterCompute=&context->compute;
					context->fleet[0].noc=context->baseline.baseline.baseline.noc;
				}
			}
			auto& dram=context->baseline.baseline.baseline.baseline.dram;
			if (ready && !context->fleetRoutes) {
				ready=BlackholeDramAllCoresHeld(context->io,dram);
				for (unsigned w=1;w<TT_PIONEER_WORKERS && ready;w++) ready=PioneerWorkerNocProbe(context->io,dram,w,context->fleet[w].noc);
				context->fleetRoutes=ready;
			}
			for (unsigned w=0;w<plan.workers && ready;w++) {
				auto& f=context->fleet[w];
				ready=context->io.AllocateFleetWorker(w) && context->io.SelectRuntimeCore(w);
				if (ready && !f.Store().ready) ready=!f.Store().saved && PioneerFleetLeaseBegin(context->io,dram,w,f);
				if (ready) ready=PioneerRuntimeMatvecPrepare(context->io,dram,f.noc,f.Store(),f.Compute(),false,true,true,true,true,true,true);
				if (ready && !f.preludeSaved) ready=PioneerFleetPreludePrepare(context->io,f);
				if (ready) ready=PioneerPersistentPrepare(context->io,f,inlineMath);
			}
			if (ready) {
				context->io.RuntimeProfileMark(TT_PROFILE_PREPARED);
				PioneerPersistentMatvec(context->io,dram,context->host,context->fleet,context->bytes,request->weight_offset,
					request->inputs,request->outputs,request->workers,request->input,request->scale,request->output,request->result);
			}
			if (!context->io.SelectRuntimeCore(0)) { context->lease.recovery_required=true; request->result.recovery_required=1; }
			if (context->Unsafe()) { context->io.QuarantineHostPage(); request->result.recovery_required=1; sColdRecoveryRequired=true; }
			auto& r=request->result;
			context->io.runtimeProfile.End(uint64(system_time()),r.state,r.recovery_required,r.started,r.completed,0);
			if (!PioneerPersistentProof(r,request->inputs,request->outputs,request->workers)) memset(request->output,0,request->outputs*4);
			status=user_memcpy(data,request,length);
		}
	}
	release_sem(sLock); free(request); return status;
}

static status_t
RuntimeProfileControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	if (data==NULL || length!=sizeof(tt_runtime_profile_result)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	status=AcquireProbeLock(cookie); if (status!=B_OK) return status;
	if (!cookie->context || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else status=user_memcpy(data,&cookie->context->io.runtimeProfile.Snapshot(),sizeof(tt_runtime_profile_result));
	release_sem(sLock); return status;
}

static status_t
ResidentAttentionControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	if (!data || length!=sizeof(tt_pioneer_resident_attention)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	auto* request=(tt_pioneer_resident_attention*)malloc(length); if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,length);
	if (status!=B_OK || (request->version!=1 && request->version!=2 && request->version!=3) || request->reserved
		|| !PioneerResidentAttentionInput(request->tokens,request->layer,request->input)) {
		free(request); return status!=B_OK?status:B_BAD_VALUE;
	}
	status=AcquireProbeLock(cookie); if (status!=B_OK) { free(request); return status; }
	auto* context=cookie->context;
	if (!context || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if ((context->runtimeCoreMode!=5 && context->runtimeCoreMode!=8) || !context->fleet || !context->compute.ready
		|| !context->uploadedPages || !PioneerResidentAttentionCapacity(context->bytes)) status=B_NOT_ALLOWED;
	else {
		memset(&request->result,0,sizeof(request->result)); memset(request->output,0,sizeof(request->output));
		auto& kv=context->residentAttention;
		bool ready=(!context->attention.saved || context->attention.resident)
			&& request->tokens<=kv.tokens[request->layer]+1
			&& PioneerResidentAttentionVersion(request->version,kv,context->runtimeCoreMode==8);
		if (ready && !kv.reserved) {
			for (uint64 page=(context->bytes-TT_RESIDENT_KV_BYTES)/4096;page<context->bytes/4096;page++)
				ready &= !context->initialized[page];
			if (ready) {
				kv.reserved=true; kv.payloadBlocks=request->version>=2; kv.coordinated=request->version==3;
				context->io.payloadBlocks=kv.payloadBlocks;
				context->io.persistentHostStages=kv.payloadBlocks;
			}
		}
		ready=ready && (context->runtimeCoreMode==8
			?PioneerCoordinatedMasterBoundary(context->io,context->baseline.baseline.baseline.baseline.dram,context->fleet,context->attention.ready)
			:PioneerFleetMasterBoundary(context->io,context->baseline.baseline.baseline.baseline.dram,context->fleet,context->attention.ready))
			&& context->io.MapAttention() && context->io.BulkMappingStable()
			&& PioneerAttentionTilePrepare(context->io,context->baseline.baseline.baseline.baseline.dram,
				context->baseline.baseline.baseline.noc,context->lease,context->attention,true)
			&& SelectResidentFleetEntry(*context,true);
		if (ready) ready=PioneerResidentAttentionRun(context->io,context->baseline.baseline.baseline.baseline.dram,
			context->baseline.baseline.baseline.noc,context->lease,context->attention,context->bulk,kv,
			context->bytes,request->tokens,request->layer,request->input,request->output,request->result);
		if (context->Unsafe()) {
			context->lease.recovery_required=true; context->io.QuarantineHostPage();
			PioneerQuarantineOutputBuffer(context->io,context->bulkHost); sColdRecoveryRequired=true;
			request->result.math.recovery_required=1;
		}
		if (!ready) memset(request->output,0,sizeof(request->output));
		status=user_memcpy(data,request,length);
	}
	release_sem(sLock); free(request); return status;
}

static status_t
CoordinatorProbeControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	if (!data || length!=sizeof(tt_pioneer_coordinator_probe)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	tt_pioneer_coordinator_probe request={};
	status=user_memcpy(&request,data,sizeof(request));
	if (status!=B_OK || request.version!=1 || !request.workers || request.workers>119
		|| request.reserved[0] || request.reserved[1]) return status!=B_OK?status:B_BAD_VALUE;
	status=AcquireProbeLock(cookie); if (status!=B_OK) return status;
	auto* context=cookie->context;
	if (!context || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if (context->runtimeCoreMode!=5 || !context->fleet || !context->fleetRoutes
		|| !context->compute.ready || !context->bulk.ready || !context->uploadedPages) status=B_NOT_ALLOWED;
	else {
		memset(&request.result,0,sizeof(request.result));
		auto& dram=context->baseline.baseline.baseline.baseline.dram;
		bool ready=PioneerPeerPcMasterHeld(context->io,dram,context->fleet,context->attention.ready)
			&& context->io.MapCoordinator() && PioneerCoordinatorPrepare(context->io,dram,
				context->baseline.baseline.baseline.noc,context->lease,context->coordinator);
		if (ready) PioneerCoordinatorProbe(context->io,dram,context->fleet,context->bulk,
			context->coordinator,context->attention.ready,request.workers,request.result);
		if (context->Unsafe()) {
			context->lease.recovery_required=true; context->io.QuarantineHostPage();
			PioneerQuarantineOutputBuffer(context->io,context->bulkHost); sColdRecoveryRequired=true;
			request.result.recovery_required=1;
		}
		status=user_memcpy(data,&request,sizeof(request));
	}
	release_sem(sLock); return status;
}

static status_t
PeerPcReadControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	if (!data || length!=sizeof(tt_pioneer_peer_pc_read)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	tt_pioneer_peer_pc_read request={};
	status=user_memcpy(&request,data,sizeof(request));
	if (status!=B_OK || request.version!=1 || !request.worker || request.worker>=TT_PIONEER_WORKERS
		|| request.reserved[0] || request.reserved[1]) return status!=B_OK?status:B_BAD_VALUE;
	status=AcquireProbeLock(cookie); if (status!=B_OK) return status;
	auto* context=cookie->context;
	if (!context || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if (context->runtimeCoreMode!=5 || !context->fleet || !context->fleetRoutes
		|| !context->compute.ready || !context->bulk.ready || !context->uploadedPages) status=B_NOT_ALLOWED;
	else {
		PioneerPeerPcRead(context->io,context->baseline.baseline.baseline.baseline.dram,
			context->fleet,context->bulk,context->attention.ready,request.worker,request.result);
		if (context->Unsafe()) {
			context->lease.recovery_required=true; context->io.QuarantineHostPage();
			PioneerQuarantineOutputBuffer(context->io,context->bulkHost); sColdRecoveryRequired=true;
			request.result.recovery_required=1;
		}
		status=user_memcpy(data,&request,sizeof(request));
	}
	release_sem(sLock); return status;
}

static status_t
FleetAttentionControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	const size_t fixed=sizeof(tt_pioneer_fleet_attention)-256*1024*sizeof(uint32_t);
	if (!data || length<fixed || length>sizeof(tt_pioneer_fleet_attention)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	auto* request=(tt_pioneer_fleet_attention*)malloc(length); if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,fixed);
	if (status!=B_OK || request->version!=1 || request->reserved[0] || request->reserved[1]
		|| length!=PioneerFleetAttentionBytes(request->tokens)) { free(request); return status!=B_OK?status:B_BAD_VALUE; }
	status=user_memcpy(request->prefix,(const uint8*)data+fixed,length-fixed);
	if (status!=B_OK) { free(request); return status; }
	for (unsigned group=0;group<8;group++) if (!PioneerAttentionTileInputValid(request->tokens,request->query+group*256,
		request->prefix+group*request->tokens*128,request->prefix+group*request->tokens*128+request->tokens*64)) { free(request); return B_BAD_VALUE; }
	status=AcquireProbeLock(cookie); if (status!=B_OK) { free(request); return status; }
	auto* context=cookie->context;
	if (!context || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if (context->runtimeCoreMode!=5 || context->residentAttention.reserved || !context->fleet || !context->compute.ready || !context->uploadedPages) status=B_NOT_ALLOWED;
	else {
		memset(request->result,0,sizeof(request->result)); memset(request->output,0,sizeof(request->output));
		bool ready=FleetHeld(*context) && context->io.MapAttention()
			&& PioneerAttentionTilePrepare(context->io,context->baseline.baseline.baseline.baseline.dram,
				context->baseline.baseline.baseline.noc,context->lease,context->attention)
			&& SelectFleetEntry(*context,true,true);
		for (unsigned group=0;group<8 && ready;group++) {
			ready=PioneerAttentionTileRun(context->io,context->baseline.baseline.baseline.baseline.dram,
				context->baseline.baseline.baseline.noc,context->lease,context->attention,request->tokens,
				request->query+group*256,request->prefix+group*request->tokens*128,
				request->prefix+group*request->tokens*128+request->tokens*64,request->output+group*256,request->result[group]);
		}
		if (context->Unsafe()) {
			context->lease.recovery_required=true; context->io.QuarantineHostPage(); sColdRecoveryRequired=true;
			for (auto& r:request->result) r.recovery_required=1;
		}
		if (!ready) memset(request->output,0,sizeof(request->output));
		status=user_memcpy(data,request,length);
	}
	release_sem(sLock); free(request); return status;
}

static status_t
AttentionTileTestControl(PioneerProbeCookie* cookie,void* data,size_t length,bool shared)
{
	if (data==NULL || length!=sizeof(tt_pioneer_attention_tile_test)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	auto* request=(tt_pioneer_attention_tile_test*)malloc(sizeof(tt_pioneer_attention_tile_test));
	if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,sizeof(*request));
	if (status!=B_OK || request->version!=1 || request->reserved[0] || request->reserved[1]
		|| !PioneerAttentionTileInputValid(request->tokens,request->query,request->key,request->value)) {
		free(request); return status!=B_OK?status:B_BAD_VALUE;
	}
	status=AcquireProbeLock(cookie); if (status!=B_OK) { free(request); return status; }
	PioneerStoreContext* context=cookie->context;
	if (!context || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if (shared?(context->runtimeCoreMode!=3 || !context->uploadedPages || !context->compute.ready || !context->secondCompute.ready)
		:(context->uploadedPages || (context->runtimeCoreMode!=-1 && context->runtimeCoreMode!=4)
			|| context->compute.saved || context->secondCompute.saved || context->secondLease.saved)) status=B_NOT_ALLOWED;
	else {
		if (!shared) context->runtimeCoreMode=4;
		request->result={}; request->result.version=1; request->result.state=TT_ATTN_TILE_CONFIG_FAILED;
		memset(request->output,0,sizeof(request->output));
		bool ready=context->lease.ready;
		if (!ready && !context->lease.saved)
			ready=PioneerResidentLeaseBegin(context->io,context->baseline.baseline.baseline.noc,context->lease);
		if (ready && shared) ready=PioneerRuntimeAttentionPeersHeld(context->io,context->baseline.baseline.baseline.baseline.dram,
			context->baseline.baseline.baseline.noc,context->secondNoc,context->lease,context->secondLease,
			context->compute,context->secondCompute,context->attention);
		ready=ready && context->io.MapAttention()
			&& PioneerAttentionTilePrepare(context->io,context->baseline.baseline.baseline.baseline.dram,
				context->baseline.baseline.baseline.noc,context->lease,context->attention);
		if (ready && shared) ready=PioneerRuntimeAttentionSelect(context->io,context->baseline.baseline.baseline.baseline.dram,
			context->baseline.baseline.baseline.noc,context->secondNoc,context->lease,context->secondLease,
			context->compute,context->secondCompute,context->attention,true);
		if (ready) PioneerAttentionTileRun(context->io,context->baseline.baseline.baseline.baseline.dram,
			context->baseline.baseline.baseline.noc,context->lease,context->attention,
			request->tokens,request->query,request->key,request->value,request->output,request->result);
		if (context->Unsafe()) {
			context->lease.recovery_required=true; context->io.QuarantineHostPage();
			request->result.recovery_required=1; sColdRecoveryRequired=true;
		}
		status=user_memcpy(data,request,sizeof(*request));
	}
	release_sem(sLock); free(request); return status;
}

#ifdef TT_PIONEER_DRIVER_RELOAD
// PIONEER_DRIVER_RELOAD_CONTROL_BEGIN
static status_t
DriverReloadControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	if (!data || length!=sizeof(tt_pioneer_driver_reload)
		|| (operation!=TT_PCI_PROBE_RELOAD_STATUS && operation!=TT_PCI_PROBE_RELOAD_QUIESCE)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	tt_pioneer_driver_reload r={}; status=user_memcpy(&r,data,sizeof(r));
	if (status!=B_OK || !PioneerDriverReloadRequest(r)) return status!=B_OK?status:B_BAD_VALUE;
	status=acquire_sem(sLock); if (status!=B_OK) return status;
	if (!sReloadGuard || sReloadGuard->version!=1 || !sReloadGeneration) status=B_DEV_NOT_READY;
	else if ((r.expected_tag && r.expected_tag!=PIONEER_DRIVER_BUILD_TAG)
		|| (r.expected_generation && r.expected_generation!=sReloadGeneration)) status=B_BAD_VALUE;
	else if (operation==TT_PCI_PROBE_RELOAD_QUIESCE &&
		(!r.expected_tag || !r.expected_generation || sColdRecoveryRequired || sRuntimeOwner
			|| cookie->context || sOpenCookies!=1)) status=B_BUSY;
	else {
		if (operation==TT_PCI_PROBE_RELOAD_QUIESCE) sReloadQuiesced=true;
		r={}; r.version=1; r.driver=TT_Q38_DISPATCH_DRIVER; r.build_tag=PIONEER_DRIVER_BUILD_TAG;
		r.generation=sReloadGeneration; r.guard_version=sReloadGuard->version;
		r.quiesced=sReloadQuiesced; r.cold=sColdRecoveryRequired; r.active_owner=sRuntimeOwner!=NULL;
		r.open_count=sOpenCookies;
		// Failed reply delivery leaves the hardware gate CLOSED. It never
		// resumes computation or clears the resident cold latch.
		status=user_memcpy(data,&r,sizeof(r));
	}
	release_sem(sLock); return status;
}
// PIONEER_DRIVER_RELOAD_CONTROL_END
#endif

static status_t
ProbeControl(void* cookie, uint32 operation, void* data, size_t length)
{
	PioneerProbeCookie* instance=(PioneerProbeCookie*)cookie;
	if (geteuid() != 0 || instance==NULL || instance->magic!=0x50313030)
		return B_NOT_ALLOWED;
#ifdef TT_PIONEER_DRIVER_RELOAD
	if (operation==TT_PCI_PROBE_RELOAD_STATUS || operation==TT_PCI_PROBE_RELOAD_QUIESCE)
		return DriverReloadControl(instance,operation,data,length);
#endif
#ifdef PIONEER_Q38_GDN_ZERO_DIAGNOSTIC
	if (operation==TT_PCI_PROBE_Q38_GDN_ZERO_SUPPORT || operation==TT_PCI_PROBE_Q38_GDN_ZERO_INITIALIZE)
		return Q38GdnZeroControl(instance,operation,data,length);
#endif
#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
	if (operation>=TT_PCI_PROBE_Q38_RGDN_ROOT_SUPPORT && operation<=TT_PCI_PROBE_Q38_RGDN_ROOT_END)
		return Q38RgdnRootControl(instance,operation,data,length);
#endif
#ifdef PIONEER_Q38_LOCAL_DELTA_PROBE
	if (operation>=TT_PCI_PROBE_Q38_DELTA_SUPPORT && operation<=TT_PCI_PROBE_Q38_DELTA_ACCEPT)
		return Q38DeltaControl(instance,operation,data,length);
#endif
#ifdef PIONEER_Q38_SPAN_PRELOAD
	if (operation>=TT_PCI_PROBE_Q38_SPAN_SUPPORT && operation<=
#ifdef PIONEER_Q38_SPAN_FAST
		TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD
#else
		TT_PCI_PROBE_Q38_SPAN_END
#endif
		)
		return Q38SpanControl(instance,operation,data,length);
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
#ifdef TT_PIONEER_Q38_WORK_MODE
	if (operation==TT_PCI_PROBE_Q38_WORK_SUPPORT || operation==TT_PCI_PROBE_Q38_WORK_RUN)
		return Q38DispatchControl(instance,operation,data,length);
#endif
	if (operation==TT_PCI_PROBE_Q38_DISPATCH_SUPPORT || operation==TT_PCI_PROBE_Q38_DISPATCH_RUN)
		return Q38DispatchControl(instance,operation,data,length);
	if (operation==TT_PCI_PROBE_Q38_COHORT_SUPPORT || operation==TT_PCI_PROBE_Q38_COHORT_RUN
		|| operation==TT_PCI_PROBE_Q38_PANEL_RUN || operation==TT_PCI_PROBE_Q38_STREAM_RUN) return B_NOT_ALLOWED;
#else
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT
	if (operation==TT_PCI_PROBE_Q38_COHORT_SUPPORT || operation==TT_PCI_PROBE_Q38_COHORT_RUN)
		return Q38CohortControl(instance,operation,data,length);
#endif
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_STREAM
	if (operation==TT_PCI_PROBE_Q38_STREAM_SUPPORT || operation==TT_PCI_PROBE_Q38_STREAM_RUN)
		return Q38StreamControl(instance,operation,data,length);
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_BANKS
	if (operation>=TT_PCI_PROBE_Q38_BANK_SUPPORT && operation<=TT_PCI_PROBE_Q38_BANK_BULK_UPLOAD)
		return Q38BankControl(instance,operation,data,length);
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_PANEL
	if (operation==TT_PCI_PROBE_Q38_PANEL_SUPPORT || operation==TT_PCI_PROBE_Q38_PANEL_RUN)
		return Q38PanelControl(instance,operation,data,length);
#endif
	if (operation==TT_PCI_PROBE_PIONEER_SESSION_BEGIN || operation==TT_PCI_PROBE_PIONEER_SESSION_END
		|| operation==TT_PCI_PROBE_PIONEER_SESSION_STATUS || operation==TT_PCI_PROBE_PIONEER_SESSION_SUPPORT)
		return ResidentSessionControl(instance,operation,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_STORE_UPLOAD || operation==TT_PCI_PROBE_PIONEER_STORE_READ)
		return ResidentStoreControl(instance,operation,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_DMA
		|| operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_FP16 || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_INT8
		|| operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8 || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_COMPACT
		|| operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_NATIVE || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_NATIVE_F32_SCALES || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_BULK
		|| operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_SECOND || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_DUAL || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_DUAL_REUSE) {
		bool reuse=operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_DUAL_REUSE;
		bool dual=reuse || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_DUAL;
		bool second=dual || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_SECOND;
		bool bulk=second || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_BULK;
		bool f32Scales=bulk || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_NATIVE_F32_SCALES;
		bool native=f32Scales || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_NATIVE;
		bool compact=native || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_COMPACT;
		bool q8=compact || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8;
		return RuntimeMatvecControl(instance,data,length,operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_DMA,
			operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_FP16,q8 || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_INT8,q8,compact,native,f32Scales,bulk,second,dual,reuse);
	}
	if (operation==TT_PCI_PROBE_PIONEER_RUNTIME_PROFILE)
		return RuntimeProfileControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_RUNTIME_Q8_FLEET)
		return RuntimeFleetControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_COORDINATED_FLEET)
		return CoordinatedFleetControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_LAYER_FFN)
		return LayerFfnControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_LAYER_GLU)
		return LayerGluControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_LAYER_WIDE)
		return LayerWideControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_LAYER_NARROW)
		return LayerWideControl(instance,data,length,true);
	if (operation==TT_PCI_PROBE_PIONEER_DECODER_MODEL)
		return DecoderModelControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_DECODER_RUN)
		return DecoderRunControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_DECODER_GREEDY)
		return DecoderGreedyControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_DECODER_BATCH)
		return DecoderBatchControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_DECODER_STAGES)
		return DecoderStageProfileControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_LAYER_SPAN)
		return LayerSpanControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_COORDINATED_SUPPORT)
		return CoordinatedSupportControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_RUNTIME_PERSISTENT_FLEET)
		return RuntimePersistentFleetControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_FLEET_ATTENTION)
		return FleetAttentionControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_RESIDENT_ATTENTION)
		return ResidentAttentionControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_PEER_PC_READ)
		return PeerPcReadControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_COORDINATOR_PROBE)
		return CoordinatorProbeControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_ATTENTION_TILE_TEST || operation==TT_PCI_PROBE_PIONEER_ATTENTION_TILE_SHARED)
		return AttentionTileTestControl(instance,data,length,operation==TT_PCI_PROBE_PIONEER_ATTENTION_TILE_SHARED);
	if (operation==TT_PCI_PROBE_PIONEER_STREAM_UPLOAD)
		return ResidentStreamControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_BULK_UPLOAD)
		return BulkUploadControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_HOST_WRITE_SUPPORT || operation==TT_PCI_PROBE_PIONEER_HOST_WRITE_PAGE_SUPPORT) {
		if (data==NULL || length!=sizeof(uint32)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		release_sem(sLock); const uint32 version=1;
		return user_memcpy(data,&version,sizeof(version));
	}
	if (operation==TT_PCI_PROBE_PIONEER_HOST_WRITE || operation==TT_PCI_PROBE_PIONEER_HOST_WRITE_PAGE) {
		if (data==NULL || length!=sizeof(tt_pioneer_host_write_result)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		status=sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		if (status!=B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager==NULL) status=get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
		tt_pioneer_profile profile={};
		if (status==B_OK) status=GetPioneerPciProfile(sDeviceManager,sCard,profile);
		profile.status=status;
		tt_pioneer_host_write_result result={};
		tt_pioneer_dram_samples_result& baseline=result.baseline.baseline.baseline.baseline;
		tt_pioneer_arc_result& control=baseline.control.control.control;
		tt_arc_telemetry_result& telemetry=baseline.control.control.telemetry;
		tt_tensix_reset_result& tensix=baseline.control.tensix;
		control.pci.version=control.arc.version=telemetry.version=tensix.version=baseline.dram.version=1;
		control.arc.tlb_restored=telemetry.tlb_restored=tensix.tlb_restored=1;
		baseline.dram.data_restored=baseline.dram.tlb_restored=result.baseline.baseline.baseline.noc.tlb_restored=1;
		result.baseline.baseline.path.tlb_restored=result.baseline.audit.tlb_restored=1;
		tt_host_read_result& transfer=result.write.transfer;
		transfer.bme_restored=transfer.l1_restored=transfer.niu_restored=transfer.tlb_restored=1;
		if (status==B_OK) {
			PioneerHostWriteIO io(profile,control.arc,telemetry,tensix,baseline.dram,
				result.baseline.baseline.baseline.noc,result.baseline.baseline.path,result.baseline.audit,result.write,
				operation==TT_PCI_PROBE_PIONEER_HOST_WRITE_PAGE?4096:64);
			BlackholePioneerMmioProbe(io,control.pci);
		}
		if (transfer.recovery_required || result.baseline.audit.recovery_required
			|| result.baseline.baseline.path.recovery_required || result.baseline.baseline.baseline.noc.recovery_required
			|| baseline.dram.recovery_required || control.pci.state==TT_PIONEER_MMIO_RESTORE_FAILED) {
			transfer.recovery_required=1; sColdRecoveryRequired=true;
		}
		sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		release_sem(sLock); return user_memcpy(data,&result,sizeof(result));
	}
	if (operation==TT_PCI_PROBE_PIONEER_HOST_READ_SUPPORT || operation==TT_PCI_PROBE_PIONEER_HOST_READ_PAGE_SUPPORT) {
		if (data==NULL || length!=sizeof(uint32)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		release_sem(sLock); const uint32 version=1;
		return user_memcpy(data,&version,sizeof(version));
	}
	if (operation==TT_PCI_PROBE_PIONEER_HOST_READ || operation==TT_PCI_PROBE_PIONEER_HOST_READ_PAGE) {
		if (data==NULL || length!=sizeof(tt_pioneer_host_read_result)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		status=sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		if (status!=B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager==NULL) status=get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
		tt_pioneer_profile profile={};
		if (status==B_OK) status=GetPioneerPciProfile(sDeviceManager,sCard,profile);
		profile.status=status;
		tt_pioneer_host_read_result result={};
		tt_pioneer_dram_samples_result& baseline=result.baseline.baseline.baseline.baseline;
		tt_pioneer_arc_result& control=baseline.control.control.control;
		tt_arc_telemetry_result& telemetry=baseline.control.control.telemetry;
		tt_tensix_reset_result& tensix=baseline.control.tensix;
		control.pci.version=control.arc.version=telemetry.version=tensix.version=baseline.dram.version=1;
		control.arc.tlb_restored=telemetry.tlb_restored=tensix.tlb_restored=1;
		baseline.dram.data_restored=baseline.dram.tlb_restored=result.baseline.baseline.baseline.noc.tlb_restored=1;
		result.baseline.baseline.path.tlb_restored=result.baseline.audit.tlb_restored=1;
		result.read.bme_restored=result.read.l1_restored=result.read.niu_restored=result.read.tlb_restored=1;
		if (status==B_OK) {
			PioneerHostReadIO io(profile,control.arc,telemetry,tensix,baseline.dram,
				result.baseline.baseline.baseline.noc,result.baseline.baseline.path,result.baseline.audit,result.read,
				operation==TT_PCI_PROBE_PIONEER_HOST_READ_PAGE?4096:64);
			BlackholePioneerMmioProbe(io,control.pci);
		}
		if (result.read.recovery_required || result.baseline.audit.recovery_required
			|| result.baseline.baseline.path.recovery_required || result.baseline.baseline.baseline.noc.recovery_required
			|| baseline.dram.recovery_required || control.pci.state==TT_PIONEER_MMIO_RESTORE_FAILED) {
			result.read.recovery_required=1; sColdRecoveryRequired=true;
		}
		sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		release_sem(sLock); return user_memcpy(data,&result,sizeof(result));
	}
	if (operation==TT_PCI_PROBE_PIONEER_HOST_DMA_AUDIT_SUPPORT) {
		if (data==NULL || length!=sizeof(uint32)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		release_sem(sLock); const uint32 version=1;
		return user_memcpy(data,&version,sizeof(version));
	}
	if (operation==TT_PCI_PROBE_PIONEER_HOST_DMA_AUDIT) {
		if (data==NULL || length!=sizeof(tt_pioneer_host_dma_audit_result)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		status=sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		if (status!=B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager==NULL) status=get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
		tt_pioneer_profile profile={};
		if (status==B_OK) status=GetPioneerPciProfile(sDeviceManager,sCard,profile);
		profile.status=status;
		tt_pioneer_host_dma_audit_result result={};
		tt_pioneer_dram_samples_result& baseline=result.baseline.baseline.baseline;
		tt_pioneer_arc_result& control=baseline.control.control.control;
		tt_arc_telemetry_result& telemetry=baseline.control.control.telemetry;
		tt_tensix_reset_result& tensix=baseline.control.tensix;
		control.pci.version=control.arc.version=telemetry.version=tensix.version=baseline.dram.version=1;
		control.arc.tlb_restored=telemetry.tlb_restored=tensix.tlb_restored=1;
		baseline.dram.data_restored=baseline.dram.tlb_restored=result.baseline.baseline.noc.tlb_restored=1;
		result.baseline.path.tlb_restored=result.audit.tlb_restored=1;
		if (status==B_OK) {
			PioneerHostDmaAuditIO io(profile,control.arc,telemetry,tensix,baseline.dram,
				result.baseline.baseline.noc,result.baseline.path,result.audit);
			BlackholePioneerMmioProbe(io,control.pci);
		}
		if (result.audit.recovery_required || result.baseline.path.recovery_required
			|| result.baseline.baseline.noc.recovery_required || baseline.dram.recovery_required
			|| control.pci.state==TT_PIONEER_MMIO_RESTORE_FAILED) {
			result.audit.recovery_required=1; sColdRecoveryRequired=true;
		}
		sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		release_sem(sLock); return user_memcpy(data,&result,sizeof(result));
	}
	if (operation==TT_PCI_PROBE_PIONEER_OUTPUT_BUFFER_SUPPORT) {
		if (data==NULL || length!=sizeof(uint32)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		release_sem(sLock); const uint32 version=1;
		return user_memcpy(data,&version,sizeof(version));
	}
	if (operation==TT_PCI_PROBE_PIONEER_OUTPUT_BUFFER_PROBE) {
		if (data==NULL || length!=sizeof(tt_output_buffer_probe_result)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		tt_output_buffer_probe_result result={};
		PioneerOutputBufferProbeIO io;
		PioneerOutputBufferProbe(io,result); // CPU-only, no physical address escapes to user/device
		release_sem(sLock); return user_memcpy(data,&result,sizeof(result));
	}
	if (operation==TT_PCI_PROBE_PIONEER_HOST_PAGE_SUPPORT) {
		if (data==NULL || length!=sizeof(uint32)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		release_sem(sLock); const uint32 version=1;
		return user_memcpy(data,&version,sizeof(version));
	}
	if (operation==TT_PCI_PROBE_PIONEER_HOST_PAGE) {
		if (data==NULL || length!=sizeof(tt_host_page_result)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		tt_host_page_result result={};
		PioneerHostPageIO io;
		PioneerHostPageProbe(io,result); // CPU-only: never calls PCI/MMIO
		release_sem(sLock); return user_memcpy(data,&result,sizeof(result));
	}
	if (operation==TT_PCI_PROBE_PIONEER_HOST_PATH_SUPPORT) {
		if (data==NULL || length!=sizeof(uint32)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		release_sem(sLock); const uint32 version=1;
		return user_memcpy(data,&version,sizeof(version));
	}
	if (operation==TT_PCI_PROBE_PIONEER_HOST_PATH) {
		if (data==NULL || length!=sizeof(tt_pioneer_host_path_result)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		status=sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		if (status!=B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager==NULL) status=get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
		tt_pioneer_profile profile={};
		if (status==B_OK) status=GetPioneerPciProfile(sDeviceManager,sCard,profile);
		profile.status=status;
		tt_pioneer_host_path_result result={};
		tt_pioneer_dram_samples_result& baseline=result.baseline.baseline;
		tt_pioneer_arc_result& control=baseline.control.control.control;
		tt_arc_telemetry_result& telemetry=baseline.control.control.telemetry;
		tt_tensix_reset_result& tensix=baseline.control.tensix;
		control.pci.version=control.arc.version=telemetry.version=tensix.version=baseline.dram.version=1;
		control.arc.tlb_restored=telemetry.tlb_restored=tensix.tlb_restored=1;
		baseline.dram.data_restored=baseline.dram.tlb_restored=result.baseline.noc.tlb_restored=result.path.tlb_restored=1;
		if (status==B_OK) {
			PioneerHostPathIO io(profile,control.arc,telemetry,tensix,baseline.dram,result.baseline.noc,result.path);
			BlackholePioneerMmioProbe(io,control.pci);
		}
		if (result.path.recovery_required || result.baseline.noc.recovery_required || baseline.dram.recovery_required
			|| control.pci.state==TT_PIONEER_MMIO_RESTORE_FAILED) {
			result.path.recovery_required=1; sColdRecoveryRequired=true;
		}
		sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		release_sem(sLock); return user_memcpy(data,&result,sizeof(result));
	}
	if (operation==TT_PCI_PROBE_PIONEER_RESIDENT_GEMV_SUPPORT) {
		if (data==NULL || length!=sizeof(uint32)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		release_sem(sLock); const uint32 version=1;
		return user_memcpy(data,&version,sizeof(version));
	}
	if (operation==TT_PCI_PROBE_PIONEER_RESIDENT_GEMV) {
		if (data==NULL || length!=sizeof(tt_pioneer_resident_gemv_result)) return B_BAD_VALUE;
		struct { uint32 version,inputs; } request={};
		status_t status=user_memcpy(&request,data,sizeof(request));
		if (status!=B_OK) return status;
		if (request.version!=1 || !BlackholeResidentGemvShape(request.inputs)) return B_BAD_VALUE;
		status=AcquireProbeLock(); if (status!=B_OK) return status;
		status=sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		if (status!=B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager==NULL) status=get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
		tt_pioneer_profile profile={};
		if (status==B_OK) status=GetPioneerPciProfile(sDeviceManager,sCard,profile);
		profile.status=status;
		tt_pioneer_resident_gemv_result reply={}; reply.version=1; reply.inputs=request.inputs;
		tt_pioneer_resident_math_result& result=reply.result;
		tt_pioneer_dram_samples_result& baseline=result.baseline.baseline;
		tt_pioneer_arc_result& control=baseline.control.control.control;
		tt_arc_telemetry_result& telemetry=baseline.control.control.telemetry;
		tt_tensix_reset_result& tensix=baseline.control.tensix;
		control.pci.version=control.arc.version=telemetry.version=tensix.version=baseline.dram.version=1;
		control.arc.tlb_restored=telemetry.tlb_restored=tensix.tlb_restored=1;
		baseline.dram.data_restored=baseline.dram.tlb_restored=result.baseline.noc.tlb_restored=1;
		result.compute.gddr_restored=result.compute.niu_restored=1;
		result.compute.math.l1_restored=result.compute.math.tlb_restored=1;
		if (status==B_OK) {
			PioneerResidentMathIO io(profile,control.arc,telemetry,tensix,baseline.dram,result.baseline.noc,result.compute,request.inputs);
			BlackholePioneerMmioProbe(io,control.pci);
		}
		if (result.compute.recovery_required || result.compute.math.recovery_required || result.baseline.noc.recovery_required
			|| baseline.dram.recovery_required || control.pci.state==TT_PIONEER_MMIO_RESTORE_FAILED) {
			result.compute.recovery_required=1; sColdRecoveryRequired=true;
		}
		sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		release_sem(sLock); return user_memcpy(data,&reply,sizeof(reply));
	}
	if (operation==TT_PCI_PROBE_PIONEER_RESIDENT_MATH_SUPPORT) {
		if (data==NULL || length!=sizeof(uint32)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		release_sem(sLock); const uint32 version=1;
		return user_memcpy(data,&version,sizeof(version));
	}
	if (operation==TT_PCI_PROBE_PIONEER_RESIDENT_MATH) {
		if (data==NULL || length!=sizeof(tt_pioneer_resident_math_result)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		status=sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		if (status!=B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager==NULL) status=get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
		tt_pioneer_profile profile={};
		if (status==B_OK) status=GetPioneerPciProfile(sDeviceManager,sCard,profile);
		profile.status=status;
		tt_pioneer_resident_math_result result={};
		tt_pioneer_dram_samples_result& baseline=result.baseline.baseline;
		tt_pioneer_arc_result& control=baseline.control.control.control;
		tt_arc_telemetry_result& telemetry=baseline.control.control.telemetry;
		tt_tensix_reset_result& tensix=baseline.control.tensix;
		control.pci.version=control.arc.version=telemetry.version=tensix.version=baseline.dram.version=1;
		control.arc.tlb_restored=telemetry.tlb_restored=tensix.tlb_restored=1;
		baseline.dram.data_restored=baseline.dram.tlb_restored=result.baseline.noc.tlb_restored=1;
		result.compute.gddr_restored=result.compute.niu_restored=1;
		result.compute.math.l1_restored=result.compute.math.tlb_restored=1;
		if (status==B_OK) {
			PioneerResidentMathIO io(profile,control.arc,telemetry,tensix,baseline.dram,result.baseline.noc,result.compute);
			BlackholePioneerMmioProbe(io,control.pci);
		}
		if (result.compute.recovery_required || result.compute.math.recovery_required || result.baseline.noc.recovery_required
			|| baseline.dram.recovery_required || control.pci.state==TT_PIONEER_MMIO_RESTORE_FAILED) {
			result.compute.recovery_required=1; sColdRecoveryRequired=true;
		}
		sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		release_sem(sLock); return user_memcpy(data,&result,sizeof(result));
	}
	if (operation==TT_PCI_PROBE_PIONEER_RESIDENT_READ_SUPPORT) {
		if (data==NULL || length!=sizeof(uint32)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		release_sem(sLock); const uint32 version=1;
		return user_memcpy(data,&version,sizeof(version));
	}
	if (operation==TT_PCI_PROBE_PIONEER_RESIDENT_READ) {
		if (data==NULL || length!=sizeof(tt_pioneer_resident_read_result)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		status=sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		if (status!=B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager==NULL) status=get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
		tt_pioneer_profile profile={};
		if (status==B_OK) status=GetPioneerPciProfile(sDeviceManager,sCard,profile);
		profile.status=status;
		tt_pioneer_resident_read_result result={};
		tt_pioneer_dram_samples_result& baseline=result.baseline.baseline;
		tt_pioneer_arc_result& control=baseline.control.control.control;
		tt_arc_telemetry_result& telemetry=baseline.control.control.telemetry;
		tt_tensix_reset_result& tensix=baseline.control.tensix;
		control.pci.version=control.arc.version=telemetry.version=tensix.version=baseline.dram.version=1;
		control.arc.tlb_restored=telemetry.tlb_restored=tensix.tlb_restored=1;
		baseline.dram.data_restored=baseline.dram.tlb_restored=result.baseline.noc.tlb_restored=1;
		result.read.l1_restored=result.read.niu_restored=result.read.tlb_restored=1;
		if (status==B_OK) {
			PioneerResidentReadIO io(profile,control.arc,telemetry,tensix,baseline.dram,result.baseline.noc,result.read);
			BlackholePioneerMmioProbe(io,control.pci);
		}
		if (result.read.recovery_required || result.baseline.noc.recovery_required
			|| baseline.dram.recovery_required || control.pci.state==TT_PIONEER_MMIO_RESTORE_FAILED) {
			result.read.recovery_required=1; sColdRecoveryRequired=true;
		}
		sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		release_sem(sLock);
		return user_memcpy(data,&result,sizeof(result));
	}
	if (operation==TT_PCI_PROBE_PIONEER_FLEET_NOC_SUPPORT) {
		if (!data || length!=sizeof(uint32)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		release_sem(sLock); const uint32 workers=TT_PIONEER_WORKERS;
		return user_memcpy(data,&workers,sizeof(workers));
	}
	if (operation==TT_PCI_PROBE_PIONEER_FLEET_NOC) {
		if (!data || length!=sizeof(tt_pioneer_fleet_noc_result)) return B_BAD_VALUE;
		thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
		if (status!=B_OK) return status;
		if (thread.team!=instance->team) return B_NOT_ALLOWED;
		auto* result=(tt_pioneer_fleet_noc_result*)calloc(1,sizeof(tt_pioneer_fleet_noc_result));
		if (!result) return B_NO_MEMORY;
		status=AcquireProbeLock(); if (status!=B_OK) { free(result); return status; }
		status=sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		if (status!=B_OK) { release_sem(sLock); free(result); return status; }
		if (!sDeviceManager) status=get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
		tt_pioneer_profile profile={};
		if (status==B_OK) status=GetPioneerPciProfile(sDeviceManager,sCard,profile);
		profile.status=status;
		auto& control=result->baseline.control.control.control;
		control.pci.version=control.arc.version=result->baseline.control.control.telemetry.version=1;
		control.arc.tlb_restored=result->baseline.control.control.telemetry.tlb_restored=1;
		result->baseline.control.tensix.version=result->baseline.control.tensix.tlb_restored=1;
		result->baseline.dram.version=result->baseline.dram.data_restored=result->baseline.dram.tlb_restored=1;
		if (status==B_OK) { PioneerFleetNocIO io(profile,*result); BlackholePioneerMmioProbe(io,control.pci); }
		if (result->baseline.dram.recovery_required || control.pci.state==TT_PIONEER_MMIO_RESTORE_FAILED)
			result->recovery_required=1;
		if (result->recovery_required) sColdRecoveryRequired=true;
		sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		status=user_memcpy(data,result,sizeof(*result));
		release_sem(sLock); free(result); return status;
	}
	if (operation==TT_PCI_PROBE_PIONEER_COMPUTE_NOC_SUPPORT || operation==TT_PCI_PROBE_PIONEER_SECOND_COMPUTE_NOC_SUPPORT) {
		if (data==NULL || length!=sizeof(uint32)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		release_sem(sLock); const uint32 version=operation==TT_PCI_PROBE_PIONEER_SECOND_COMPUTE_NOC_SUPPORT?2:1;
		return user_memcpy(data,&version,sizeof(version));
	}
	if (operation==TT_PCI_PROBE_PIONEER_COMPUTE_NOC || operation==TT_PCI_PROBE_PIONEER_SECOND_COMPUTE_NOC) {
		if (data==NULL || length!=sizeof(tt_pioneer_compute_noc_result)) return B_BAD_VALUE;
		bool second=operation==TT_PCI_PROBE_PIONEER_SECOND_COMPUTE_NOC;
		if (second) {
			thread_info thread={}; status_t caller=get_thread_info(find_thread(NULL),&thread);
			if (caller!=B_OK) return caller;
			if (thread.team!=instance->team) return B_NOT_ALLOWED;
		}
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		status=sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		if (status!=B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager==NULL) status=get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
		tt_pioneer_profile profile={};
		if (status==B_OK) status=GetPioneerPciProfile(sDeviceManager,sCard,profile);
		profile.status=status;
		tt_pioneer_compute_noc_result result={};
		tt_pioneer_arc_result& control=result.baseline.control.control.control;
		tt_arc_telemetry_result& telemetry=result.baseline.control.control.telemetry;
		tt_tensix_reset_result& tensix=result.baseline.control.tensix;
		control.pci.version=control.arc.version=telemetry.version=tensix.version=result.baseline.dram.version=1;
		control.arc.tlb_restored=telemetry.tlb_restored=tensix.tlb_restored=1;
		result.baseline.dram.data_restored=result.baseline.dram.tlb_restored=result.noc.tlb_restored=1;
		if (status==B_OK) {
			PioneerComputeNocIO io(profile,control.arc,telemetry,tensix,result.baseline.dram,result.noc,second?3:2);
			BlackholePioneerMmioProbe(io,control.pci);
		}
		if (result.noc.recovery_required || result.baseline.dram.recovery_required
			|| control.pci.state==TT_PIONEER_MMIO_RESTORE_FAILED) {
			result.noc.recovery_required=1; sColdRecoveryRequired=true;
		}
		sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		release_sem(sLock);
		return user_memcpy(data,&result,sizeof(result));
	}
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
		|| operation == TT_PCI_PROBE_PIONEER_IDENTITY_MATMUL_SUPPORT || operation == TT_PCI_PROBE_PIONEER_DENSE_MATMUL_SUPPORT || operation == TT_PCI_PROBE_PIONEER_DENSE_SRCA_SUPPORT || operation == TT_PCI_PROBE_PIONEER_PATTERN_SRCB_SUPPORT || operation == TT_PCI_PROBE_PIONEER_DATA_TILE_SUPPORT || operation == TT_PCI_PROBE_PIONEER_DATA_BATCH_SUPPORT) {
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
		if (operation == TT_PCI_PROBE_PIONEER_DATA_BATCH_SUPPORT) version = 40;
		unsigned stage = version >= 31 ? version-15 : 0;
		const tt_matmul_support support = {version,BlackholeTensixCodeWords(stage),BlackholeTensixImageWords(stage),0x6000,0x70000,BlackholeTensixParkPc(stage),0x17f800,0x87ffff,0x46800};
		release_sem(sLock);
		return user_memcpy(data,&support,sizeof(support));
	}
	if (operation == TT_PCI_PROBE_PIONEER_DATA_BATCH) {
		if (data == NULL || length != sizeof(tt_pioneer_data_batch)) return B_BAD_VALUE;
		struct Transaction {
			tt_pioneer_data_batch* p;
			Transaction() : p((tt_pioneer_data_batch*)calloc(1,sizeof(tt_pioneer_data_batch))) {}
			~Transaction() { free(p); }
		} transaction;
		if (transaction.p == NULL) return B_NO_MEMORY;
		status_t status = user_memcpy(transaction.p,data,offsetof(tt_pioneer_data_batch,output));
		if (status != B_OK) return status;
		if (transaction.p->version != 1 || transaction.p->count == 0
			|| transaction.p->count > TT_DATA_TILE_BATCH_MAX || transaction.p->completed != 0
			|| transaction.p->reserved != 0) return B_BAD_VALUE;
		for (unsigned i = 0; i < transaction.p->count; i++)
			if (!BlackholeTileDataValid(transaction.p->input[i])) return B_BAD_VALUE;
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
				false,NULL,24,transaction.p->input,&transaction.p->output[0][0],
				transaction.p->count,&transaction.p->completed);
			BlackholePioneerMmioProbe(io,control.pci);
		}
		if (result.math.recovery_required || result.baseline.dram.recovery_required || control.pci.state == TT_PIONEER_MMIO_RESTORE_FAILED) {
			result.math.recovery_required = 1; sColdRecoveryRequired = true;
		}
		sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		release_sem(sLock);
		if (transaction.p->completed != transaction.p->count || result.math.state != TT_MATMUL_DATA_EXECUTED
			|| !result.math.completed || !result.math.stopped || !result.math.l1_restored
			|| !result.math.tlb_restored || !control.pci.restored || result.math.recovery_required)
			memset(transaction.p->output,0,sizeof(transaction.p->output));
		return user_memcpy(data,transaction.p,sizeof(tt_pioneer_data_batch));
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
