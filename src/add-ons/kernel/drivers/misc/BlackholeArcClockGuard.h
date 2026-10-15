/* SPDX-License-Identifier: MIT */
#ifndef BLACKHOLE_ARC_CLOCK_GUARD_H
#define BLACKHOLE_ARC_CLOCK_GUARD_H
#include "BlackholeArcQueueProbe.h"

enum {
	TT_ARC_CLOCK_REFUSED = 0, TT_ARC_CLOCK_BUSY_VERIFIED = 1,
	TT_ARC_CLOCK_RESTORED = 2, TT_ARC_CLOCK_QUERY_FAILED = 3,
	TT_ARC_CLOCK_BUSY_FAILED = 4, TT_ARC_CLOCK_NOT_SETTLED = 5,
	TT_ARC_CLOCK_IDLE_FAILED = 6, TT_ARC_CLOCK_RESTORE_FAILED = 7
};
struct tt_arc_clock_guard_result {
	uint32_t version, state, busy_requested, busy_acknowledged, idle_acknowledged;
	uint32_t restored, recovery_required, messages, last_message;
	uint32_t before_mhz, busy_mhz, after_mhz, before_mode, busy_mode, after_mode;
	tt_arc_queue_result last;
};
template<class IO> bool BlackholeArcClockMessage(IO& io, const tt_arc_scratch_result& arc,
	const tt_arc_telemetry_result& health, tt_arc_clock_guard_result& r, uint32_t message)
{
	if (r.recovery_required) return false;
	r.messages++; r.last_message = message;
	bool okay = BlackholeArcQueueProbe(io,arc,health,r.last,message);
	r.recovery_required |= r.last.recovery_required;
	return okay;
}

// Cold-start, sole-owner Pioneer control. Requires observed idle 800 MHz,
// unforced firmware DVFS, and a conservative ASIC temperature. Uses normal
// firmware voltage-before-frequency arbitration; NEVER FORCE_AICLK/FORCE_VDD.
// A caller MUST leave after any non-uncertain busy attempt, even failed enter.
template<class IO> bool BlackholeArcClockEnter(IO& io, const tt_arc_scratch_result& arc,
	const tt_arc_telemetry_result& health, tt_arc_clock_guard_result& r)
{
	r = {}; r.version = 1; r.restored = 1;
	r.last.request_restored = r.last.tlb_restored = 1;
	if (health.version != 1 || (health.present_mask & 4) == 0
		|| health.values[2] == 0 || health.values[2] > 70U*65536) return false;
	r.state = TT_ARC_CLOCK_QUERY_FAILED;
	if (!BlackholeArcClockMessage(io,arc,health,r,0x34)) return false;
	r.before_mhz = r.last.clock_mhz; r.before_mode = r.last.clock_mode;
	r.state = TT_ARC_CLOCK_REFUSED;
	if (r.before_mhz != 800 || r.before_mode != 3) return false;
	r.busy_requested = 1; r.restored = 0;
	r.state = TT_ARC_CLOCK_BUSY_FAILED;
	if (!BlackholeArcClockMessage(io,arc,health,r,0x52)) return false;
	r.busy_acknowledged = 1; r.state = TT_ARC_CLOCK_NOT_SETTLED;
	for (unsigned attempt = 0; attempt < 16; attempt++) {
		io.Pause();
		if (!BlackholeArcClockMessage(io,arc,health,r,0x34)) return false;
		r.busy_mhz = r.last.clock_mhz; r.busy_mode = r.last.clock_mode;
		if (r.busy_mode != 3) return false;
		if (r.busy_mhz > r.before_mhz && r.busy_mhz <= 1400) {
			r.state = TT_ARC_CLOCK_BUSY_VERIFIED; return true;
		}
	}
	return false;
}
template<class IO> bool BlackholeArcClockLeave(IO& io, const tt_arc_scratch_result& arc,
	const tt_arc_telemetry_result& health, tt_arc_clock_guard_result& r)
{
	if (r.recovery_required) return false; // no more firmware commands after uncertainty
	if (!r.busy_requested) return r.restored;
	uint32_t previousState = r.state;
	r.state = TT_ARC_CLOCK_IDLE_FAILED;
	if (BlackholeArcClockMessage(io,arc,health,r,0x54)) {
		r.idle_acknowledged = 1; r.state = TT_ARC_CLOCK_RESTORE_FAILED;
		for (unsigned attempt = 0; attempt < 16; attempt++) {
			io.Pause();
			if (!BlackholeArcClockMessage(io,arc,health,r,0x34)) break;
			r.after_mhz = r.last.clock_mhz; r.after_mode = r.last.clock_mode;
			if (r.after_mode != 3) break;
			if (r.after_mhz == r.before_mhz) {
				r.restored = 1;
				r.state = previousState;
				if (previousState == TT_ARC_CLOCK_BUSY_VERIFIED) r.state = TT_ARC_CLOCK_RESTORED;
				return true;
			}
		}
	}
	r.recovery_required = 1; // cannot prove removal of our busy request
	return false;
}
#endif
