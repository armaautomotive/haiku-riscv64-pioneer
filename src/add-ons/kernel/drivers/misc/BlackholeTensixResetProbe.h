/* Distributed under the MIT License. */
#ifndef BLACKHOLE_TENSIX_RESET_PROBE_H
#define BLACKHOLE_TENSIX_RESET_PROBE_H

#include "BlackholeArcTelemetryProbe.h"

enum {
	TT_TENSIX_RESET_REFUSED = 0,
	TT_TENSIX_RESET_HELD = 1,
	TT_TENSIX_RESET_RUNNING = 2,
	TT_TENSIX_RESET_CONFIG_FAILED = 3,
	TT_TENSIX_RESET_RESTORE_FAILED = 4
};

struct tt_tensix_reset_result {
	uint32_t version, state, tlb_restored, niu_config;
	uint32_t tlb_before[3], tlb_trial[3], tlb_after[3];
	uint32_t reset[3];
};

static inline uint32_t
BlackholeTensixResetTlbWord(unsigned i)
{
	// Physical NoC0 (1,2), local base 0xffa00000, strict unicast read.
	// Public UMD: enabled-column bit 0 names physical x=1; row y=2 is Tensix.
	const uint32_t words[] = {0x7fd, 0x40840800, 0xa40};
	return i < 3 ? words[i] : 0;
}

template<class IO>
bool
BlackholeTensixResetProbe(IO& io, const tt_arc_telemetry_result& telemetry,
	tt_tensix_reset_result& r)
{
	r = {};
	r.version = 1;
	r.tlb_restored = 1;
	// Private test for the measured firmware, not an arbitrary tile interface.
	if (telemetry.version != 1 || telemetry.state != TT_ARC_TELEMETRY_OK
		|| !telemetry.tlb_restored || (telemetry.present_mask & 0x201) != 0x201
		|| telemetry.values[0] != 0x12050001
		|| (telemetry.values[9] & 1) == 0 || (telemetry.values[9] & ~0x3fffU) != 0)
		return false;
	r.niu_config = io.NiuRead();
	// The pinned UMD's fill_tensix_noc0_translated_mapping() maps the first
	// enabled physical column to x=1. Bit 0 above requires x=1 itself enabled,
	// so this sole tile is (1,2) with translation either on or off. This does
	// NOT justify arbitrary physical coordinates under translation.
	if (r.niu_config == UINT32_MAX)
		return false;
	for (unsigned i = 0; i < 3; i++) r.tlb_before[i] = io.TlbRead(i);
	if ((r.tlb_before[2] & 0xffffc000) != 0) return false;
	r.tlb_restored = 0;
	for (unsigned i = 0; i < 3; i++) io.TlbWrite(i, BlackholeTensixResetTlbWord(i));
	bool configured = true;
	for (unsigned i = 0; i < 3; i++) {
		r.tlb_trial[i] = io.TlbRead(i);
		configured &= r.tlb_trial[i] == BlackholeTensixResetTlbWord(i);
	}
	r.state = TT_TENSIX_RESET_CONFIG_FAILED;
	if (configured) {
		for (unsigned i = 0; i < 3; i++) r.reset[i] = io.ResetRead();
		// BRISC, TRISC0/1/2 and NCRISC. Never assert/deassert any reset here.
		bool held = r.reset[0] != UINT32_MAX && r.reset[0] == r.reset[1]
			&& r.reset[1] == r.reset[2] && (r.reset[0] & 0x47800) == 0x47800;
		r.state = held ? TT_TENSIX_RESET_HELD : TT_TENSIX_RESET_RUNNING;
	}
	for (unsigned i = 0; i < 3; i++) io.TlbWrite(i, r.tlb_before[i]);
	r.tlb_restored = 1;
	for (unsigned i = 0; i < 3; i++) {
		r.tlb_after[i] = io.TlbRead(i);
		r.tlb_restored &= r.tlb_after[i] == r.tlb_before[i];
	}
	if (!r.tlb_restored) r.state = TT_TENSIX_RESET_RESTORE_FAILED;
	return r.state == TT_TENSIX_RESET_HELD && r.tlb_restored;
}

#endif
