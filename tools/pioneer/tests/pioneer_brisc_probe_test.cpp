/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstring>
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeBriscSmokeProbe.h"

struct MockBrisc {
	uint32_t tlb[2][3] = {{1, 2, 3}, {4, 5, 6}};
	uint32_t memory[TT_BRISC_IMAGE_WORDS], original[TT_BRISC_IMAGE_WORDS];
	uint32_t reset = 0x47800;
	unsigned configWrites = 0, imageWrites = 0, resetWrites = 0, resetReads = 0;
	unsigned pauses = 0, pcReads = 0, startPolls = 0;
	unsigned dropConfig = 0, dropImage = 0, dropReset = 0, changeResetAt = 0, transientResetAt = 0;
	bool timeout = false, badResult = false, badPC = false, failRestore = false;
	bool everStarted = false, executed = false;
	MockBrisc() { for (unsigned i = 0; i < TT_BRISC_IMAGE_WORDS; i++) memory[i] = original[i] = 0xabab1234 ^ i; }
	uint32_t BriscTlbRead(unsigned s, unsigned w) { assert(s < 2 && w < 3); return tlb[s][w]; }
	void BriscTlbWrite(unsigned s, unsigned w, uint32_t value)
	{
		assert(s < 2 && w < 3);
		if (++configWrites != dropConfig) tlb[s][w] = value;
	}
	void checkWindows()
	{
		for (unsigned s = 0; s < 2; s++)
			for (unsigned w = 0; w < 3; w++) assert(tlb[s][w] == BlackholeBriscTlbWord(s, w));
	}
	uint32_t ResetRead()
	{
		checkWindows();
		if (++resetReads == transientResetAt) return 0;
		if (resetReads == changeResetAt) reset = 0; // unknown running cores
		if (reset == 0x47000 && ++startPolls == 2 && !timeout) {
			memory[20] = badResult ? 529 : 528; memory[21] = 0x50494f4e; executed = true;
		}
		return reset;
	}
	void ResetWrite(uint32_t value)
	{
		checkWindows();
		assert(value == 0x47000 || value == 0x47800);
		if (value == 0x47000) {
			assert(!everStarted && reset == 0x47800);
			for (unsigned i = 0; i < TT_BRISC_IMAGE_WORDS; i++) assert(memory[i] == BlackholeBriscImageWord(i));
			everStarted = true;
		} else assert(reset == 0x47000);
		if (++resetWrites != dropReset) reset = value;
	}
	uint32_t ImageRead(unsigned i) { checkWindows(); assert(i < TT_BRISC_IMAGE_WORDS); return memory[i]; }
	void ImageWrite(unsigned i, uint32_t value)
	{
		checkWindows(); assert(i < TT_BRISC_IMAGE_WORDS && reset == 0x47800);
		// The production policy must NEVER restore while execution is unknown.
		if (everStarted) assert(executed && !timeout && !badResult && !badPC);
		imageWrites++;
		if (imageWrites != dropImage && !(failRestore && value == original[i])) memory[i] = value;
	}
	void Pause() { checkWindows(); assert(++pauses < 512); }
	uint32_t PcRead() { checkWindows(); pcReads++; return badPC ? 0x70000 : 0x70048; }
};
static tt_tensix_reset_result held()
{
	tt_tensix_reset_result c = {}; c.version = 1; c.state = TT_TENSIX_RESET_HELD; c.tlb_restored = 1;
	for (unsigned i = 0; i < 3; i++) c.reset[i] = 0x47800;
	return c;
}
int main()
{
	auto core = held(); tt_brisc_smoke_result r;
	MockBrisc io;
	assert(BlackholeBriscSmokeProbe(io, core, r));
	assert(r.started && r.completed && r.stopped && r.l1_restored && r.tlb_restored && !r.recovery_required);
	assert(r.checksum == 528 && r.completion == 0x50494f4e && r.polls == 2);
	assert(io.imageWrites == 44 && io.resetWrites == 2 && io.pcReads == 2);
	assert(memcmp(io.memory, io.original, sizeof(io.memory)) == 0 && io.reset == 0x47800);
	assert(io.tlb[0][0] == 1 && io.tlb[1][2] == 6);
	for (unsigned drop = 1; drop <= 12; drop++) {
		io = {}; io.dropConfig = drop;
		assert(!BlackholeBriscSmokeProbe(io, core, r));
		if (drop <= 6) assert(io.imageWrites == 0 && io.resetWrites == 0 && r.tlb_restored);
		else assert(!r.tlb_restored && r.recovery_required && r.l1_restored);
	}
	for (unsigned drop = 1; drop <= 44; drop++) {
		io = {}; io.dropImage = drop;
		bool okay = BlackholeBriscSmokeProbe(io, core, r);
		if (drop <= 22) assert(!okay && r.state == TT_BRISC_IMAGE_FAILED && io.resetWrites == 0);
		else assert(okay && io.imageWrites == 66);
		assert(r.l1_restored && r.tlb_restored && !r.recovery_required);
		assert(memcmp(io.memory, io.original, sizeof(io.memory)) == 0);
	}
	for (unsigned drop = 1; drop <= 2; drop++) {
		io = {}; io.dropReset = drop;
		assert(!BlackholeBriscSmokeProbe(io, core, r));
		assert(r.recovery_required && !r.l1_restored && r.tlb_restored && io.imageWrites == 22);
	}
	io = {}; io.timeout = true;
	assert(!BlackholeBriscSmokeProbe(io, core, r) && r.polls == 128 && r.stopped && !r.completed);
	assert(r.recovery_required && io.imageWrites == 22 && io.resetWrites == 2);
	io = {}; io.badResult = true;
	assert(!BlackholeBriscSmokeProbe(io, core, r) && r.recovery_required && io.imageWrites == 22);
	io = {}; io.badPC = true;
	assert(!BlackholeBriscSmokeProbe(io, core, r) && r.recovery_required && io.imageWrites == 22);
	io = {}; io.failRestore = true;
	assert(!BlackholeBriscSmokeProbe(io, core, r) && r.recovery_required && !r.l1_restored && io.imageWrites == 66);
	for (unsigned change = 1; change <= 14; change++) {
		io = {}; io.changeResetAt = change;
		assert(!BlackholeBriscSmokeProbe(io, core, r));
		assert(r.tlb_restored);
		if (io.imageWrites != 0) assert(!r.l1_restored && r.recovery_required);
	}
	io = {}; core.reset[2] = 0;
	assert(!BlackholeBriscSmokeProbe(io, core, r) && io.configWrites == 0);
	io = {}; core = held(); io.transientResetAt = 4;
	assert(!BlackholeBriscSmokeProbe(io, core, r) && r.state == TT_BRISC_CORE_CHANGED);
	assert(r.recovery_required && io.imageWrites == 21 && io.resetWrites == 0);
	io = {}; core = held(); io.tlb[0][2] = 0x4000;
	assert(!BlackholeBriscSmokeProbe(io, core, r) && io.configWrites == 0);
}
