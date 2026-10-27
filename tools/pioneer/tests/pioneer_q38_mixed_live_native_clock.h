/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_LIVE_NATIVE_CLOCK_TEST_H
#define PIONEER_Q38_MIXED_LIVE_NATIVE_CLOCK_TEST_H
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeArcClockGuard.h"
// TEST bridge to a REAL private PioneerQ38ClockLease,with raw simulated
// ARC/VM endpoints in a separate TU. NO mutable snapshot or ready setter.
class MixedLiveNativeClock {
 struct Impl;Impl* impl;
public:
 bool quarantined=false; // diagnostic mirror only;cannot clear private Unsafe
 MixedLiveNativeClock();~MixedLiveNativeClock();
 MixedLiveNativeClock(const MixedLiveNativeClock&)=delete;
 MixedLiveNativeClock& operator=(const MixedLiveNativeClock&)=delete;
 static tt_arc_scratch_result Arc();static tt_arc_telemetry_result Health();
 bool Begin(const tt_arc_scratch_result&,const tt_arc_telemetry_result&);
 bool End(const tt_arc_scratch_result&,const tt_arc_telemetry_result&,bool);
 bool Unsafe()const;bool RequiresExplicitEnd()const;
 const tt_arc_clock_guard_result& Snapshot()const;
 void Quarantine();void Fault(unsigned);
 unsigned IO()const;unsigned Notifications()const;unsigned Unmaps()const;
};
#endif
