/* SPDX-License-Identifier: MIT */
#include "PioneerDriverReloadGuard.h"
#include "PioneerDriverRecoveryGuard.h"
#include "PioneerDriverReloadState.h"
#include <KernelExport.h>
// B_KEEP_LOADED is essential:reloading the legacy driver must NEVER erase
// a quarantined card's cold-recovery latch or its retained host ownership.
static PioneerDriverReloadState sState;
static sem_id sStateLock=-1;
static int32 sForcedCold=0;
static int32 sFaultEpoch=0;
static bool Cold() { return atomic_get(&sForcedCold)!=0; }
static status_t Attach(uint64* generation)
{
	if (!generation) return B_BAD_VALUE;
	*generation=0;
	status_t status=acquire_sem(sStateLock); if (status!=B_OK) return status;
	bool okay=!Cold() && sState.Attach(*generation);
	release_sem(sStateLock); return okay?B_OK:B_DEV_NOT_READY;
}
static status_t Detach(uint64 generation,bool clean)
{
	status_t status=acquire_sem(sStateLock);
	if (status!=B_OK) { atomic_or(&sForcedCold,1); return status; }
	bool okay=sState.Detach(generation,clean && !Cold());
	if (!okay) atomic_or(&sForcedCold,1);
	release_sem(sStateLock); return okay?B_OK:B_DEV_NOT_READY;
}
static void Fault(uint64)
{
	// No wait,allocation,PCI/MMIO or cleanup after uncertainty.
	atomic_add(&sFaultEpoch,1);
	atomic_or(&sForcedCold,1);
}

static status_t BeginRecovery(uint64 expectedGeneration,uint64* ticket)
{
	if (!ticket) return B_BAD_VALUE;
	*ticket=0;
	status_t status=acquire_sem(sStateLock); if (status!=B_OK) return status;
	if (!Cold() || !sState.attached || expectedGeneration!=sState.generation)
		status=B_DEV_NOT_READY;
	else {
		// Fault() is intentionally lock-free. Mirror its atomic cold latch only
		// after observing it under the state lock, and capture its epoch.
		sState.Fault(expectedGeneration);
		bool okay=sState.BeginRecovery(expectedGeneration,
			(uint32)atomic_get(&sFaultEpoch),*ticket);
		if (!okay) status=B_BUSY;
	}
	release_sem(sStateLock);
	return status;
}

static status_t CompleteRecovery(uint64 ticket,
	const PioneerDriverRecoveryProof* proof,uint64* newGeneration)
{
	if (!ticket || !proof || !newGeneration) return B_BAD_VALUE;
	*newGeneration=0;
	status_t status=acquire_sem(sStateLock); if (status!=B_OK) return status;
	uint32 begunFaultEpoch=0;
	uint64 generation=0;
	bool okay=Cold() && sState.CompleteRecovery(ticket,
		(uint32)atomic_get(&sFaultEpoch),*proof,begunFaultEpoch,generation);
	if (okay) {
		// Clear only the latch covered by this ticket. Fault() publishes its
		// epoch before its latch, so the second epoch check catches a concurrent
		// fault on either side of the clear without losing the quarantine.
		atomic_and(&sForcedCold,~1);
		if ((uint32)atomic_get(&sFaultEpoch)!=begunFaultEpoch) {
			sState.Fault(generation);
			atomic_or(&sForcedCold,1);
			okay=false;
		}
	}
	if (okay) *newGeneration=generation;
	release_sem(sStateLock);
	return okay?B_OK:B_DEV_NOT_READY;
}

static status_t AbortRecovery(uint64 ticket)
{
	status_t status=acquire_sem(sStateLock); if (status!=B_OK) return status;
	bool okay=sState.AbortRecovery(ticket);
	release_sem(sStateLock);
	return okay?B_OK:B_BAD_VALUE;
}

static status_t StandardOps(int32 operation,...)
{
	if (operation==B_MODULE_INIT) {
		if (sStateLock>=B_OK) return B_OK;
		sStateLock=create_sem(1,"P100A persistent driver reload guard");
		return sStateLock<B_OK?sStateLock:B_OK;
	}
	// The kernel pins this module for the remainder of this boot. Refuse a
	// manual unload too;there is no reset shortcut hidden in standard ops.
	return operation==B_MODULE_UNINIT?B_BUSY:B_BAD_VALUE;
}
static pioneer_driver_reload_guard_info sGuard={
	{PIONEER_DRIVER_RELOAD_GUARD_MODULE,B_KEEP_LOADED,StandardOps},1,Attach,Detach,Fault,Cold
};
static pioneer_driver_recovery_guard_info sRecoveryGuard={
	{PIONEER_DRIVER_RECOVERY_GUARD_MODULE,B_KEEP_LOADED,StandardOps},
	1,BeginRecovery,CompleteRecovery,AbortRecovery
};
extern "C" {
module_info* modules[]={&sGuard.info,&sRecoveryGuard.info,NULL};
}
