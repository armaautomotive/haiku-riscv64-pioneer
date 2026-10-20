/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NORMAL_CLOCK_H
#define PIONEER_Q38_NORMAL_CLOCK_H
#include "PioneerQ38ArcQueue.h"
#include "BlackholeArcClockGuard.h"
// Default-OFF kernel-private context owner. Only the firmware's normal
// BUSY/IDLE arbiter is used; NO forced clock/voltage/reset/flash/DMA.
// The native controller must prove exclusive ownership and ALL workers
// held before Enter, and ALL compute/DMA known complete before Finish.
// These methods do not themselves provide those physical authorities.
// On ANY uncertainty the caller must retain ALL owners and latch cold.
template<class IO> class PioneerQ38NormalClock {
	IO& io;
	PioneerQ38ArcQueue<IO> queue;
	tt_arc_clock_guard_result result={};
	bool attempted=false,closed=false;
	bool Message(const tt_arc_scratch_result& arc,const tt_arc_telemetry_result& health,unsigned message) {
		if (Unsafe()) return false;
		result.messages++; result.last_message=message;
		if (queue.Message(arc,health,result.last,message)) return true;
		if (queue.Unsafe() || result.busy_requested) Quarantine();
		return false;
	}
public:
	explicit PioneerQ38NormalClock(IO& transport):io(transport),queue(transport) {}
	PioneerQ38NormalClock(const PioneerQ38NormalClock&)=delete;
	PioneerQ38NormalClock& operator=(const PioneerQ38NormalClock&)=delete;
	const tt_arc_clock_guard_result& Snapshot() const { return result; } // CPU metadata ONLY
	bool Unsafe() const { return result.recovery_required || queue.Unsafe(); }
	bool Active() const { return result.busy_requested && !result.restored; }
	void Quarantine() { queue.Quarantine(); result.recovery_required=1; } // MEMORY ONLY
	bool Enter(const tt_arc_scratch_result& arc,const tt_arc_telemetry_result& health) {
		if (attempted || closed || Unsafe()) return false;
		attempted=true; result={}; result.version=1; result.restored=1;
		result.last.request_restored=result.last.tlb_restored=1;
		if (health.version!=1 || (health.present_mask&4)==0 || !health.values[2]
			|| health.values[2]>70U*65536) return false; // thermal refusal,ZERO IO
		result.state=TT_ARC_CLOCK_QUERY_FAILED;
		if (!Message(arc,health,0x34)) return false;
		result.before_mhz=result.last.clock_mhz; result.before_mode=result.last.clock_mode;
		result.state=TT_ARC_CLOCK_REFUSED;
		if (result.before_mhz!=800 || result.before_mode!=3) return false;
		result.busy_requested=1; result.restored=0; result.state=TT_ARC_CLOCK_BUSY_FAILED;
		if (!Message(arc,health,0x52)) return false;
		result.busy_acknowledged=1; result.state=TT_ARC_CLOCK_NOT_SETTLED;
		for (unsigned attempt=0;attempt<16;attempt++) {
			io.Pause(); if (!Message(arc,health,0x34)) return false;
			result.busy_mhz=result.last.clock_mhz; result.busy_mode=result.last.clock_mode;
			if (result.busy_mode!=3) { Quarantine(); return false; }
			if (result.busy_mhz>800 && result.busy_mhz<=1400) {
				result.state=TT_ARC_CLOCK_BUSY_VERIFIED; return true;
			}
		}
		// Every query completed cleanly,but BUSY did not settle. The native
		// caller owes explicit Finish while ALL workers are still held.
		return false;
	}
	bool Finish(const tt_arc_scratch_result& arc,const tt_arc_telemetry_result& health) {
		if (Unsafe()) return false; // NO cleanup commands after compute/ARC UNKNOWN
		if (closed) return result.restored==1;
		if (!attempted) return false;
		closed=true; // at most ONE lifecycle finish,even if its reply is lost
		if (!result.busy_requested) return result.restored==1;
		const unsigned previous=result.state;
		result.state=TT_ARC_CLOCK_IDLE_FAILED;
		if (!Message(arc,health,0x54)) return false;
		result.idle_acknowledged=1; result.state=TT_ARC_CLOCK_RESTORE_FAILED;
		for (unsigned attempt=0;attempt<16;attempt++) {
			io.Pause(); if (!Message(arc,health,0x34)) return false;
			result.after_mhz=result.last.clock_mhz; result.after_mode=result.last.clock_mode;
			if (result.after_mode!=3) { Quarantine(); return false; }
			if (result.after_mhz==result.before_mhz) {
				result.restored=1;
				result.state=previous;
				if (previous==TT_ARC_CLOCK_BUSY_VERIFIED) result.state=TT_ARC_CLOCK_RESTORED;
				return true;
			}
		}
		Quarantine(); return false;
	}
};
#endif
