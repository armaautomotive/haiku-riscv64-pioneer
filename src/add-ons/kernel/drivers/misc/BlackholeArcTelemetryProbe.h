/* Distributed under the MIT License. */
#ifndef BLACKHOLE_ARC_TELEMETRY_PROBE_H
#define BLACKHOLE_ARC_TELEMETRY_PROBE_H

#include "BlackholeArcScratchProbe.h"

enum {
	TT_ARC_TELEMETRY_COUNT = 12,
	TT_ARC_TELEMETRY_REFUSED = 0,
	TT_ARC_TELEMETRY_OK = 1,
	TT_ARC_TELEMETRY_CONFIG_FAILED = 2,
	TT_ARC_TELEMETRY_BAD_TABLE = 3,
	TT_ARC_TELEMETRY_INCOMPLETE = 4,
	TT_ARC_TELEMETRY_CHANGED = 5,
	TT_ARC_TELEMETRY_RESTORE_FAILED = 6
};

struct tt_arc_telemetry_result {
	uint32_t version, state, tlb_restored;
	uint32_t table_version, entry_count, present_mask;
	uint32_t tags_address, data_address;
	uint32_t tlb_before[3], tlb_trial[3], tlb_after[3];
	uint32_t values[TT_ARC_TELEMETRY_COUNT];
};

static inline uint16_t
BlackholeTelemetryTag(unsigned i)
{
	// Public UMD TelemetryTag values. Deliberately exclude board/ASIC IDs.
	const uint16_t tags[] = {28, 29, 11, 13, 51, 41, 36, 22, 50, 34, 32, 10};
	return i < TT_ARC_TELEMETRY_COUNT ? tags[i] : 0;
}

static inline bool
BlackholeTelemetryRange(uint32_t address, uint32_t length)
{
	// Conservative first 128 KiB of ARC CSM, not its entire SRAM aperture.
	return (address & 3) == 0 && length != 0 && (length & 3) == 0
		&& address >= 0x10000000 && length <= 0x20000
		&& (uint64_t)(address - 0x10000000) + length <= 0x20000;
}

static inline uint32_t
BlackholeTelemetryTlbWord(unsigned i)
{
	// NoC CSM pointers are used directly, not as ARC/APB/XBAR offsets.
	return i == 0 ? 0x80 : BlackholeArcScratchTlbWord(i);
}

template<class IO>
bool
BlackholeArcTelemetryProbe(IO& io, const tt_arc_scratch_result& arc,
	tt_arc_telemetry_result& r)
{
	r = {};
	r.version = 1;
	r.tlb_restored = 1;
	r.tags_address = arc.telemetry_tags;
	r.data_address = arc.telemetry_data;
	if (arc.version != 1 || arc.state != TT_ARC_SCRATCH_READY
		|| !arc.tlb_restored || !arc.telemetry_present
		|| !BlackholeTelemetryRange(r.tags_address, 8)
		|| !BlackholeTelemetryRange(r.data_address, 4)
		|| r.tags_address == r.data_address)
		return false;
	for (unsigned i = 0; i < 3; i++) r.tlb_before[i] = io.TlbRead(i);
	if ((r.tlb_before[2] & 0xffffc000) != 0)
		return false;
	r.tlb_restored = 0;
	for (unsigned i = 0; i < 3; i++) io.TlbWrite(i, BlackholeTelemetryTlbWord(i));
	bool configured = true;
	for (unsigned i = 0; i < 3; i++) {
		r.tlb_trial[i] = io.TlbRead(i);
		configured &= r.tlb_trial[i] == BlackholeTelemetryTlbWord(i);
	}
	r.state = TT_ARC_TELEMETRY_CONFIG_FAILED;
	if (configured) {
		r.table_version = io.CsmRead(r.tags_address);
		r.entry_count = io.CsmRead(r.tags_address + 4);
		r.state = TT_ARC_TELEMETRY_BAD_TABLE;
		bool valid = (r.table_version >> 16) <= 1
			&& r.entry_count != 0 && r.entry_count <= 128;
		uint32_t tableBytes = valid ? 8 + r.entry_count * 4 : 0;
		uint32_t dataBytes = valid ? r.entry_count * 4 : 0;
		valid = valid && BlackholeTelemetryRange(r.tags_address, tableBytes)
			&& BlackholeTelemetryRange(r.data_address, dataBytes)
			&& ((uint64_t)r.tags_address + tableBytes <= r.data_address
				|| (uint64_t)r.data_address + dataBytes <= r.tags_address);
		uint32_t offsets[TT_ARC_TELEMETRY_COUNT] = {};
		uint32_t found = 0;
		for (unsigned i = 0; valid && i < r.entry_count; i++) {
			uint32_t entry = io.CsmRead(r.tags_address + 8 + 4 * i);
			uint32_t tag = entry & 0xffff;
			uint32_t offset = entry >> 16;
			if (offset >= r.entry_count) { valid = false; break; }
			for (unsigned j = 0; j < TT_ARC_TELEMETRY_COUNT; j++) {
				if (tag != BlackholeTelemetryTag(j)) continue;
				if ((found & (1U << j)) != 0) { valid = false; break; }
				found |= 1U << j;
				offsets[j] = offset;
			}
		}
		if (valid) {
			for (unsigned j = 0; j < TT_ARC_TELEMETRY_COUNT; j++) {
				if ((found & (1U << j)) == 0) continue;
				uint32_t address = r.data_address + 4 * offsets[j];
				if (!BlackholeTelemetryRange(address, 4)) { valid = false; break; }
				r.values[j] = io.CsmRead(address);
				r.present_mask |= 1U << j;
			}
		}
		if (valid) {
			r.state = (r.present_mask & 7) == 7 ? TT_ARC_TELEMETRY_OK : TT_ARC_TELEMETRY_INCOMPLETE;
			if (io.CsmRead(r.tags_address) != r.table_version
				|| io.CsmRead(r.tags_address + 4) != r.entry_count) {
				r.state = TT_ARC_TELEMETRY_CHANGED;
				r.present_mask = 0;
			}
		}
	}
	for (unsigned i = 0; i < 3; i++) io.TlbWrite(i, r.tlb_before[i]);
	r.tlb_restored = 1;
	for (unsigned i = 0; i < 3; i++) {
		r.tlb_after[i] = io.TlbRead(i);
		r.tlb_restored &= r.tlb_after[i] == r.tlb_before[i];
	}
	if (!r.tlb_restored) r.state = TT_ARC_TELEMETRY_RESTORE_FAILED;
	return r.state == TT_ARC_TELEMETRY_OK && r.tlb_restored;
}

#endif
