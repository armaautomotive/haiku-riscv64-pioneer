/* SPDX-License-Identifier: MIT */
#ifndef BLACKHOLE_DRAM_ROUTE_PROBE_H
#define BLACKHOLE_DRAM_ROUTE_PROBE_H

#include "BlackholeTensixResetProbe.h"

enum {
	TT_DRAM_ROUTE_REFUSED = 0, TT_DRAM_ROUTE_PASSED = 1,
	TT_DRAM_ROUTE_CONFIG_FAILED = 2, TT_DRAM_ROUTE_BAD_ID = 3,
	TT_DRAM_ROUTE_CHANGED = 4, TT_DRAM_ROUTE_RESTORE_FAILED = 5,
	TT_DRAM_SOURCE_WORDS = 16, TT_DRAM_ID_WORDS = 4
};

struct tt_dram_route_result {
	uint32_t version, state, tlb_restored, recovery_required;
	uint32_t source[TT_DRAM_SOURCE_WORDS];
	uint32_t source_after[TT_DRAM_SOURCE_WORDS];
	uint32_t tlb_before[3], tlb_trial[3], tlb_after[3];
	uint32_t id[3][TT_DRAM_ID_WORDS];
};

// Independent encoding from the pinned public Blackhole NIU/TLB register
// descriptions. No code from the documentation is incorporated here.
// Private fixed route: enabled physical bank 0 / port 0 (0,0), addressed as
// translated (18,12) for this exact firmware and f7 GDDR harvesting mask.
static inline uint32_t BlackholeDramRouteTlbWord(unsigned word)
{
	const uint32_t words[] = {0x7fd, 0x89189000, 0xa41};
	return word < 3 ? words[word] : 0;
}

static inline uint32_t BlackholeDramSourceOffset(unsigned word)
{
	if (word == 0) return 0x100; // NIU_CFG_0
	if (word <= 6) return 0x118 + (word - 1) * 4; // X translation
	if (word <= 12) return 0x130 + (word - 7) * 4; // Y translation
	if (word == 13) return 0x150; // untranslated-column mask
	if (word == 14) return 0x154; // untranslated-row mask
	return word == 15 ? 0x16c : UINT32_MAX; // DDR translation feature flags
}

static inline uint32_t BlackholeDramIdOffset(unsigned word)
{
	const uint32_t offsets[] = {0x44, 0x48, 0x100, 0x148};
	return word < TT_DRAM_ID_WORDS ? offsets[word] : UINT32_MAX;
}

static inline bool BlackholeDramSourceRoutesBankZero(const uint32_t* source)
{
	// Refuse nonstandard DDR remapping rather than guessing its semantics.
	if ((source[0] & 0x5000) != 0x4000 || (source[15] & 0xc00) != 0
		|| (source[14] & (1U << 12)) != 0 || (source[13] & (1U << 18)) != 0)
		return false;
	uint32_t x = (source[1 + 18 / 6] >> ((18 % 6) * 5)) & 31;
	uint32_t y = (source[7 + 12 / 6] >> ((12 % 6) * 5)) & 31;
	return x == 0 && y == 0;
}

static inline bool BlackholeDramBankZeroId(const uint32_t* id)
{
	return (id[0] & 0xfff) == 0 && ((id[0] >> 12) & 127) == 17
		&& ((id[0] >> 19) & 127) == 12 && (id[0] & 0xf0000000) == 0x10000000
		&& id[1] == 0x00080000 // NoC 0, DRAM type, bank/group 0
		&& (id[2] & 0x1000) == 0 // containing tile is not clock-disabled
		// Firmware 18.5 on the measured board leaves NOC_ID_LOGICAL at
		// zero. This writable software label is NOT the routing table.
		// Physical NODE_ID + typed ENDPOINT_ID and the source translation
		// proof above independently identify the destination. Accept only
		// the observed unset label or the prescribed translated label.
		&& ((id[3] & 0xfff) == 0 || (id[3] & 0xfff) == (18U | (12U << 6)));
}

// IO has only fixed, read-only NIU register accesses and the existing fixed
// TLB control. No GDDR payload, NIU configuration, core reset or DMA writes.
template<class IO>
bool BlackholeDramRouteProbe(IO& io, const tt_arc_telemetry_result& telemetry,
	const tt_tensix_reset_result& core, tt_dram_route_result& r)
{
	r = {}; r.version = 1; r.tlb_restored = 1;
	if (telemetry.version != 1 || telemetry.state != TT_ARC_TELEMETRY_OK
		|| !telemetry.tlb_restored || (telemetry.present_mask & 0x3c1) != 0x3c1
		|| telemetry.values[0] != 0x12050001 || telemetry.values[6] != 0xf7
		|| telemetry.values[7] != 0x5515 || telemetry.values[8] != 0
		|| core.version != 1 || core.state != TT_TENSIX_RESET_HELD
		|| !core.tlb_restored || core.reset[0] != 0x47800
		|| core.reset[1] != 0x47800 || core.reset[2] != 0x47800)
		return false;
	for (unsigned i = 0; i < TT_DRAM_SOURCE_WORDS; i++) r.source[i] = io.SourceRead(i);
	if (r.source[0] != core.niu_config || !BlackholeDramSourceRoutesBankZero(r.source))
		return false;
	for (unsigned i = 0; i < 3; i++) r.tlb_before[i] = io.TlbRead(i);
	if ((r.tlb_before[2] & 0xffffc000) != 0) return false;
	r.tlb_restored = 0;
	for (unsigned i = 0; i < 3; i++) io.TlbWrite(i, BlackholeDramRouteTlbWord(i));
	bool configured = true;
	for (unsigned i = 0; i < 3; i++) {
		r.tlb_trial[i] = io.TlbRead(i);
		configured &= r.tlb_trial[i] == BlackholeDramRouteTlbWord(i);
	}
	r.state = TT_DRAM_ROUTE_CONFIG_FAILED;
	if (configured) {
		r.state = TT_DRAM_ROUTE_PASSED;
		for (unsigned sample = 0; sample < 3; sample++) {
			for (unsigned word = 0; word < TT_DRAM_ID_WORDS; word++)
				r.id[sample][word] = io.DramIdRead(word);
			if (!BlackholeDramBankZeroId(r.id[sample])) r.state = TT_DRAM_ROUTE_BAD_ID;
			for (unsigned word = 0; word < TT_DRAM_ID_WORDS; word++)
				if (r.id[sample][word] != r.id[0][word]) r.state = TT_DRAM_ROUTE_BAD_ID;
		}
	}
	for (unsigned i = 0; i < TT_DRAM_SOURCE_WORDS; i++) {
		r.source_after[i] = io.SourceRead(i);
		if (r.source_after[i] != r.source[i]) {
			r.state = TT_DRAM_ROUTE_CHANGED;
			r.recovery_required = 1;
		}
	}
	for (unsigned attempt = 0; attempt < 2 && !r.tlb_restored; attempt++) {
		for (unsigned i = 0; i < 3; i++) io.TlbWrite(i, r.tlb_before[i]);
		r.tlb_restored = 1;
		for (unsigned i = 0; i < 3; i++) {
			r.tlb_after[i] = io.TlbRead(i);
			r.tlb_restored &= r.tlb_after[i] == r.tlb_before[i];
		}
	}
	if (!r.tlb_restored) {
		r.state = TT_DRAM_ROUTE_RESTORE_FAILED;
		r.recovery_required = 1;
	}
	return r.state == TT_DRAM_ROUTE_PASSED && r.tlb_restored && !r.recovery_required;
}
#endif
