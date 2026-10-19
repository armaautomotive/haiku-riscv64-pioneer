/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_DRIVER_RELOAD_GUARD_H
#define PIONEER_DRIVER_RELOAD_GUARD_H
#include <module.h>
#define PIONEER_DRIVER_RELOAD_GUARD_MODULE "generic/pioneer_p100a_reload_guard/v1"
struct pioneer_driver_reload_guard_info {
	module_info info;
	uint32 version;
	status_t (*attach)(uint64* generation);
	status_t (*detach)(uint64 generation,bool clean);
	void (*fault)(uint64 generation);
	bool (*cold)();
};
#endif
