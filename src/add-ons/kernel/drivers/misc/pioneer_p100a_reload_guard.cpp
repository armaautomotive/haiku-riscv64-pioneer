/* SPDX-License-Identifier: MIT */
#include "PioneerDriverReloadGuard.h"
#include "PioneerDriverRecoveryGuard.h"
#include "PioneerDriverReloadState.h"
#include <KernelExport.h>
// B_KEEP_LOADED is essential:reloading the legacy driver must NEVER erase
// a quarantined card's cold-recovery latch or its retained host ownership.
static PioneerDriverReloadState sState;
static sem_id sStateLock=-1;
// Bit 0 is cold; the remaining bits are a saturating fault epoch. Publishing
// both in one CAS prevents a recovery from missing a fault between two atomics.
static int32 sFaultState=0;
static bool Cold() { return (uint32(atomic_get(&sFaultState))&1)!=0; }
static void Fault(uint64)
{
	for (;;) {
		const uint32 old=uint32(atomic_get(&sFaultState));
		const uint32 next=old>=0xfffffffeU?0xffffffffU:((old&~1U)+2U)|1U;
		if (uint32(atomic_test_and_set(&sFaultState,int32(next),int32(old)))==old)
			return;
	}
}
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
	if (status!=B_OK) { Fault(generation); return status; }
	bool okay=sState.Detach(generation,clean && !Cold());
	if (!okay) Fault(generation);
	release_sem(sStateLock); return okay?B_OK:B_DEV_NOT_READY;
}
static status_t BeginRecovery(uint64 expectedGeneration,uint64* ticket)
{
	if (!ticket) return B_BAD_VALUE;
	*ticket=0;
	status_t status=acquire_sem(sStateLock); if (status!=B_OK) return status;
	const uint32 faultState=uint32(atomic_get(&sFaultState));
	if (!(faultState&1) || faultState==0xffffffffU || !sState.attached
		|| expectedGeneration!=sState.generation)
		status=B_DEV_NOT_READY;
	else {
		// Fault() is intentionally lock-free. Mirror its atomic cold latch only
		// after observing it under the state lock, and capture its epoch.
		sState.Fault(expectedGeneration);
		bool okay=sState.BeginRecovery(expectedGeneration,faultState,*ticket);
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
	const uint64 oldGeneration=sState.generation;
	bool okay=Cold() && sState.CompleteRecovery(ticket,
		uint32(atomic_get(&sFaultState)),*proof,begunFaultEpoch,generation);
	if (okay) {
		// The CAS clears exactly the cold state covered by this ticket. A
		// concurrent fault changes the word, so it can never be erased.
		if (uint32(atomic_test_and_set(&sFaultState,
			int32(begunFaultEpoch&~1U),int32(begunFaultEpoch)))!=begunFaultEpoch) {
			sState.generation=oldGeneration;
			sState.Fault(oldGeneration);
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
