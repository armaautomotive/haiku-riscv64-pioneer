/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_DRIVER_RECOVERY_GUARD_H
#define PIONEER_DRIVER_RECOVERY_GUARD_H
#include <module.h>
#include "PioneerDriverReloadState.h"

#define PIONEER_DRIVER_RECOVERY_GUARD_MODULE \
	"generic/pioneer_p100a_reload_guard/recovery/v1"
#define PIONEER_DRIVER_RECOVERY_GUARD_V2_MODULE \
	"generic/pioneer_p100a_reload_guard/recovery/v2"

// This API is kernel-module-only. Its proof is produced by the device driver
// after reset and reinitialization; no userspace ioctl can supply or clear it.
struct pioneer_driver_recovery_guard_info {
	module_info info;
	uint32 version;
	status_t (*begin)(uint64 expectedGeneration,uint64* ticket);
	status_t (*complete)(uint64 ticket,
		const PioneerDriverRecoveryProof* proof,uint64* newGeneration);
	status_t (*abort)(uint64 ticket);
};

// v2 preserves v1 and adds a non-mutating ticket/epoch check for a retained
// recovery. This is a point-in-time check; complete() still performs the
// final fault-epoch check and atomic cold-latch transition.
struct pioneer_driver_recovery_guard_v2_info {
	module_info info;
	uint32 version;
	status_t (*begin)(uint64 expectedGeneration,uint64* ticket);
	status_t (*complete)(uint64 ticket,
		const PioneerDriverRecoveryProof* proof,uint64* newGeneration);
	status_t (*abort)(uint64 ticket);
	status_t (*validate)(uint64 ticket);
};

#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
#define PIONEER_DRIVER_FABRIC_RECOVERY_GUARD_MODULE \
	"generic/pioneer_p100a_reload_guard/fabric-recovery/witness"
// Experimental and non-loadable. Existing v1/v2 callers continue to use
// their unchanged full-ASIC-reset proof and module interfaces.
struct pioneer_driver_fabric_recovery_guard_info {
	module_info info;
	uint32 version;
	status_t (*begin)(uint64 expectedGeneration,uint64* ticket);
	status_t (*complete)(uint64 ticket,
		const PioneerDriverFabricRecoveryProof* proof,uint64* newGeneration);
	status_t (*abort)(uint64 ticket);
	status_t (*validate)(uint64 ticket);
};
#endif

#endif
