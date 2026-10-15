/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_RUNTIME_PROFILE_H
#define PIONEER_RUNTIME_PROFILE_H
#include <stdint.h>
enum {
	TT_PROFILE_BEGIN=0,TT_PROFILE_PREPARED=1,TT_PROFILE_STAGED=2,TT_PROFILE_LAUNCHED=3,
	TT_PROFILE_COMPLETED=4,TT_PROFILE_PIO=5,TT_PROFILE_STOPPED=6,TT_PROFILE_DMA=7,TT_PROFILE_FINISHED=8,
	TT_PROFILE_SLOTS=9
};
// Per-session CPU monotonic wall time, INCLUDING guards and polling. Never
// a device-cycle counter or device-only throughput claim; no physical address.
struct tt_runtime_profile_result {
	uint32_t version,available,mask,format,state,recovery_required,started,completed,stopped,clock_stable;
	uint64_t sequence,microseconds[TT_PROFILE_SLOTS];
};
static_assert(sizeof(tt_runtime_profile_result)==120,"fixed profile metadata ABI");
class PioneerRuntimeProfile {
	tt_runtime_profile_result value={};
	uint64_t origin=0,last=0;
public:
	PioneerRuntimeProfile() { value.version=1; }
	const tt_runtime_profile_result& Snapshot() const { return value; }
	void Begin(uint64_t now,unsigned format) {
		uint64_t sequence=value.sequence+1; if (!sequence) sequence=1;
		value={}; value.version=1; value.sequence=sequence; value.format=format;
		value.clock_stable=1; value.mask=1; origin=now; last=0;
	}
	void Mark(uint64_t now,unsigned slot) {
		if (!value.sequence || value.available || slot>=TT_PROFILE_SLOTS || (value.mask&(1U<<slot))) return;
		if (now<origin || now-origin<last) { value.clock_stable=0; return; }
		last=now-origin; value.microseconds[slot]=last; value.mask|=1U<<slot;
	}
	void End(uint64_t now,unsigned state,unsigned recovery,unsigned started,unsigned completed,unsigned stopped) {
		if (!value.sequence || value.available) return;
		Mark(now,TT_PROFILE_FINISHED); value.available=1; value.state=state; value.recovery_required=recovery;
		value.started=started; value.completed=completed; value.stopped=stopped;
	}
};
#endif
