/* Distributed under the MIT License. */
#include <cassert>
#include <cstring>
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeL1ScratchProbe.h"

struct MockL1 {
	uint32_t tlb[2][3] = {}, memory[16] = {}, original[16] = {};
	unsigned configWrites = 0, dataWrites = 0, dataReads = 0, resetReads = 0;
	unsigned dropConfig = 0, dropData = 0, changeReset = 0;
	bool failRestore = false;
	MockL1() { for (unsigned i = 0; i < 16; i++) memory[i] = original[i] = 0x12345678 ^ i; }
	uint32_t L1TlbRead(unsigned s, unsigned w) { assert(s < 2 && w < 3); return tlb[s][w]; }
	void L1TlbWrite(unsigned s, unsigned w, uint32_t v)
	{
		assert(s < 2 && w < 3);
		if (++configWrites != dropConfig) tlb[s][w] = v;
	}
	void CheckWindows()
	{
		for (unsigned s = 0; s < 2; s++)
			for (unsigned w = 0; w < 3; w++) assert(tlb[s][w] == BlackholeL1TlbWord(s, w));
	}
	uint32_t ResetRead() { CheckWindows(); return ++resetReads == changeReset ? 0 : 0x47800; }
	uint32_t L1Read(unsigned i) { assert(i < 16); CheckWindows(); dataReads++; return memory[i]; }
	void L1Write(unsigned i, uint32_t v)
	{
		assert(i < 16); CheckWindows(); dataWrites++;
		if (dataWrites != dropData && !(failRestore && v == original[i])) memory[i] = v;
	}
};

static tt_tensix_reset_result HeldCore()
{
	tt_tensix_reset_result c = {};
	c.version = 1; c.state = TT_TENSIX_RESET_HELD; c.tlb_restored = 1;
	for (unsigned i = 0; i < 3; i++) c.reset[i] = 0x47800;
	return c;
}

int main()
{
	assert((BlackholeL1TlbWord(0, 2) & (1 << 9)) == 0); // Dynamic VC for mixed data traffic.
	assert(BlackholeL1TlbWord(1, 0) == 0x7fd && BlackholeL1TlbWord(1, 2) == 0xa40);
	MockL1 io;
	tt_l1_scratch_result r;
	auto core = HeldCore();
	assert(BlackholeL1ScratchProbe(io, core, r));
	assert(r.passes == 2 && r.mismatch_mask == 0 && r.data_restored && r.tlb_restored);
	assert(io.configWrites == 12 && io.dataWrites == 48 && io.dataReads == 64 && io.resetReads == 4);
	assert(memcmp(io.memory, io.original, sizeof(io.memory)) == 0);
	for (unsigned drop = 1; drop <= 12; drop++) {
		io = {}; io.dropConfig = drop;
		// Every programmed/restored word must differ to detect its loss.
		for (unsigned s = 0; s < 2; s++)
			for (unsigned w = 0; w < 3; w++) io.tlb[s][w] = 11 + s * 3 + w;
		assert(!BlackholeL1ScratchProbe(io, core, r));
		if (drop <= 6) assert(io.dataReads == 0 && io.dataWrites == 0 && r.tlb_restored);
		else assert(r.state == TT_L1_RESTORE_FAILED && !r.tlb_restored && r.data_restored);
	}
	for (unsigned drop = 1; drop <= 48; drop++) {
		io = {}; io.dropData = drop;
		bool passed = BlackholeL1ScratchProbe(io, core, r);
		if (drop <= 32) assert(!passed && r.state == TT_L1_DATA_MISMATCH);
		else assert(passed && io.dataWrites == 64); // One bounded restore retry.
		assert(r.data_restored && r.tlb_restored && memcmp(io.memory, io.original, sizeof(io.memory)) == 0);
	}
	for (unsigned change = 1; change <= 4; change++) {
		io = {}; io.changeReset = change;
		assert(!BlackholeL1ScratchProbe(io, core, r) && r.state == TT_L1_CORE_CHANGED);
		assert(r.tlb_restored && r.data_restored && memcmp(io.memory, io.original, sizeof(io.memory)) == 0);
		if (change <= 2) assert(io.dataWrites == 0);
	}
	io = {}; io.failRestore = true;
	assert(!BlackholeL1ScratchProbe(io, core, r) && r.state == TT_L1_RESTORE_FAILED);
	assert(!r.data_restored && r.tlb_restored && io.dataWrites == 64);
	io = {}; core.reset[0] = 0;
	assert(!BlackholeL1ScratchProbe(io, core, r) && io.configWrites == 0);
	core = HeldCore(); io = {}; io.tlb[1][2] = 0x4000;
	assert(!BlackholeL1ScratchProbe(io, core, r) && io.configWrites == 0);
	io = {}; io.tlb[0][0] = 11; io.tlb[1][2] = 13;
	assert(BlackholeL1ScratchProbe(io, core, r) && r.tlb_restored && io.tlb[0][0] == 11 && io.tlb[1][2] == 13);
}
