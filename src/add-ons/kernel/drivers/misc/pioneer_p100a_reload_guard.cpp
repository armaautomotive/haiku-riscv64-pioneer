/* SPDX-License-Identifier: MIT */
#include "PioneerDriverReloadGuard.h"
#include "PioneerDriverReloadState.h"
#include <KernelExport.h>
// B_KEEP_LOADED is essential:reloading the legacy driver must NEVER erase
// a quarantined card's cold-recovery latch or its retained host ownership.
static PioneerDriverReloadState sState;
static sem_id sStateLock=-1;
static int32 sForcedCold=0;
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
	atomic_or(&sForcedCold,1);
}
static status_t StandardOps(int32 operation,...)
{
	if (operation==B_MODULE_INIT) {
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
extern "C" {
module_info* modules[]={&sGuard.info,NULL};
}
