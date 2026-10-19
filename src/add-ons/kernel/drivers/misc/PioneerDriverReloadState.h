/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_DRIVER_RELOAD_STATE_H
#define PIONEER_DRIVER_RELOAD_STATE_H
#include <stdint.h>
// CPU metadata only. The resident guard module serializes every call.
// There is deliberately NO recovery/reset/clear-cold operation.
struct PioneerDriverReloadState {
	uint64_t generation=0;
	bool attached=false,cold=false;
	bool Attach(uint64_t& result) {
		result=0;
		if (cold || attached || generation==UINT64_MAX) return false;
		attached=true; result=++generation; return true;
	}
	bool Detach(uint64_t owner,bool clean) {
		if (!attached || !owner || owner!=generation) { cold=true; return false; }
		attached=false; cold |= !clean; return !cold;
	}
	void Fault(uint64_t) { cold=true; } // stale owner is NOT permission to clear safety state
};
#endif
