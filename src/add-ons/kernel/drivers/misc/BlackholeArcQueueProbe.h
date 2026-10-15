/* SPDX-License-Identifier: MIT */
#ifndef BLACKHOLE_ARC_QUEUE_PROBE_H
#define BLACKHOLE_ARC_QUEUE_PROBE_H
#include "BlackholeArcTelemetryProbe.h"

enum {
	TT_ARC_QUEUE_CSM_BYTES = 0x30000,
	TT_ARC_QUERY_REFUSED = 0, TT_ARC_QUERY_PASSED = 1,
	TT_ARC_QUERY_BAD_DESCRIPTOR = 2, TT_ARC_QUERY_BUSY = 3,
	TT_ARC_QUERY_STAGE_FAILED = 4, TT_ARC_QUERY_SUBMIT_FAILED = 5,
	TT_ARC_QUERY_NOTIFY_FAILED = 6, TT_ARC_QUERY_TIMEOUT = 7,
	TT_ARC_QUERY_CHANGED = 8, TT_ARC_QUERY_BAD_RESPONSE = 9,
	TT_ARC_QUERY_ACK_FAILED = 10, TT_ARC_QUERY_RESTORE_FAILED = 11
};
struct tt_arc_queue_result {
	uint32_t version, state, submitted, responded, acknowledged, request_restored;
	uint32_t tlb_restored, recovery_required, polls, base, request_address, response_address;
	uint32_t descriptor[4], before[8], after[8], status, clock_mhz, clock_mode, msi_status;
	uint32_t tlb_before[3], tlb_after[3];
};
static inline bool BlackholeArcQueueRange(uint32_t address, uint32_t length)
{
	// Pinned v18.5.0 DTS documents 512 KiB CSM. Only the first 192 KiB
	// is needed for this card's descriptor/queues; legacy telemetry stays 128 KiB.
	return (address & 3) == 0 && length != 0 && (length & 3) == 0
		&& address >= 0x10000000 && length <= TT_ARC_QUEUE_CSM_BYTES
		&& uint64_t(address-0x10000000)+length <= TT_ARC_QUEUE_CSM_BYTES;
}
static inline uint32_t BlackholeArcQueueTlbWord(bool apb, unsigned word)
{
	return word == 0 ? (apb ? 0x400 : 0x80) : word == 1 ? 0x04004000 : 0x40;
}
template<class IO> bool BlackholeArcQueueConfigure(IO& io, bool apb)
{
	for (unsigned w = 0; w < 3; w++) io.TlbWrite(w,BlackholeArcQueueTlbWord(apb,w));
	bool okay = true;
	for (unsigned w = 0; w < 3; w++) okay &= io.TlbRead(w) == BlackholeArcQueueTlbWord(apb,w);
	return okay;
}
static inline bool BlackholeArcQueueOverlap(uint32_t a, uint32_t an, uint32_t b, uint32_t bn)
{
	return uint64_t(a) < uint64_t(b)+bn && uint64_t(b) < uint64_t(a)+an;
}
template<class IO> void BlackholeArcQueueHeader(IO& io, uint32_t base, uint32_t* header)
{
	for (unsigned w = 0; w < 8; w++) header[w] = io.CsmRead(base+4*w);
}
static inline bool BlackholeArcQueueSame(const uint32_t* a, const uint32_t* b)
{
	bool okay = true; for (unsigned w = 0; w < 8; w++) okay &= a[w] == b[w]; return okay;
}
static inline bool BlackholeArcQueueHeaderValid(const uint32_t* h)
{
	return h[0] < 8 && h[1] < 8 && h[4] < 8 && h[5] < 8 && h[2] == 0 && h[3] == 0 && h[7] == 0;
}

// Installed v18.5.0 protocol only: application queue 3, four entries.
// Default is GET_AICLK (0x34). Internal clock guards may use only the
// normal BUSY/IDLE arbiter requests 0x52/0x54, all with NO arguments.
// No arbitrary-message ioctl, forced frequency/voltage, reset, flash or DMA.
// Queue pointers/serial advance by the normal protocol, never get rewound.
// Saved request bytes stay kernel-local. Firmware-owned response bytes are
// never written by the host. Uncertain publication or
// response ownership leaves those bytes in place and latches cold recovery.
template<class IO>
bool BlackholeArcQueueProbe(IO& io, const tt_arc_scratch_result& arc,
	const tt_arc_telemetry_result& health, tt_arc_queue_result& r, uint32_t message = 0x34)
{
	r = {}; r.version = 1; r.request_restored = r.tlb_restored = 1;
	if (message != 0x34 && message != 0x52 && message != 0x54) return false;
	if (arc.version != 1 || arc.state != TT_ARC_SCRATCH_READY || !arc.tlb_restored
		|| health.version != 1 || health.state != TT_ARC_TELEMETRY_OK || !health.tlb_restored
		|| (health.present_mask & 0x3c1) != 0x3c1 || health.values[0] != 0x12050001
		|| health.values[6] != 0xf7 || health.values[7] != 0x5515 || health.values[8] != 0 || health.values[9] != 0xfff
		|| health.entry_count == 0 || health.entry_count > 128
		|| !BlackholeTelemetryRange(health.tags_address,8+health.entry_count*4)
		|| !BlackholeTelemetryRange(health.data_address,health.entry_count*4)
		|| !BlackholeArcQueueRange(arc.message_queue,16)) return false;
	for (unsigned w = 0; w < 3; w++) r.tlb_before[w] = io.TlbRead(w);
	if ((r.tlb_before[2] & 0xffffc000) != 0) return false;
	r.tlb_restored = 0;
	uint32_t savedRequest[8] = {}, current[8] = {};
	bool modified = false, idleOwned = false;
	do {
		r.state = TT_ARC_QUERY_BAD_DESCRIPTOR;
		if (!BlackholeArcQueueConfigure(io,false)) break;
		for (unsigned w = 0; w < 4; w++) r.descriptor[w] = io.CsmRead(arc.message_queue+4*w);
		// Exact four-entry/four-queue descriptor from the pinned firmware.
		uint32_t allBase = r.descriptor[0];
		if (r.descriptor[1] != 0x404 || r.descriptor[2] != 0 || r.descriptor[3] != 0
			|| !BlackholeArcQueueRange(allBase,4*288)
			|| BlackholeArcQueueOverlap(allBase,4*288,arc.message_queue,16)
			|| BlackholeArcQueueOverlap(allBase,4*288,health.tags_address,8+health.entry_count*4)
			|| BlackholeArcQueueOverlap(allBase,4*288,health.data_address,health.entry_count*4)) break;
		// Check the notification path before modifying or publishing a slot.
		r.state = TT_ARC_QUERY_NOTIFY_FAILED;
		if (!BlackholeArcQueueConfigure(io,true)) break;
		r.msi_status = io.MsiStatusRead();
		if ((r.msi_status & 0x301) != 1 || !BlackholeArcQueueConfigure(io,false)) break;
		r.state = TT_ARC_QUERY_BAD_DESCRIPTOR;
		bool descriptorStable = true;
		for (unsigned w = 0; w < 4; w++) descriptorStable &= io.CsmRead(arc.message_queue+4*w) == r.descriptor[w];
		if (!descriptorStable) break;
		r.base = allBase + 3*288;
		r.state = TT_ARC_QUERY_BUSY;
		BlackholeArcQueueHeader(io,r.base,r.before);
		io.Pause(); BlackholeArcQueueHeader(io,r.base,current);
		if (!BlackholeArcQueueHeaderValid(r.before) || !BlackholeArcQueueSame(current,r.before)
			|| r.before[0] != r.before[4] || r.before[1] != r.before[5]) break;
		r.request_address = r.base+32+(r.before[0]%4)*32;
		r.response_address = r.base+32+4*32+(r.before[1]%4)*32;
		io.SetQueueBounds(r.base,r.request_address,r.response_address);
		for (unsigned w = 0; w < 8; w++) {
			savedRequest[w] = io.CsmRead(r.request_address+4*w);
		}
		BlackholeArcQueueHeader(io,r.base,current);
		if (!BlackholeArcQueueSame(current,r.before)) break;
		idleOwned = true; modified = true; r.request_restored = 0;
		r.state = TT_ARC_QUERY_STAGE_FAILED;
		for (unsigned w = 0; w < 8; w++) io.QueueWrite(r.request_address+4*w,w == 0 ? message : 0);
		bool staged = true;
		for (unsigned w = 0; w < 8; w++) staged &= io.CsmRead(r.request_address+4*w) == (w == 0 ? message : 0U);
		if (!staged) break;
		BlackholeArcQueueHeader(io,r.base,current);
		if (!BlackholeArcQueueSame(current,r.before)) { idleOwned = false; r.state = TT_ARC_QUERY_CHANGED; break; }
		uint32_t nextRequest = (r.before[0]+1)%8, nextResponse = (r.before[1]+1)%8;
		r.state = TT_ARC_QUERY_SUBMIT_FAILED;
		r.submitted = 1; idleOwned = false; // before the potentially effective write
		io.QueueWrite(r.base,nextRequest);
		if (io.CsmRead(r.base) != nextRequest) break;
		r.state = TT_ARC_QUERY_NOTIFY_FAILED;
		if (!BlackholeArcQueueConfigure(io,true)) break;
		r.msi_status = io.MsiStatusRead();
		if ((r.msi_status & 0x301) != 1) break;
		io.NotifyArc(); // fixed zero write to inbound 800b0000, not host MSI/DMA
		if (!BlackholeArcQueueConfigure(io,false)) break;
		r.state = TT_ARC_QUERY_TIMEOUT;
		for (unsigned attempt = 0; attempt < 2000; attempt++) {
			r.polls++; BlackholeArcQueueHeader(io,r.base,current);
			if (!BlackholeArcQueueHeaderValid(current) || current[0] != nextRequest || current[1] != r.before[1]
				|| (current[4] != r.before[4] && current[4] != nextRequest)
				|| (current[5] != r.before[5] && current[5] != nextResponse)
				|| (current[6] != r.before[6] && current[6] != uint32_t(r.before[6]+1))) {
				r.state = TT_ARC_QUERY_CHANGED; break;
			}
			if (current[4] == nextRequest && current[5] == nextResponse && current[6] == uint32_t(r.before[6]+1)) {
				r.responded = 1; break;
			}
			io.Pause();
		}
		if (!r.responded) break;
		uint32_t response[8]; for (unsigned w = 0; w < 8; w++) response[w] = io.CsmRead(r.response_address+4*w);
		r.status = response[0]; r.clock_mhz = response[1]; r.clock_mode = response[2];
		bool validResponse = r.status == 0 && (message == 0x34
			? r.clock_mhz >= 200 && r.clock_mhz <= 1400 && r.clock_mode >= 1 && r.clock_mode <= 3
			: r.clock_mhz == 0 && r.clock_mode == 0);
		for (unsigned w = 3; w < 8; w++) validResponse &= response[w] == 0;
		BlackholeArcQueueHeader(io,r.base,r.after);
		if (!BlackholeArcQueueSame(current,r.after)) { r.state = TT_ARC_QUERY_CHANGED; break; }
		r.state = TT_ARC_QUERY_ACK_FAILED;
		io.QueueWrite(r.base+4,nextResponse);
		if (io.CsmRead(r.base+4) != nextResponse) break;
		current[1] = nextResponse;
		BlackholeArcQueueHeader(io,r.base,r.after);
		if (!BlackholeArcQueueSame(current,r.after)) { r.state = TT_ARC_QUERY_CHANGED; break; }
		io.Pause(); BlackholeArcQueueHeader(io,r.base,r.after);
		if (!BlackholeArcQueueSame(current,r.after)) { r.state = TT_ARC_QUERY_CHANGED; break; }
		r.acknowledged = 1; idleOwned = true;
		r.state = validResponse ? TT_ARC_QUERY_PASSED : TT_ARC_QUERY_BAD_RESPONSE;
	} while (false);
	// Restore payload bytes only with proven idle ownership. Never rewind
	// pointers or last_serial, and never rewrite bytes after uncertain submit.
	if (modified && idleOwned) {
		if (!BlackholeArcQueueConfigure(io,false)) idleOwned = false;
		else {
			BlackholeArcQueueHeader(io,r.base,current);
			const uint32_t* expected = r.acknowledged ? r.after : r.before;
			idleOwned = BlackholeArcQueueSame(current,expected);
		}
		if (idleOwned) for (unsigned attempt = 0; attempt < 2 && !r.request_restored; attempt++) {
			for (unsigned w = 0; w < 8; w++) {
				io.QueueWrite(r.request_address+4*w,savedRequest[w]);
			}
			r.request_restored = 1;
			for (unsigned w = 0; w < 8; w++) {
				r.request_restored &= io.CsmRead(r.request_address+4*w) == savedRequest[w];
			}
			BlackholeArcQueueHeader(io,r.base,current);
			r.request_restored &= BlackholeArcQueueSame(current,r.acknowledged ? r.after : r.before);
			if (!r.request_restored && !BlackholeArcQueueSame(current,r.acknowledged ? r.after : r.before)) break;
		}
	}
	for (unsigned attempt = 0; attempt < 2 && !r.tlb_restored; attempt++) {
		for (unsigned w = 0; w < 3; w++) io.TlbWrite(w,r.tlb_before[w]);
		r.tlb_restored = 1;
		for (unsigned w = 0; w < 3; w++) { r.tlb_after[w] = io.TlbRead(w); r.tlb_restored &= r.tlb_after[w] == r.tlb_before[w]; }
	}
	if (!r.request_restored || !r.tlb_restored || (r.submitted && !r.acknowledged)) r.recovery_required = 1;
	if (r.state == TT_ARC_QUERY_PASSED && r.recovery_required) r.state = TT_ARC_QUERY_RESTORE_FAILED;
	return r.state == TT_ARC_QUERY_PASSED && r.submitted && r.responded && r.acknowledged && !r.recovery_required;
}
#endif
