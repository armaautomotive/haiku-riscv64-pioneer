/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_ECAM_LEASE_PROBE_H
#define PIONEER_ECAM_LEASE_PROBE_H
#include <Drivers.h>
enum { PIONEER_ECAM_LEASE_PROBE = B_DEVICE_OP_CODES_END + 0x352 };
struct pioneer_ecam_lease_probe_result {
	uint32 version;
	status_t managerStatus;
	status_t pciStatus;
	uint32 cards;
	status_t profileStatus;
	uint32 profileStage;
	uint32 profileReady;
	status_t leaseModuleStatus;
	status_t leaseStatus;
	status_t leaseCallbackStatus;
	uint32 leaseCallbacks;
	status_t recoveryModuleStatus;
	status_t recoveryStatus;
	uint32 recoveryCallbacks;
};
#endif
