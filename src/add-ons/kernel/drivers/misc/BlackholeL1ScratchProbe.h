/* Distributed under the MIT License. */
#ifndef BLACKHOLE_L1_SCRATCH_PROBE_H
#define BLACKHOLE_L1_SCRATCH_PROBE_H

#include "BlackholeTensixResetProbe.h"

enum {
	TT_L1_REFUSED = 0,
	TT_L1_PASSED = 1,
	TT_L1_CONFIG_FAILED = 2,
	TT_L1_CORE_CHANGED = 3,
	TT_L1_DATA_MISMATCH = 4,
	TT_L1_RESTORE_FAILED = 5,
	TT_L1_WORDS = 16
};

struct tt_l1_scratch_result {
	uint32_t version, state, tlb_restored, data_restored;
	uint32_t tlb_before[2][3], tlb_trial[2][3], tlb_after[2][3];
	uint32_t reset_before, reset_after, mismatch_mask, passes;
};

static inline uint32_t
BlackholeL1TlbWord(unsigned slot, unsigned word)
{
	if (slot > 1 || word > 2) return 0;
	// Window 200: local zero, strict BIDIRECTIONAL/DYNAMIC VC for data.
	// Window 201: local ffa00000, strict READ-ONLY/static VC for reset CSR.
	if (word == 0) return slot == 0 ? 0 : 0x7fd;
	if (word == 1) return 0x40840800; // Only enabled tile (1,2).
	return slot == 0 ? 0x40 : 0xa40;
}

static inline uint32_t
BlackholeL1Pattern(unsigned pass, unsigned word)
{
	return (pass == 0 ? 0xa5a55a5aU : 0x5a5aa5a5U) ^ (0x01010101U * word);
}

template<class IO>
bool
BlackholeL1ScratchProbe(IO& io, const tt_tensix_reset_result& core,
	tt_l1_scratch_result& r)
{
	r = {};
	r.version = 1;
	r.tlb_restored = r.data_restored = 1;
	if (core.version != 1 || core.state != TT_TENSIX_RESET_HELD
		|| !core.tlb_restored || core.reset[0] == UINT32_MAX
		|| core.reset[0] != core.reset[1] || core.reset[1] != core.reset[2]
		|| (core.reset[0] & 0x47800) != 0x47800)
		return false;
	for (unsigned s = 0; s < 2; s++) {
		for (unsigned w = 0; w < 3; w++) r.tlb_before[s][w] = io.L1TlbRead(s, w);
		if ((r.tlb_before[s][2] & 0xffffc000) != 0) return false;
	}
	r.tlb_restored = 0;
	for (unsigned s = 0; s < 2; s++)
		for (unsigned w = 0; w < 3; w++) io.L1TlbWrite(s, w, BlackholeL1TlbWord(s, w));
	bool configured = true;
	for (unsigned s = 0; s < 2; s++) {
		for (unsigned w = 0; w < 3; w++) {
			r.tlb_trial[s][w] = io.L1TlbRead(s, w);
			configured &= r.tlb_trial[s][w] == BlackholeL1TlbWord(s, w);
		}
	}
	r.state = TT_L1_CONFIG_FAILED;
	if (configured) {
		r.reset_before = io.ResetRead();
		r.state = TT_L1_CORE_CHANGED;
		if (r.reset_before == core.reset[0]) {
			// The final 64 bytes of this stopped tile's 1.5 MiB L1 only.
			// Original contents stay kernel-local and never enter the ioctl.
			uint32_t saved[TT_L1_WORDS];
			for (unsigned i = 0; i < TT_L1_WORDS; i++) saved[i] = io.L1Read(i);
			bool wrote = false;
			r.state = TT_L1_PASSED;
			for (unsigned pass = 0; pass < 2; pass++) {
				if (io.ResetRead() != core.reset[0]) { r.state = TT_L1_CORE_CHANGED; break; }
				wrote = true;
				r.data_restored = 0;
				for (unsigned i = 0; i < TT_L1_WORDS; i++) io.L1Write(i, BlackholeL1Pattern(pass, i));
				for (unsigned i = 0; i < TT_L1_WORDS; i++) {
					if (io.L1Read(i) != BlackholeL1Pattern(pass, i)) r.mismatch_mask |= 1U << (pass * 16 + i);
				}
				r.passes++;
				if (r.mismatch_mask != 0) { r.state = TT_L1_DATA_MISMATCH; break; }
			}
			r.reset_after = io.ResetRead();
			if (r.reset_after != core.reset[0]) r.state = TT_L1_CORE_CHANGED;
			if (wrote) {
				// At most two restoration attempts, before changing either TLB.
				for (unsigned attempt = 0; attempt < 2 && !r.data_restored; attempt++) {
					for (unsigned i = 0; i < TT_L1_WORDS; i++) io.L1Write(i, saved[i]);
					r.data_restored = 1;
					for (unsigned i = 0; i < TT_L1_WORDS; i++) r.data_restored &= io.L1Read(i) == saved[i];
				}
			}
		}
	}
	for (unsigned s = 0; s < 2; s++)
		for (unsigned w = 0; w < 3; w++) io.L1TlbWrite(s, w, r.tlb_before[s][w]);
	r.tlb_restored = 1;
	for (unsigned s = 0; s < 2; s++) {
		for (unsigned w = 0; w < 3; w++) {
			r.tlb_after[s][w] = io.L1TlbRead(s, w);
			r.tlb_restored &= r.tlb_after[s][w] == r.tlb_before[s][w];
		}
	}
	if (!r.tlb_restored || !r.data_restored) r.state = TT_L1_RESTORE_FAILED;
	return r.state == TT_L1_PASSED && r.tlb_restored && r.data_restored && r.passes == 2;
}

#endif
