/* SPDX-License-Identifier: MIT */
#ifndef BLACKHOLE_DRAM_SAMPLES_PROBE_H
#define BLACKHOLE_DRAM_SAMPLES_PROBE_H
#include "BlackholeDramRouteProbe.h"

enum {
	TT_DRAM_SAMPLE_REFUSED = 0, TT_DRAM_SAMPLE_ROUTES_OK = 1,
	TT_DRAM_SAMPLE_PASSED = 2, TT_DRAM_SAMPLE_CONFIG_FAILED = 3,
	TT_DRAM_SAMPLE_BAD_ROUTE = 4, TT_DRAM_SAMPLE_CORE_CHANGED = 5,
	TT_DRAM_SAMPLE_DATA_MISMATCH = 6, TT_DRAM_SAMPLE_SOURCE_CHANGED = 7,
	TT_DRAM_SAMPLE_RESTORE_FAILED = 8,
	TT_DRAM_BANKS = 7, TT_DRAM_REGIONS = 5, TT_DRAM_WORDS = 16
};
struct tt_dram_samples_result {
	uint32_t version, state, tlb_restored, data_restored, recovery_required;
	uint32_t routes_verified, cores_checked, saved_regions, modified_regions, passes, mismatches;
	uint32_t source[TT_DRAM_SOURCE_WORDS], source_after[TT_DRAM_SOURCE_WORDS];
	uint32_t translated[TT_DRAM_BANKS][2];
	uint32_t id[TT_DRAM_BANKS][3][TT_DRAM_ID_WORDS];
	uint32_t tlb_before[2][3], tlb_after[2][3];
};

static inline uint32_t BlackholeDramBank(unsigned bank)
{
	const uint32_t banks[] = {0, 1, 2, 4, 5, 6, 7};
	return bank < TT_DRAM_BANKS ? banks[bank] : UINT32_MAX;
}
static inline uint32_t BlackholeDramPhysicalX(unsigned bank)
{
	return bank < 3 ? 0 : 9;
}
static inline uint32_t BlackholeDramPhysicalY(unsigned bank)
{
	const uint32_t rows[] = {0, 2, 9, 0, 2, 9, 5};
	return bank < TT_DRAM_BANKS ? rows[bank] : UINT32_MAX;
}
static inline uint32_t BlackholeDramSampleBase(unsigned region)
{
	// 64-byte samples, all outside the pinned HAL's low reservations and
	// below MEM_DRAM_SIZE=ff000000. Never access the top 16 MiB NIU area.
	const uint32_t bases[] = {0x04000000, 0x40000000, 0x80000000, 0xc0000000, 0xfe000000};
	return region < TT_DRAM_REGIONS ? bases[region] : UINT32_MAX;
}
static inline uint32_t BlackholeDramSamplePattern(unsigned pass, unsigned bank, unsigned region, unsigned word)
{
	uint32_t value = 0x5a69c3e7 ^ (BlackholeDramBank(bank) * 0x1010101)
		^ (region * 0x1234567) ^ (word * 0x10203);
	return pass == 0 ? value : ~value;
}
static inline uint32_t BlackholeDramFixedTlbWord(uint32_t base, uint32_t x, uint32_t y,
	bool readOnly, unsigned word)
{
	// Private policy helper, not an ioctl accepting addresses/coordinates.
	if (word == 0) return base >> 21;
	if (word == 1) return (x << 11) | (y << 17) | (x << 23) | ((y & 7) << 29);
	return word == 2 ? (y >> 3) | (readOnly ? 0xa40U : 0x40U) : 0;
}
static inline uint32_t BlackholeDramSourceX(const uint32_t* source, unsigned x)
{
	return (source[1 + x / 6] >> ((x % 6) * 5)) & 31;
}
static inline uint32_t BlackholeDramSourceY(const uint32_t* source, unsigned y)
{
	return (source[7 + y / 6] >> ((y % 6) * 5)) & 31;
}
static inline bool BlackholeDramBuildRoutes(tt_dram_samples_result& r)
{
	if (!BlackholeDramSourceRoutesBankZero(r.source)) return false;
	for (unsigned bank = 0; bank < TT_DRAM_BANKS; bank++) {
		unsigned matches = 0;
		// Search only the eight documented translated DRAM port-0 slots.
		// Never follow a firmware table to arbitrary coordinates.
		for (unsigned x = 17; x <= 18; x++) for (unsigned y = 12; y <= 21; y += 3) {
			if ((r.source[13] & (1U << x)) != 0 || (r.source[14] & (1U << y)) != 0) return false;
			if (BlackholeDramSourceX(r.source, x) == BlackholeDramPhysicalX(bank)
				&& BlackholeDramSourceY(r.source, y) == BlackholeDramPhysicalY(bank)) {
				r.translated[bank][0] = x; r.translated[bank][1] = y; matches++;
			}
		}
		if (matches != 1) return false;
	}
	// Firmware ENABLED_TENSIX_COL is in die order; fff enables physical
	// columns 1..6 and 11..16 (HARVESTING_NOC_LOCATIONS in pinned UMD).
	uint32_t physicalMask = 0;
	const unsigned columns[] = {1,2,3,4,5,6,7,10,11,12,13,14};
	for (unsigned column : columns) {
		unsigned x = BlackholeDramSourceX(r.source, column);
		if (x > 16 || (r.source[13] & (1U << column)) != 0 || (physicalMask & (1U << x)) != 0) return false;
		physicalMask |= 1U << x;
	}
	if (physicalMask != 0x1f87e) return false;
	for (unsigned y = 2; y <= 11; y++)
		if ((r.source[14] & (1U << y)) != 0 || BlackholeDramSourceY(r.source, y) != y) return false;
	return true;
}
template<class IO>
bool BlackholeDramConfigure(IO& io, unsigned slot, uint32_t base, uint32_t x, uint32_t y, bool readOnly)
{
	for (unsigned word = 0; word < 3; word++) io.DramTlbWrite(slot, word, BlackholeDramFixedTlbWord(base, x, y, readOnly, word));
	bool okay = true;
	for (unsigned word = 0; word < 3; word++)
		okay &= io.DramTlbRead(slot, word) == BlackholeDramFixedTlbWord(base, x, y, readOnly, word);
	return okay;
}
template<class IO>
bool BlackholeDramSourceStable(IO& io, const tt_dram_samples_result& r)
{
	bool okay = true;
	for (unsigned word = 0; word < TT_DRAM_SOURCE_WORDS; word++) okay &= io.SourceRead(word) == r.source[word];
	return okay;
}
template<class IO>
bool BlackholeDramAllCoresHeld(IO& io, tt_dram_samples_result& r)
{
	const unsigned columns[] = {1,2,3,4,5,6,7,10,11,12,13,14};
	for (unsigned column : columns) for (unsigned y = 2; y <= 11; y++) {
		if (!BlackholeDramConfigure(io, 1, 0xffa00000, column, y, true)) {
			r.state = TT_DRAM_SAMPLE_CONFIG_FAILED; return false;
		}
		if (io.ResetRead() != 0x47800 || io.ResetRead() != 0x47800) {
			r.state = TT_DRAM_SAMPLE_CORE_CHANGED; return false;
		}
		r.cores_checked++;
	}
	if (!BlackholeDramConfigure(io, 1, 0xffa00000, 1, 2, true)) {
		r.state = TT_DRAM_SAMPLE_CONFIG_FAILED; return false;
	}
	return true;
}
template<class IO>
bool BlackholeDramSelectSample(IO& io, const tt_dram_samples_result& r, unsigned bank, unsigned region)
{
	return BlackholeDramConfigure(io, 0, BlackholeDramSampleBase(region),
		r.translated[bank][0], r.translated[bank][1], false);
}

// Fixed reversible sample test only. Original memory never leaves the kernel.
// The entire set is written before verification, detecting aliases between
// banks/regions. This samples high addresses; it is NOT a full-capacity audit.
template<class IO>
bool BlackholeDramSamplesProbe(IO& io, const tt_arc_telemetry_result& t,
	const tt_tensix_reset_result& c, bool writeSamples, tt_dram_samples_result& r)
{
	r = {}; r.version = 1; r.tlb_restored = r.data_restored = 1;
	if (t.version != 1 || t.state != TT_ARC_TELEMETRY_OK || !t.tlb_restored
		|| (t.present_mask & 0x3c1) != 0x3c1 || t.values[0] != 0x12050001
		|| t.values[6] != 0xf7 || t.values[7] != 0x5515 || t.values[8] != 0 || t.values[9] != 0xfff
		|| c.version != 1 || c.state != TT_TENSIX_RESET_HELD || !c.tlb_restored
		|| c.reset[0] != 0x47800 || c.reset[1] != 0x47800 || c.reset[2] != 0x47800) return false;
	for (unsigned word = 0; word < TT_DRAM_SOURCE_WORDS; word++) r.source[word] = io.SourceRead(word);
	if (r.source[0] != c.niu_config || !BlackholeDramBuildRoutes(r)) return false;
	for (unsigned slot = 0; slot < 2; slot++) {
		for (unsigned word = 0; word < 3; word++) r.tlb_before[slot][word] = io.DramTlbRead(slot, word);
		if ((r.tlb_before[slot][2] & 0xffffc000) != 0) return false;
	}
	r.tlb_restored = 0;
	uint32_t saved[TT_DRAM_BANKS][TT_DRAM_REGIONS][TT_DRAM_WORDS] = {};
	bool modified[TT_DRAM_BANKS][TT_DRAM_REGIONS] = {};
	bool sourceOkay = true, coresOkay = true;
	do {
		r.state = TT_DRAM_SAMPLE_CONFIG_FAILED;
		for (unsigned bank = 0; bank < TT_DRAM_BANKS; bank++) {
			if (!BlackholeDramConfigure(io, 1, 0xffa00000, r.translated[bank][0], r.translated[bank][1], true)) break;
			bool okay = true;
			for (unsigned sample = 0; sample < 3; sample++) {
				for (unsigned word = 0; word < TT_DRAM_ID_WORDS; word++) r.id[bank][sample][word] = io.DramIdRead(word);
				const uint32_t* id = r.id[bank][sample];
				uint32_t xy = BlackholeDramPhysicalX(bank) | (BlackholeDramPhysicalY(bank) << 6);
				uint32_t logical = r.translated[bank][0] | (r.translated[bank][1] << 6);
				okay &= (id[0] & 0xfff) == xy && ((id[0] >> 12) & 127) == 17
					&& ((id[0] >> 19) & 127) == 12 && (id[0] & 0xf0000000) == 0x10000000
					&& id[1] == (0x80000 | BlackholeDramBank(bank))
					&& (id[2] & 0x9000) == 0x8000 // clock on, GDDR AXI subordinate selected
					// The NIU label retains its documented power-on physical
					// coordinates on firmware 18.5, not an arbitrary zero.
					&& ((id[3] & 0xfff) == xy || (id[3] & 0xfff) == logical);
				for (unsigned word = 0; word < TT_DRAM_ID_WORDS; word++) okay &= id[word] == r.id[bank][0][word];
			}
			if (!okay) { r.state = TT_DRAM_SAMPLE_BAD_ROUTE; break; }
			r.routes_verified++;
		}
		if (r.routes_verified != TT_DRAM_BANKS) break;
		r.state = TT_DRAM_SAMPLE_ROUTES_OK;
		if (!writeSamples) break;
		if (!BlackholeDramAllCoresHeld(io, r)) break;
		if (!BlackholeDramSourceStable(io, r)) { r.state = TT_DRAM_SAMPLE_SOURCE_CHANGED; sourceOkay = false; break; }
		bool okay = true;
		for (unsigned bank = 0; bank < TT_DRAM_BANKS && okay; bank++) for (unsigned region = 0; region < TT_DRAM_REGIONS; region++) {
			if (!BlackholeDramSelectSample(io, r, bank, region)) { r.state = TT_DRAM_SAMPLE_CONFIG_FAILED; okay = false; break; }
			if (io.ResetRead() != 0x47800) { r.state = TT_DRAM_SAMPLE_CORE_CHANGED; coresOkay = okay = false; break; }
			for (unsigned word = 0; word < TT_DRAM_WORDS; word++) saved[bank][region][word] = io.DramRead(word);
			r.saved_regions++;
		}
		if (!okay) break;
		for (unsigned pass = 0; pass < 2 && okay; pass++) {
			for (unsigned bank = 0; bank < TT_DRAM_BANKS && okay; bank++) for (unsigned region = 0; region < TT_DRAM_REGIONS; region++) {
				if (!BlackholeDramSelectSample(io, r, bank, region)) { r.state = TT_DRAM_SAMPLE_CONFIG_FAILED; okay = false; break; }
				if (io.ResetRead() != 0x47800) { r.state = TT_DRAM_SAMPLE_CORE_CHANGED; coresOkay = okay = false; break; }
				if (!modified[bank][region]) r.modified_regions++;
				modified[bank][region] = true; r.data_restored = 0;
				for (unsigned word = 0; word < TT_DRAM_WORDS; word++) io.DramWrite(word, BlackholeDramSamplePattern(pass, bank, region, word));
			}
			if (!okay) break;
			for (unsigned bank = 0; bank < TT_DRAM_BANKS && okay; bank++) for (unsigned region = 0; region < TT_DRAM_REGIONS; region++) {
				if (!BlackholeDramSelectSample(io, r, bank, region)) { r.state = TT_DRAM_SAMPLE_CONFIG_FAILED; okay = false; break; }
				if (io.ResetRead() != 0x47800) { r.state = TT_DRAM_SAMPLE_CORE_CHANGED; coresOkay = okay = false; break; }
				for (unsigned word = 0; word < TT_DRAM_WORDS; word++)
					if (io.DramRead(word) != BlackholeDramSamplePattern(pass, bank, region, word)) r.mismatches++;
			}
			r.passes++;
			if (r.mismatches != 0) { r.state = TT_DRAM_SAMPLE_DATA_MISMATCH; okay = false; }
		}
		if (okay) r.state = TT_DRAM_SAMPLE_PASSED;
	} while (false);
	for (unsigned word = 0; word < TT_DRAM_SOURCE_WORDS; word++) {
		r.source_after[word] = io.SourceRead(word);
		if (r.source_after[word] != r.source[word]) sourceOkay = false;
	}
	if (!sourceOkay) { r.state = TT_DRAM_SAMPLE_SOURCE_CHANGED; r.recovery_required = 1; }
	if (!r.data_restored && sourceOkay && coresOkay) {
		// Re-prove ownership of ALL enabled tiles even after a pattern or
		// configuration failure, before writing any original payload back.
		if (!BlackholeDramAllCoresHeld(io, r)) coresOkay = false;
		if (!BlackholeDramSourceStable(io, r)) {
			sourceOkay = false; r.state = TT_DRAM_SAMPLE_SOURCE_CHANGED;
		}
	}
	if (!r.data_restored && sourceOkay && coresOkay) {
		// Never restore payload through an unverified route or while core
		// ownership is unknown. Each restore re-verifies the fixed window.
		bool restoredAll = true;
		if (!BlackholeDramConfigure(io, 1, 0xffa00000, 1, 2, true)) restoredAll = false;
		for (unsigned bank = 0; bank < TT_DRAM_BANKS && restoredAll; bank++) for (unsigned region = 0; region < TT_DRAM_REGIONS; region++) {
			if (!modified[bank][region]) continue;
			bool restored = false;
			for (unsigned attempt = 0; attempt < 2 && !restored; attempt++) {
				if (!BlackholeDramSelectSample(io, r, bank, region)) continue;
				if (io.ResetRead() != 0x47800) break;
				for (unsigned word = 0; word < TT_DRAM_WORDS; word++) io.DramWrite(word, saved[bank][region][word]);
				restored = io.ResetRead() == 0x47800;
				for (unsigned word = 0; word < TT_DRAM_WORDS; word++) restored &= io.DramRead(word) == saved[bank][region][word];
			}
			if (!restored) { restoredAll = false; break; }
		}
		r.data_restored = restoredAll;
	}
	for (unsigned attempt = 0; attempt < 2 && !r.tlb_restored; attempt++) {
		for (unsigned slot = 0; slot < 2; slot++) for (unsigned word = 0; word < 3; word++) io.DramTlbWrite(slot, word, r.tlb_before[slot][word]);
		r.tlb_restored = 1;
		for (unsigned slot = 0; slot < 2; slot++) for (unsigned word = 0; word < 3; word++) {
			r.tlb_after[slot][word] = io.DramTlbRead(slot, word);
			r.tlb_restored &= r.tlb_after[slot][word] == r.tlb_before[slot][word];
		}
	}
	if (!r.data_restored || !r.tlb_restored) { r.state = TT_DRAM_SAMPLE_RESTORE_FAILED; r.recovery_required = 1; }
	return r.state == (writeSamples ? TT_DRAM_SAMPLE_PASSED : TT_DRAM_SAMPLE_ROUTES_OK)
		&& r.data_restored && r.tlb_restored && !r.recovery_required;
}
#endif
