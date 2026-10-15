/* SPDX-License-Identifier: MIT */
#ifndef BLACKHOLE_CLOCKED_MATMUL_PROBE_H
#define BLACKHOLE_CLOCKED_MATMUL_PROBE_H
#include "BlackholeArcClockGuard.h"
#include "BlackholeTensixMatmulProbe.h"

template<class IO>
bool BlackholeClockedMatmulProbe(IO& io, tt_dram_samples_result& baseline,
	tt_tensix_matmul_result& math, tt_arc_clock_guard_result& clock)
{
	bool entered = io.EnterNormalClock(clock);
	if (clock.recovery_required) return false;
	bool passed = entered && BlackholeTensixMatmulProbe(io,baseline,math);
	if (math.recovery_required) {
		// Never issue another ARC command after uncertain compute. Cold
		// recovery also resets the nonpersistent firmware busy request.
		clock.recovery_required = 1;
		return false;
	}
	bool left = io.LeaveNormalClock(clock);
	return entered && passed && left && clock.restored && !clock.recovery_required;
}
#endif
