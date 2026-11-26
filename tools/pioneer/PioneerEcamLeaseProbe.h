/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_ECAM_LEASE_PROBE_H
#define PIONEER_ECAM_LEASE_PROBE_H
#include <Drivers.h>
#include <OS.h>
enum { PIONEER_ECAM_LEASE_PROBE = B_DEVICE_OP_CODES_END + 0x352 };
enum { PIONEER_ECAM_LEASE_HOLD = B_DEVICE_OP_CODES_END + 0x353,
	PIONEER_ECAM_LEASE_STATE = B_DEVICE_OP_CODES_END + 0x354 };
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
struct pioneer_ecam_lease_hold {
	uint32 version;
	sem_id ready;
	sem_id release;
	status_t profileStatus;
	uint32 profileStage;
	uint32 profileReady;
	status_t holdStatus;
	status_t callbackStatus;
};
struct pioneer_ecam_lease_state {
	uint32 version;
	status_t stateStatus;
	uint32 active;
	thread_id waiter;
	int32 waiters;
};
#endif
