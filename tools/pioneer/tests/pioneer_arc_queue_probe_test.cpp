/* SPDX-License-Identifier: MIT */
#include <array>
#include <cassert>
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeArcQueueProbe.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeArcClockGuard.h"

struct MockQueue {
	std::array<uint32_t,TT_ARC_QUEUE_CSM_BYTES/4> csm = {};
	uint32_t tlb[3] = {1,2,3}, original[4][8] = {}, base = 0x1002a208;
	uint32_t request = 0, response = 0, msiStatus = 1;
	uint32_t clock = 800, mode = 3;
	unsigned tlbWrites = 0, writes = 0, polls = 0, pauses = 0, notifications = 0;
	unsigned dropTlb = 0, dropWrite = 0;
	bool notified = false, timeout = false, badReply = false, changed = false, failRestore = false;
	bool ignoreBusy = false, ignoreIdle = false;
	uint32_t& At(uint32_t address) { assert(BlackholeArcQueueRange(address,4)); return csm[(address-0x10000000)/4]; }
	MockQueue(unsigned req = 0, unsigned resp = 0) {
		At(0x10001000) = 0x10029ea8; At(0x10001004) = 0x404;
		At(base) = At(base+16) = req; At(base+4) = At(base+20) = resp;
		At(base+24) = 0xffffffff; // counter wraps normally
		request = base+32+(req%4)*32; response = base+160+(resp%4)*32;
		for (unsigned slot = 0; slot < 4; slot++) for (unsigned w = 0; w < 8; w++)
			At(base+32+slot*32+4*w) = original[slot][w] = 0xaabb0000+slot*256+w;
	}
	void Check(bool apb) { for (unsigned w = 0; w < 3; w++) assert(tlb[w] == BlackholeArcQueueTlbWord(apb,w)); }
	uint32_t TlbRead(unsigned w) { assert(w < 3); return tlb[w]; }
	void TlbWrite(unsigned w, uint32_t v) { assert(w < 3); if (++tlbWrites != dropTlb) tlb[w] = v; }
	uint32_t CsmRead(uint32_t a) {
		Check(false);
		if (notified && a == base && ++polls == 3 && !timeout) {
			At(base+16) = At(base);
			At(base+20) = (At(base+4)+1)%8; At(base+24)++;
			uint32_t message = At(request);
			assert(message == 0x34 || message == 0x52 || message == 0x54);
			if (message == 0x52 && !ignoreBusy) clock = 1350;
			if (message == 0x54 && !ignoreIdle) clock = 800;
			At(response) = badReply ? 0xff : 0;
			At(response+4) = message == 0x34 ? clock : 0; At(response+8) = message == 0x34 ? mode : 0;
			if (changed) At(base+4) = (At(base+4)+2)%8;
		}
		return At(a);
	}
	void SetQueueBounds(uint32_t q, uint32_t req, uint32_t resp) {
		assert(q == base && req == base+32+(At(base)%4)*32 && resp == base+160+(At(base+4)%4)*32);
		request = req; response = resp;
	}
	void QueueWrite(uint32_t a, uint32_t v) {
		Check(false);
		assert(a == base || a == base+4 || (a >= request && a < request+32));
		if (a >= request && a < request+32) assert(At(base) == At(base+16) && At(base+4) == At(base+20));
		if (++writes == dropWrite || (failRestore && a >= request && v == original[(request-base-32)/32][(a-request)/4])) return;
		At(a) = v;
	}
	uint32_t MsiStatusRead() { Check(true); return msiStatus; }
	void NotifyArc() { Check(true); assert(At(base) != At(base+16)); notified = true; polls = 0; notifications++; }
	void Pause() { assert(++pauses < 5000); }
	bool Restored() { for (unsigned slot = 0; slot < 4; slot++) for (unsigned w = 0; w < 8; w++) if (At(base+32+slot*32+4*w) != original[slot][w]) return false; return true; }
	static tt_arc_scratch_result Arc() {
		tt_arc_scratch_result a = {}; a.version = 1; a.state = TT_ARC_SCRATCH_READY; a.tlb_restored = 1; a.message_queue = 0x10001000; return a;
	}
	static tt_arc_telemetry_result Health() {
		tt_arc_telemetry_result h = {}; h.version = 1; h.state = TT_ARC_TELEMETRY_OK; h.tlb_restored = 1; h.present_mask = 0xfff;
		h.values[0] = 0x12050001; h.values[6] = 0xf7; h.values[7] = 0x5515; h.values[9] = 0xfff;
		h.values[2] = 48*65536;
		h.tags_address = 0x10002000; h.data_address = 0x10003000; h.entry_count = 63; return h;
	}
};
int main()
{
	auto arc = MockQueue::Arc(); auto health = MockQueue::Health(); tt_arc_queue_result r;
	MockQueue io;
	assert(BlackholeArcQueueRange(0x1002fffc,4));
	assert(!BlackholeArcQueueRange(0x1002fffc,8));
	assert(!BlackholeArcQueueRange(0x10030000,4));
	assert(!BlackholeTelemetryRange(0x10029ea8,4)); // old conservative telemetry bound unchanged
	assert(BlackholeArcQueueProbe(io,arc,health,r));
	assert(r.submitted && r.responded && r.acknowledged && r.request_restored && r.tlb_restored && !r.recovery_required);
	assert(r.clock_mhz == 800 && r.clock_mode == 3 && r.status == 0 && io.notifications == 1 && io.Restored());
	assert(r.before[6] == 0xffffffff && r.after[6] == 0 && r.after[0] == 1 && r.after[1] == 1);
	unsigned configWrites = io.tlbWrites, totalWrites = io.writes;
	for (unsigned req = 0; req < 8; req++) for (unsigned resp = 0; resp < 8; resp++) {
		io = MockQueue(req,resp); assert(BlackholeArcQueueProbe(io,arc,health,r) && io.Restored());
		assert(r.after[0] == (req+1)%8 && r.after[1] == (resp+1)%8);
	}
	for (unsigned drop = 1; drop <= totalWrites; drop++) {
		io = {}; io.dropWrite = drop;
		bool okay = BlackholeArcQueueProbe(io,arc,health,r);
		if (drop <= 8) assert(!okay && !r.submitted && r.request_restored && !r.recovery_required && io.Restored());
		else if (drop <= 10) assert(!okay && r.recovery_required && !r.request_restored);
		else assert(okay && r.request_restored && io.Restored());
	}
	for (unsigned drop = 1; drop <= configWrites; drop++) {
		io = {}; io.dropTlb = drop;
		BlackholeArcQueueProbe(io,arc,health,r);
		assert(r.tlb_restored);
		if (r.submitted && !r.acknowledged) assert(r.recovery_required && !r.request_restored);
	}
	io = {}; io.timeout = true;
	assert(!BlackholeArcQueueProbe(io,arc,health,r) && r.polls == 2000 && r.recovery_required && !r.acknowledged && io.writes == 9);
	io = {}; io.badReply = true;
	assert(!BlackholeArcQueueProbe(io,arc,health,r) && r.state == TT_ARC_QUERY_BAD_RESPONSE && r.acknowledged && !r.recovery_required && io.Restored());
	io = {}; io.changed = true;
	assert(!BlackholeArcQueueProbe(io,arc,health,r) && r.state == TT_ARC_QUERY_CHANGED && r.recovery_required && io.writes == 9);
	io = {}; io.failRestore = true;
	assert(!BlackholeArcQueueProbe(io,arc,health,r) && r.state == TT_ARC_QUERY_RESTORE_FAILED && r.recovery_required && r.acknowledged);
	io = {}; io.msiStatus = 0;
	assert(!BlackholeArcQueueProbe(io,arc,health,r) && r.state == TT_ARC_QUERY_NOTIFY_FAILED && !r.recovery_required && !r.submitted && io.notifications == 0 && io.writes == 0);
	io = {}; io.At(io.base) = 8;
	assert(!BlackholeArcQueueProbe(io,arc,health,r) && r.state == TT_ARC_QUERY_BUSY && io.writes == 0);
	io = {}; io.At(0x10001004) = 0x405;
	assert(!BlackholeArcQueueProbe(io,arc,health,r) && r.state == TT_ARC_QUERY_BAD_DESCRIPTOR && io.writes == 0);
	io = {}; io.At(0x10001000) = health.data_address;
	assert(!BlackholeArcQueueProbe(io,arc,health,r) && r.state == TT_ARC_QUERY_BAD_DESCRIPTOR && io.writes == 0);
	io = {}; health.values[0] = 0;
	assert(!BlackholeArcQueueProbe(io,arc,health,r) && io.tlbWrites == 0);
	io = {}; health = MockQueue::Health();
	assert(!BlackholeArcQueueProbe(io,arc,health,r,0x33) && io.tlbWrites == 0); // no forced-frequency command
	tt_arc_clock_guard_result clock;
	io = {};
	assert(BlackholeArcClockEnter(io,arc,health,clock) && clock.busy_mhz == 1350 && !clock.restored);
	assert(BlackholeArcClockLeave(io,arc,health,clock) && clock.state == TT_ARC_CLOCK_RESTORED && clock.restored && !clock.recovery_required && io.clock == 800);
	assert(clock.messages == 5 && io.Restored());
	for (unsigned drop = 1; drop <= 90; drop++) {
		io = {}; io.dropWrite = drop;
		BlackholeArcClockEnter(io,arc,health,clock);
		unsigned beforeLeave = io.writes;
		bool uncertain = clock.recovery_required;
		BlackholeArcClockLeave(io,arc,health,clock);
		if (uncertain) assert(io.writes == beforeLeave && clock.recovery_required);
		if (!clock.recovery_required) assert(clock.restored && io.clock == 800 && io.Restored());
	}
	io = {}; io.mode = 2;
	assert(!BlackholeArcClockEnter(io,arc,health,clock) && !clock.busy_requested && clock.restored && !clock.recovery_required);
	io = {}; io.ignoreBusy = true;
	assert(!BlackholeArcClockEnter(io,arc,health,clock) && clock.busy_acknowledged && clock.state == TT_ARC_CLOCK_NOT_SETTLED);
	assert(BlackholeArcClockLeave(io,arc,health,clock) && clock.restored && !clock.recovery_required);
	io = {}; io.ignoreIdle = true;
	assert(BlackholeArcClockEnter(io,arc,health,clock));
	assert(!BlackholeArcClockLeave(io,arc,health,clock) && clock.recovery_required && !clock.restored);
	io = {};
	assert(BlackholeArcClockEnter(io,arc,health,clock));
	unsigned writesBefore = io.writes; clock.recovery_required = 1;
	assert(!BlackholeArcClockLeave(io,arc,health,clock) && io.writes == writesBefore); // no further ARC op after compute uncertainty
	io = {}; health.values[2] = 71*65536;
	assert(!BlackholeArcClockEnter(io,arc,health,clock) && clock.restored && io.writes == 0);
}
