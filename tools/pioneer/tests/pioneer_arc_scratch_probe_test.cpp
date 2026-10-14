/* Distributed under the MIT License. */
#include <cassert>
#include <cstring>
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeArcScratchProbe.h"

struct MockArc {
	uint32_t words[3] = {};
	uint32_t boot = 5, error = 0, data = 0x10002000, tags = 0x10001000;
	unsigned writes = 0, scratchReads = 0, droppedWrite = 0;
	bool changingBoot = false;
	uint32_t TlbRead(unsigned i) { assert(i < 3); return words[i]; }
	void TlbWrite(unsigned i, uint32_t value)
	{
		assert(i < 3);
		if (++writes != droppedWrite) words[i] = value;
	}
	uint32_t ScratchRead(unsigned n)
	{
		for (unsigned i = 0; i < 3; i++) assert(words[i] == BlackholeArcScratchTlbWord(i));
		scratchReads++;
		switch (n) {
			case 2: return changingBoot && scratchReads > 1 ? boot ^ 4 : boot;
			case 4: return error;
			case 11: return 0x10003000;
			case 12: return data;
			case 13: return tags;
			default: assert(false); return 0;
		}
	}
};

int main()
{
	// Independently decode the fixed 96-bit layout (43-bit address, 6-bit
	// coordinates, strict ordering at bit 70, static VC 73, read buddy 75).
	uint64_t low = BlackholeArcScratchTlbWord(0)
		| ((uint64_t)BlackholeArcScratchTlbWord(1) << 32);
	uint32_t high = BlackholeArcScratchTlbWord(2);
	assert((low & ((UINT64_C(1) << 43) - 1)) == (0x80000000U >> 21));
	assert(((low >> 43) & 63) == 8 && ((low >> 49) & 63) == 0);
	assert(((low >> 55) & 63) == 8 && (low >> 61) == 0);
	assert((high & 0x3f) == 0 && ((high >> 6) & 3) == 1);
	assert(((high >> 9) & 1) == 1 && ((high >> 11) & 1) == 1 && (high >> 12) == 0);
	assert(201U * 12 == 0x96c && 201U * 0x200000 == 0x19200000);
	MockArc io;
	tt_arc_scratch_result r;
	assert(BlackholeArcScratchProbe(io, r));
	assert(r.state == TT_ARC_SCRATCH_READY && r.tlb_restored && r.telemetry_present);
	assert(io.writes == 6 && io.scratchReads == 6);
	assert(memcmp(r.tlb_before, r.tlb_after, sizeof(r.tlb_before)) == 0);
	for (unsigned drop = 1; drop <= 6; drop++) {
		io = {}; io.droppedWrite = drop;
		bool okay = BlackholeArcScratchProbe(io, r);
		if (drop <= 3) assert(!okay && io.scratchReads == 0 && r.tlb_restored);
		else assert(!okay && r.state == TT_ARC_SCRATCH_RESTORE_FAILED);
	}
	io = {}; io.words[0] = 0x1234; io.words[1] = 0x5678; io.words[2] = 0x123;
	assert(BlackholeArcScratchProbe(io, r) && r.tlb_restored);
	assert(memcmp(r.tlb_before, r.tlb_after, sizeof(r.tlb_before)) == 0);
	io = {}; io.words[2] = 0xffffffff;
	assert(!BlackholeArcScratchProbe(io, r) && io.writes == 0 && r.tlb_restored);
	io = {}; io.boot = 1;
	assert(!BlackholeArcScratchProbe(io, r) && r.state == TT_ARC_SCRATCH_NOT_READY && r.tlb_restored);
	io = {}; io.error = 2;
	assert(!BlackholeArcScratchProbe(io, r) && r.state == TT_ARC_SCRATCH_NOT_READY && r.tlb_restored);
	io = {}; io.changingBoot = true;
	assert(!BlackholeArcScratchProbe(io, r) && r.tlb_restored);
	io = {}; io.data = 0;
	assert(BlackholeArcScratchProbe(io, r) && !r.telemetry_present && r.tlb_restored);
}
