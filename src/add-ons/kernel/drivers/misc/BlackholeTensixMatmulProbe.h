/* SPDX-License-Identifier: MIT */
#ifndef BLACKHOLE_TENSIX_MATMUL_PROBE_H
#define BLACKHOLE_TENSIX_MATMUL_PROBE_H
#include "BlackholeDramSamplesProbe.h"
#include "BlackholeTensixImage.h"

enum {
	TT_MATMUL_REFUSED = 0, TT_MATMUL_PASSED = 1, TT_MATMUL_CONFIG_FAILED = 2,
	TT_MATMUL_CORE_CHANGED = 3, TT_MATMUL_IMAGE_FAILED = 4,
	TT_MATMUL_COMPLETION_FAILED = 5, TT_MATMUL_RESULT_MISMATCH = 6,
	TT_MATMUL_STOP_FAILED = 7, TT_MATMUL_RESTORE_FAILED = 8,
	TT_MATMUL_SOURCE_CHANGED = 9
};
struct tt_tensix_matmul_result {
	uint32_t version, state, started, completed, stopped, l1_restored, tlb_restored;
	uint32_t recovery_required, context_reset, polls, phase, fixtures, marker, mismatches;
	uint32_t representative[2], pc[2], controls[3];
	uint32_t reset_before, reset_initialized, reset_running, reset_after;
	uint32_t tlb_before[2][3], tlb_after[2][3];
};
static inline uint32_t BlackholeMatmulExpected(unsigned output)
{
	return output < 128 ? 0x41800000 : 0x42c00000; // 16.0, 96.0
}
static inline uint32_t BlackholeMatmulTlbWord(unsigned slot, unsigned word)
{
	return BlackholeDramFixedTlbWord(slot == 0 ? 0 : 0xffa00000,1,2,false,word);
}

// Sole idle tile (1,2), TRISC0 only. Explicitly reinitializes its coprocessor
// context, not firmware. Saved L1 bytes remain kernel-local. Original Src/Dst,
// local RISC RAM and compute configuration are intentionally NOT preserved.
// There are no device-issued NoC requests, host DMA, IRQs or ARC messages.
template<class IO>
bool BlackholeTensixMatmulProbe(IO& io, tt_dram_samples_result& baseline,
	tt_tensix_matmul_result& r)
{
	r = {}; r.version = 1; r.l1_restored = r.tlb_restored = 1;
	if (baseline.version != 1 || baseline.state != TT_DRAM_SAMPLE_ROUTES_OK
		|| baseline.routes_verified != TT_DRAM_BANKS || !baseline.data_restored
		|| !baseline.tlb_restored || baseline.recovery_required || !BlackholeDramBuildRoutes(baseline)) return false;
	for (unsigned slot = 0; slot < 2; slot++) {
		for (unsigned word = 0; word < 3; word++) r.tlb_before[slot][word] = io.DramTlbRead(slot,word);
		if ((r.tlb_before[slot][2] & 0xffffc000) != 0) return false;
	}
	r.tlb_restored = 0;
	uint32_t saved[TT_TENSIX_IMAGE_WORDS] = {};
	bool modified = false;
	do {
		r.state = TT_MATMUL_CORE_CHANGED;
		if (!BlackholeDramAllCoresHeld(io,baseline)) break;
		r.state = TT_MATMUL_CONFIG_FAILED;
		for (unsigned slot = 0; slot < 2; slot++) for (unsigned word = 0; word < 3; word++)
			io.DramTlbWrite(slot,word,BlackholeMatmulTlbWord(slot,word));
		bool configured = true;
		for (unsigned slot = 0; slot < 2; slot++) for (unsigned word = 0; word < 3; word++)
			configured &= io.DramTlbRead(slot,word) == BlackholeMatmulTlbWord(slot,word);
		if (!configured) break;
		r.state = TT_MATMUL_CORE_CHANGED;
		r.reset_before = io.ResetRead();
		if (r.reset_before != 0x47800 || io.ResetRead() != 0x47800) break;
		for (unsigned i = 0; i < 3; i++) r.controls[i] = io.MathControlRead(i);
		// No redirected entry, debug FIFO owner or manual-sync bypass.
		if ((r.controls[0] & 1) != 0 || r.controls[1] != 0 || r.controls[2] != 0) break;
		if (!BlackholeDramSourceStable(io,baseline)) { r.state = TT_MATMUL_SOURCE_CHANGED; break; }
		for (unsigned i = 0; i < TT_TENSIX_IMAGE_WORDS; i++) saved[i] = io.MathImageRead(i);
		if (io.ResetRead() != 0x47800) break;
		modified = true; r.l1_restored = 0;
		for (unsigned i = 1; i < TT_TENSIX_IMAGE_WORDS; i++) io.MathImageWrite(i,BlackholeTensixImageWord(i));
		if (io.ResetRead() != 0x47800) break;
		io.MathImageWrite(0,BlackholeTensixImageWord(0));
		r.state = TT_MATMUL_IMAGE_FAILED;
		bool imageOkay = true;
		for (unsigned i = 0; i < TT_TENSIX_IMAGE_WORDS; i++) imageOkay &= io.MathImageRead(i) == BlackholeTensixImageWord(i);
		if (!imageOkay) break;
		r.state = TT_MATMUL_CORE_CHANGED;
		if (io.ResetRead() != 0x47800) break;
		// Reset all idle backends, operand-bank ownership, Dst validity and
		// Auto TTSync, while all five RISCs remain held. Only this one tile.
		r.context_reset = 1;
		io.MathResetWrite(0x87ffff);
		io.Pause(); r.reset_initialized = io.ResetRead();
		if (r.reset_initialized != 0x87ffff || io.ResetRead() != 0x87ffff) break;
		r.started = 1; // set before write; a dropped start is still uncertain
		io.MathResetWrite(0x46800); // only TRISC0 leaves reset
		r.state = TT_MATMUL_COMPLETION_FAILED;
		for (unsigned attempt = 0; attempt < 2000; attempt++) {
			r.polls++; r.reset_running = io.ResetRead();
			if (r.reset_running != 0x46800) { r.state = TT_MATMUL_CORE_CHANGED; break; }
			// Read-only progress snapshots also on timeout, not only after
			// a completion marker. A snapshot is never itself completion.
			if ((attempt & 127) == 0) {
				r.pc[0] = io.MathPcRead(); io.Pause(); r.pc[1] = io.MathPcRead();
			}
			r.phase = io.MathImageRead(TT_TENSIX_STATUS_INDEX);
			r.fixtures = io.MathImageRead(TT_TENSIX_STATUS_INDEX+1);
			r.marker = io.MathImageRead(TT_TENSIX_STATUS_INDEX+2);
			if (r.phase == TT_TENSIX_FINAL_PHASE && r.fixtures == 2 && r.marker == 0x4d415431) {
				io.Pause(); r.pc[0] = io.MathPcRead();
				io.Pause(); r.pc[1] = io.MathPcRead();
				if (r.pc[0] == TT_TENSIX_PARK_PC && r.pc[1] == TT_TENSIX_PARK_PC) {
					r.completed = 1;
					for (unsigned i = 0; i < TT_TENSIX_OUTPUT_WORDS; i++) {
						uint32_t value = io.MathImageRead(1 + TT_TENSIX_CODE_WORDS + i);
						if (i == 0 || i == 128) r.representative[i / 128] = value;
						if (value != BlackholeMatmulExpected(i)) r.mismatches++;
					}
					r.state = r.mismatches == 0 ? TT_MATMUL_PASSED : TT_MATMUL_RESULT_MISMATCH;
					break;
				}
			}
			io.Pause();
		}
		if (!BlackholeDramSourceStable(io,baseline)) { r.state = TT_MATMUL_SOURCE_CHANGED; break; }
		// Stop only from the known sole-running-TRISC0 state. Reset backends
		// as well; do not mistake asserting just a RISC bit for FPU drainage.
		if (io.ResetRead() != 0x46800) { r.state = TT_MATMUL_STOP_FAILED; break; }
		io.MathResetWrite(0x87ffff); io.Pause();
		if (io.ResetRead() != 0x87ffff || io.ResetRead() != 0x87ffff) { r.state = TT_MATMUL_STOP_FAILED; break; }
		io.MathResetWrite(0x47800); io.Pause();
		r.reset_after = io.ResetRead();
		r.stopped = r.reset_after == 0x47800 && io.ResetRead() == 0x47800;
		if (!r.stopped) r.state = TT_MATMUL_STOP_FAILED;
	} while (false);

	bool sourceOkay = BlackholeDramSourceStable(io,baseline);
	if (!sourceOkay) r.state = TT_MATMUL_SOURCE_CHANGED;
	bool canRestore = modified && sourceOkay && r.state != TT_MATMUL_CORE_CHANGED
		&& (!r.started || (r.completed && r.stopped));
	if (canRestore && io.ResetRead() != 0x47800) canRestore = false;
	if (canRestore) {
		for (unsigned attempt = 0; attempt < 2 && !r.l1_restored; attempt++) {
			if (io.ResetRead() != 0x47800) break;
			for (unsigned i = 1; i < TT_TENSIX_IMAGE_WORDS; i++) io.MathImageWrite(i,saved[i]);
			io.MathImageWrite(0,saved[0]);
			r.l1_restored = io.ResetRead() == 0x47800;
			for (unsigned i = 0; i < TT_TENSIX_IMAGE_WORDS; i++) r.l1_restored &= io.MathImageRead(i) == saved[i];
			r.l1_restored &= io.ResetRead() == 0x47800;
		}
	}
	for (unsigned attempt = 0; attempt < 2 && !r.tlb_restored; attempt++) {
		for (unsigned slot = 0; slot < 2; slot++) for (unsigned word = 0; word < 3; word++) io.DramTlbWrite(slot,word,r.tlb_before[slot][word]);
		r.tlb_restored = 1;
		for (unsigned slot = 0; slot < 2; slot++) for (unsigned word = 0; word < 3; word++) {
			r.tlb_after[slot][word] = io.DramTlbRead(slot,word);
			r.tlb_restored &= r.tlb_after[slot][word] == r.tlb_before[slot][word];
		}
	}
	if (!sourceOkay || !r.l1_restored || !r.tlb_restored || (r.started && (!r.completed || !r.stopped))) r.recovery_required = 1;
	if ((!r.l1_restored || !r.tlb_restored) && r.state == TT_MATMUL_PASSED) r.state = TT_MATMUL_RESTORE_FAILED;
	return r.state == TT_MATMUL_PASSED && r.started && r.completed && r.stopped
		&& r.l1_restored && r.tlb_restored && !r.recovery_required;
}
#endif
