/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_SG2042_LEASE_PROBE_MODULE_H
#define PIONEER_SG2042_LEASE_PROBE_MODULE_H
#include <module.h>

#define PIONEER_SG2042_LEASE_PROBE_MODULE "busses/pci/ecam/pioneer_lease_probe/v1"
#define PIONEER_SG2042_LEASE_CONTENTION_MODULE "busses/pci/ecam/pioneer_lease_contention/v1"
// Qualification only: the module runs its own fixed no-op callback. It has
// no caller-provided function and grants no PCI, DMA, firmware or reset access.
struct pioneer_sg2042_lease_probe_module_info {
	module_info info;
	status_t (*probe)(void* cookie, uint32* callbacks, status_t* callbackStatus);
};
// Separate module name prevents older fixed-probe builds from being mistaken
// for a larger interface containing the contention methods.
struct pioneer_sg2042_lease_contention_module_info {
	module_info info;
	// Both semaphores are owned by the qualification caller. The fixed hold
	// uses a two-second timeout if release is never signaled.
	status_t (*hold)(void* cookie, sem_id ready, sem_id release, status_t* callbackStatus);
	// Metadata only. A waiter ID is meaningful only while active is true.
	status_t (*state)(void* cookie, bool* active, thread_id* waiter, int32* waiters);
};
#endif
