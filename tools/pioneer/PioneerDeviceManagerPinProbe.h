/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_DEVICE_MANAGER_PIN_PROBE_H
#define PIONEER_DEVICE_MANAGER_PIN_PROBE_H

#include <Drivers.h>

enum { PIONEER_DM_PIN_PROBE = B_DEVICE_OP_CODES_END + 0x350 };
struct pioneer_dm_pin_probe_result {
	uint32 version;
	status_t pinModuleStatus;
	status_t managerModuleStatus;
	status_t invalidStatus;
	status_t rootStatus;
	status_t callbackStatus;
	uint32 callbacks;
};
#endif
