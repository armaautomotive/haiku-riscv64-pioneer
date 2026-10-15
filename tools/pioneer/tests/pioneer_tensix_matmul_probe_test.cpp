/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstring>
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeTensixMatmulProbe.h"

struct MockMatmul {
	uint32_t source[TT_DRAM_SOURCE_WORDS] = {
		0x6005,0x0a418820,0x1ac4a166,0x127541ee,0x1e280440,0x1662b5c3,0x147,
		0x0a418820,0x16a4a0e6,0x06a12c20,0x0c72a089,0x0e548c20,2,0,3,0
	};
	uint32_t tlb[2][3] = {{1,2,3},{4,5,6}}, controls[3] = {};
	uint32_t memory[TT_TENSIX_IMAGE_WORDS], original[TT_TENSIX_IMAGE_WORDS], reset = 0x47800;
	unsigned configWrites = 0, imageWrites = 0, resetWrites = 0, resetReads = 0, sourceReads = 0, polls = 0, pauses = 0;
	unsigned dropConfig = 0, dropImage = 0, dropReset = 0, changeResetAt = 0;
	bool timeout = false, badPC = false, badMath = false, failRestore = false, sourceChanges = false, failTlbRestore = false;
	bool everStarted = false, executed = false;
	MockMatmul() { for (unsigned i = 0; i < TT_TENSIX_IMAGE_WORDS; i++) memory[i] = original[i] = 0xabcd0000 ^ i; }
	uint32_t SourceRead(unsigned word) {
		assert(word < TT_DRAM_SOURCE_WORDS); sourceReads++;
		return source[word] ^ ((sourceChanges && sourceReads > 16) ? 1U : 0U);
	}
	uint32_t DramTlbRead(unsigned slot, unsigned word) { assert(slot < 2 && word < 3); return tlb[slot][word]; }
	void DramTlbWrite(unsigned slot, unsigned word, uint32_t value) {
		assert(slot < 2 && word < 3);
		if (++configWrites != dropConfig && !(failTlbRestore && value == slot*3+word+1)) tlb[slot][word] = value;
	}
	bool Matches(unsigned slot, uint32_t base, unsigned x, unsigned y, bool ro) {
		for (unsigned w = 0; w < 3; w++) if (tlb[slot][w] != BlackholeDramFixedTlbWord(base,x,y,ro,w)) return false;
		return true;
	}
	void Windows() {
		for (unsigned s = 0; s < 2; s++) for (unsigned w = 0; w < 3; w++) assert(tlb[s][w] == BlackholeMatmulTlbWord(s,w));
	}
	uint32_t ResetRead() {
		bool valid = Matches(1,0xffa00000,1,2,false);
		const unsigned xs[] = {1,2,3,4,5,6,7,10,11,12,13,14};
		for (unsigned x : xs) for (unsigned y = 2; y <= 11; y++) valid |= Matches(1,0xffa00000,x,y,true);
		assert(valid);
		if (++resetReads == changeResetAt) reset = 0;
		if (reset == 0x46800 && ++polls == 2 && !timeout) {
			memory[400] = 1; memory[401] = 2; memory[402] = 0x4d415431;
			for (unsigned i = 0; i < TT_TENSIX_OUTPUT_WORDS; i++) memory[144+i] = BlackholeMatmulExpected(i);
			if (badMath) memory[144] ^= 1;
			executed = true;
		}
		return reset;
	}
	uint32_t MathControlRead(unsigned word) { Windows(); assert(word < 3); return controls[word]; }
	uint32_t MathImageRead(unsigned word) { Windows(); assert(word < TT_TENSIX_IMAGE_WORDS); return memory[word]; }
	void MathImageWrite(unsigned word, uint32_t value) {
		Windows(); assert(word < TT_TENSIX_IMAGE_WORDS && reset == 0x47800);
		if (everStarted) assert(executed && !timeout && !badPC && !sourceChanges);
		if (++imageWrites != dropImage && !(failRestore && value == original[word])) memory[word] = value;
	}
	void MathResetWrite(uint32_t value) {
		Windows();
		if (value == 0x87ffff) assert(reset == (everStarted ? 0x46800U : 0x47800U));
		else if (value == 0x46800) {
			assert(reset == 0x87ffff && !everStarted);
			for (unsigned i = 0; i < TT_TENSIX_IMAGE_WORDS; i++) assert(memory[i] == BlackholeTensixImageWord(i));
			everStarted = true;
		} else { assert(value == 0x47800 && reset == 0x87ffff && everStarted); }
		if (++resetWrites != dropReset) reset = value;
	}
	uint32_t MathPcRead() { Windows(); return badPC ? 0x70000 : TT_TENSIX_PARK_PC; }
	void Pause() { assert(++pauses < 10000); }
	bool Restored() { return memcmp(memory,original,sizeof(memory)) == 0; }
	tt_dram_samples_result Baseline() {
		tt_dram_samples_result b = {}; b.version = 1; b.state = TT_DRAM_SAMPLE_ROUTES_OK;
		b.routes_verified = 7; b.data_restored = b.tlb_restored = 1;
		memcpy(b.source,source,sizeof(source)); return b;
	}
};
int main()
{
	MockMatmul io; auto baseline = io.Baseline(); tt_tensix_matmul_result r;
	assert(BlackholeTensixMatmulProbe(io,baseline,r));
	assert(r.state == TT_MATMUL_PASSED && r.context_reset && r.started && r.completed && r.stopped);
	assert(r.l1_restored && r.tlb_restored && !r.recovery_required && r.mismatches == 0 && io.Restored());
	assert(baseline.cores_checked == 120 && io.imageWrites == 806 && io.resetWrites == 4);
	unsigned configWrites = io.configWrites;
	for (unsigned drop = 1; drop <= configWrites; drop++) {
		io = {}; baseline = io.Baseline(); io.dropConfig = drop;
		bool okay = BlackholeTensixMatmulProbe(io,baseline,r);
		assert(r.l1_restored && r.tlb_restored && !r.recovery_required && io.Restored());
		if (okay) assert(r.completed && r.stopped);
	}
	for (unsigned drop = 1; drop <= 806; drop++) {
		io = {}; baseline = io.Baseline(); io.dropImage = drop;
		bool okay = BlackholeTensixMatmulProbe(io,baseline,r);
		assert(r.l1_restored && r.tlb_restored && !r.recovery_required && io.Restored());
		if (drop <= 403) assert(!okay && io.resetWrites == 0);
		else assert(okay && io.imageWrites == 1209);
	}
	for (unsigned drop = 1; drop <= 4; drop++) {
		io = {}; baseline = io.Baseline(); io.dropReset = drop;
		assert(!BlackholeTensixMatmulProbe(io,baseline,r) && r.recovery_required && !r.l1_restored && io.imageWrites == 403);
	}
	io = {}; baseline = io.Baseline(); io.timeout = true;
	assert(!BlackholeTensixMatmulProbe(io,baseline,r) && r.polls == 2000 && r.stopped && !r.completed && r.recovery_required);
	assert(io.imageWrites == 403);
	io = {}; baseline = io.Baseline(); io.badPC = true;
	assert(!BlackholeTensixMatmulProbe(io,baseline,r) && r.stopped && !r.completed && r.recovery_required);
	io = {}; baseline = io.Baseline(); io.badMath = true;
	assert(!BlackholeTensixMatmulProbe(io,baseline,r) && r.completed && r.stopped && r.mismatches == 1 && io.Restored() && !r.recovery_required);
	io = {}; baseline = io.Baseline(); io.sourceChanges = true;
	assert(!BlackholeTensixMatmulProbe(io,baseline,r) && r.recovery_required && !r.stopped && io.resetWrites == 2 && io.imageWrites == 403);
	io = {}; baseline = io.Baseline(); io.failRestore = true;
	assert(!BlackholeTensixMatmulProbe(io,baseline,r) && r.recovery_required && !r.l1_restored && io.imageWrites == 1209);
	io = {}; baseline = io.Baseline(); io.failTlbRestore = true;
	assert(!BlackholeTensixMatmulProbe(io,baseline,r) && r.recovery_required && r.l1_restored && !r.tlb_restored);
	for (unsigned control = 0; control < 3; control++) {
		io = {}; baseline = io.Baseline(); io.controls[control] = 1;
		assert(!BlackholeTensixMatmulProbe(io,baseline,r) && io.imageWrites == 0 && io.resetWrites == 0 && r.tlb_restored);
	}
	io = {}; baseline = io.Baseline(); baseline.routes_verified = 6;
	assert(!BlackholeTensixMatmulProbe(io,baseline,r) && io.configWrites == 0);
}
