/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_CLOCK_LEASE_H
#define PIONEER_Q38_CLOCK_LEASE_H
#include "PioneerQ38NormalClock.h"
// Kernel-private lifecycle adapter. No destructor cleanup. The IO object's
// Map/Unmap must manage VIRTUAL VIEWS ONLY, with no implicit ARC messages or
// hardware rollback. A native caller must provide the same exclusive bank
// context, held-worker authority at Begin, and physically proved completed
// STOP/hold/DMA authority at End. A bool is not itself that authority.
template<class IO> class PioneerQ38ClockLease {
	IO& io;
	PioneerQ38NormalClock<IO> clock;
	bool attempted=false,entered=false,ended=false;
public:
	explicit PioneerQ38ClockLease(IO& transport):io(transport),clock(transport) {}
	PioneerQ38ClockLease(const PioneerQ38ClockLease&)=delete;
	PioneerQ38ClockLease& operator=(const PioneerQ38ClockLease&)=delete;
	bool Unsafe() const { return clock.Unsafe(); }
	bool RequiresExplicitEnd() const { return attempted && !ended; }
	const tt_arc_clock_guard_result& Snapshot() const { return clock.Snapshot(); }
	void Quarantine() { clock.Quarantine(); } // MEMORY ONLY,retain ALL owners outside
	bool Begin(const tt_arc_scratch_result& arc,const tt_arc_telemetry_result& health) {
		if (attempted || ended || Unsafe()) return false;
		attempted=true;
		if (!io.Map()) return false; // partial virtual views remain owned
		entered=true; return clock.Enter(arc,health);
	}
	bool End(const tt_arc_scratch_result& arc,const tt_arc_telemetry_result& health,bool allKnownClean) {
		if (Unsafe()) return false;
		if (!allKnownClean) { Quarantine(); return false; } // NO STOP/reset/idle/unmap
		if (ended) return true;
		if (!attempted) return true; // this context never selected normal clock
		if (entered && !clock.Finish(arc,health)) { Quarantine(); return false; }
		if (!io.Unmap()) { Quarantine(); return false; }
		ended=true; return true;
	}
};
#endif
