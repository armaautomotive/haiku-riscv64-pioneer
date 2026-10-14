/* Distributed under the MIT License. */
#ifndef BLACKHOLE_ARC_SCRATCH_PROBE_H
#define BLACKHOLE_ARC_SCRATCH_PROBE_H

#include <stdint.h>

enum {
	TT_ARC_SCRATCH_REFUSED = 0,
	TT_ARC_SCRATCH_READY = 1,
	TT_ARC_SCRATCH_NOT_READY = 2,
	TT_ARC_SCRATCH_CONFIG_FAILED = 3,
	TT_ARC_SCRATCH_RESTORE_FAILED = 4
};

struct tt_arc_scratch_result {
	uint32_t version;
	uint32_t state;
	uint32_t tlb_restored;
	uint32_t tlb_before[3];
	uint32_t tlb_trial[3];
	uint32_t tlb_after[3];
	uint32_t boot_before;
	uint32_t boot_after;
	uint32_t error_status0;
	uint32_t message_queue;
	uint32_t telemetry_data;
	uint32_t telemetry_tags;
	uint32_t telemetry_present;
};

// Hardware layout from the pinned Apache-2.0 tt-umd Blackhole architecture
// constants and read-only window attributes. No vendor implementation copied.
// Last 2 MiB window = 201, NOC0 ARC = (8,0), local page = 0x80000000.
// Strict ordering, unicast-read static VC with read buddy; no data writes.
static inline uint32_t
BlackholeArcScratchTlbWord(unsigned word)
{
	const uint32_t values[] = {0x00000400, 0x04004000, 0x00000a40};
	return word < 3 ? values[word] : 0;
}

template<class IO>
bool
BlackholeArcScratchProbe(IO& io, tt_arc_scratch_result& r)
{
	r = {};
	r.version = 1;
	r.tlb_restored = 1; // No writes yet.
	for (unsigned i = 0; i < 3; i++)
		r.tlb_before[i] = io.TlbRead(i);
	// Unknown/reserved high bits are not a supported saved configuration.
	if ((r.tlb_before[2] & 0xffffc000) != 0)
		return false;
	r.tlb_restored = 0;
	for (unsigned i = 0; i < 3; i++)
		io.TlbWrite(i, BlackholeArcScratchTlbWord(i));
	bool configured = true;
	for (unsigned i = 0; i < 3; i++) {
		r.tlb_trial[i] = io.TlbRead(i);
		configured &= r.tlb_trial[i] == BlackholeArcScratchTlbWord(i);
	}
	r.state = TT_ARC_SCRATCH_CONFIG_FAILED;
	if (configured) {
		r.boot_before = io.ScratchRead(2);
		r.error_status0 = io.ScratchRead(4);
		r.message_queue = io.ScratchRead(11);
		r.telemetry_data = io.ScratchRead(12);
		r.telemetry_tags = io.ScratchRead(13);
		r.boot_after = io.ScratchRead(2);
		bool ready = r.boot_before == r.boot_after && (r.boot_before & 7) == 5
			&& r.error_status0 != UINT32_MAX && (r.error_status0 & 2) == 0;
		r.state = ready ? TT_ARC_SCRATCH_READY : TT_ARC_SCRATCH_NOT_READY;
		r.telemetry_present = r.telemetry_data != 0 && r.telemetry_tags != 0
			&& (r.telemetry_data & 3) == 0 && (r.telemetry_tags & 3) == 0;
	}
	for (unsigned i = 0; i < 3; i++)
		io.TlbWrite(i, r.tlb_before[i]);
	bool restored = true;
	for (unsigned i = 0; i < 3; i++) {
		r.tlb_after[i] = io.TlbRead(i);
		restored &= r.tlb_after[i] == r.tlb_before[i];
	}
	r.tlb_restored = restored;
	if (!restored)
		r.state = TT_ARC_SCRATCH_RESTORE_FAILED;
	return restored && r.state == TT_ARC_SCRATCH_READY;
}

#endif
