/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_SG2042_LEASE_PROBE_MODULE_H
#define PIONEER_SG2042_LEASE_PROBE_MODULE_H
#include <module.h>

#define PIONEER_SG2042_LEASE_PROBE_MODULE "busses/pci/ecam/pioneer_lease_probe/v1"
// Qualification only: the module runs its own fixed no-op callback. It has
// no caller-provided function and grants no PCI, DMA, firmware or reset access.
struct pioneer_sg2042_lease_probe_module_info {
	module_info info;
	status_t (*probe)(void* cookie, uint32* callbacks, status_t* callbackStatus);
};
#endif
