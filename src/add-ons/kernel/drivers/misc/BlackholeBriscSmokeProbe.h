/* SPDX-License-Identifier: MIT */
#ifndef BLACKHOLE_BRISC_SMOKE_PROBE_H
#define BLACKHOLE_BRISC_SMOKE_PROBE_H
#include "BlackholeTensixResetProbe.h"
#include "BlackholeBriscImage.h"

enum {
	TT_BRISC_REFUSED = 0, TT_BRISC_PASSED = 1, TT_BRISC_CONFIG_FAILED = 2,
	TT_BRISC_CORE_CHANGED = 3, TT_BRISC_IMAGE_FAILED = 4,
	TT_BRISC_COMPLETION_FAILED = 5, TT_BRISC_STOP_FAILED = 6,
	TT_BRISC_RESTORE_FAILED = 7
};
struct tt_brisc_smoke_result {
	uint32_t version, state, started, completed, stopped, l1_restored, tlb_restored;
	uint32_t recovery_required, polls, checksum, completion, pc[2];
	uint32_t reset_before, reset_running, reset_after;
	uint32_t tlb_before[2][3], tlb_trial[2][3], tlb_after[2][3];
};
static inline uint32_t BlackholeBriscTlbWord(unsigned slot, unsigned word)
{
	if (slot > 1 || word > 2) return 0;
	if (word == 0) return slot == 0 ? 0 : 0x7fd;
	if (word == 1) return 0x40840800; // sole allowed Tensix (1,2)
	return 0x40; // strict BIDIRECTIONAL/DYNAMIC VC, including reset writes
}

// IO exposes only fixed image-index, reset-word, PC-snapshot and TLB accesses.
// Failure after start without verified completion/stop leaves image bytes in
// place and requires cold recovery, even if a reset write appears to succeed.
template<class IO>
bool BlackholeBriscSmokeProbe(IO& io, const tt_tensix_reset_result& core,
	tt_brisc_smoke_result& r)
{
	r = {}; r.version = 1; r.l1_restored = r.tlb_restored = 1;
	if (core.version != 1 || core.state != TT_TENSIX_RESET_HELD || !core.tlb_restored
		|| core.reset[0] != 0x47800 || core.reset[1] != 0x47800 || core.reset[2] != 0x47800)
		return false;
	for (unsigned s = 0; s < 2; s++) {
		for (unsigned w = 0; w < 3; w++) r.tlb_before[s][w] = io.BriscTlbRead(s, w);
		if ((r.tlb_before[s][2] & 0xffffc000) != 0) return false;
	}
	r.tlb_restored = 0;
	for (unsigned s = 0; s < 2; s++)
		for (unsigned w = 0; w < 3; w++) io.BriscTlbWrite(s, w, BlackholeBriscTlbWord(s, w));
	bool configured = true;
	for (unsigned s = 0; s < 2; s++) {
		for (unsigned w = 0; w < 3; w++) {
			r.tlb_trial[s][w] = io.BriscTlbRead(s, w);
			configured &= r.tlb_trial[s][w] == BlackholeBriscTlbWord(s, w);
		}
	}
	r.state = TT_BRISC_CONFIG_FAILED;
	uint32_t saved[TT_BRISC_IMAGE_WORDS] = {};
	bool modified = false;
	if (configured) do {
		r.state = TT_BRISC_CORE_CHANGED;
		r.reset_before = io.ResetRead();
		if (r.reset_before != 0x47800 || io.ResetRead() != 0x47800) break;
		for (unsigned i = 0; i < TT_BRISC_IMAGE_WORDS; i++) saved[i] = io.ImageRead(i);
		if (io.ResetRead() != 0x47800) break;
		modified = true; r.l1_restored = 0;
		// Clear stale completion and install code first, reset jump last.
		for (unsigned i = 1; i < TT_BRISC_IMAGE_WORDS; i++) io.ImageWrite(i, BlackholeBriscImageWord(i));
		if (io.ResetRead() != 0x47800) break;
		io.ImageWrite(0, BlackholeBriscImageWord(0));
		r.state = TT_BRISC_IMAGE_FAILED;
		bool imageOkay = true;
		for (unsigned i = 0; i < TT_BRISC_IMAGE_WORDS; i++) imageOkay &= io.ImageRead(i) == BlackholeBriscImageWord(i);
		if (!imageOkay) break;
		r.state = TT_BRISC_CORE_CHANGED;
		if (io.ResetRead() != 0x47800) break;
		// Only BRISC bit 11 changes; the other four RISCs remain held.
		r.started = 1; // set before write: dropped writes still need safe cleanup
		io.ResetWrite(0x47000);
		r.state = TT_BRISC_COMPLETION_FAILED;
		for (unsigned attempt = 0; attempt < 128; attempt++) {
			r.polls++;
			r.reset_running = io.ResetRead();
			if (r.reset_running != 0x47000) { r.state = TT_BRISC_CORE_CHANGED; break; }
			r.checksum = io.ImageRead(20); r.completion = io.ImageRead(21);
			if (r.checksum == 528 && r.completion == 0x50494f4e) {
				io.Pause(); r.pc[0] = io.PcRead();
				io.Pause(); r.pc[1] = io.PcRead();
				// Snapshot alone is speculative; require the drained result words too.
				if (r.pc[0] == 0x70048 && r.pc[1] == 0x70048) { r.completed = 1; break; }
			}
			io.Pause();
		}
		// Never write an unknown reset value or assert reset on another RISC.
		uint32_t current = io.ResetRead();
		if (current == 0x47000) io.ResetWrite(0x47800);
		else if (current != 0x47800) { r.state = TT_BRISC_STOP_FAILED; break; }
		io.Pause();
		r.reset_after = io.ResetRead();
		io.Pause();
		r.stopped = r.reset_after == 0x47800 && io.ResetRead() == 0x47800;
		if (!r.stopped) { r.state = TT_BRISC_STOP_FAILED; break; }
		if (r.completed) r.state = TT_BRISC_PASSED;
	} while (false);

	// Rewriting any code is forbidden after uncertain execution or stop.
	bool canRestore = modified && r.state != TT_BRISC_CORE_CHANGED
		&& (!r.started || (r.completed && r.stopped));
	if (canRestore && io.ResetRead() != 0x47800) { canRestore = false; r.state = TT_BRISC_CORE_CHANGED; }
	if (canRestore) {
		for (unsigned attempt = 0; attempt < 2 && !r.l1_restored; attempt++) {
			if (io.ResetRead() != 0x47800) break;
			for (unsigned i = 1; i < TT_BRISC_IMAGE_WORDS; i++) io.ImageWrite(i, saved[i]);
			io.ImageWrite(0, saved[0]);
			r.l1_restored = io.ResetRead() == 0x47800;
			for (unsigned i = 0; i < TT_BRISC_IMAGE_WORDS; i++) r.l1_restored &= io.ImageRead(i) == saved[i];
			r.l1_restored &= io.ResetRead() == 0x47800;
		}
	}
	if (modified && !r.l1_restored) r.recovery_required = 1;
	for (unsigned s = 0; s < 2; s++)
		for (unsigned w = 0; w < 3; w++) io.BriscTlbWrite(s, w, r.tlb_before[s][w]);
	r.tlb_restored = 1;
	for (unsigned s = 0; s < 2; s++) {
		for (unsigned w = 0; w < 3; w++) {
			r.tlb_after[s][w] = io.BriscTlbRead(s, w);
			r.tlb_restored &= r.tlb_after[s][w] == r.tlb_before[s][w];
		}
	}
	if (!r.tlb_restored) r.recovery_required = 1;
	if (!r.l1_restored || !r.tlb_restored) {
		if (r.state != TT_BRISC_STOP_FAILED && r.state != TT_BRISC_COMPLETION_FAILED
			&& r.state != TT_BRISC_CORE_CHANGED) r.state = TT_BRISC_RESTORE_FAILED;
	}
	return r.state == TT_BRISC_PASSED && r.l1_restored && r.tlb_restored && !r.recovery_required;
}
#endif
