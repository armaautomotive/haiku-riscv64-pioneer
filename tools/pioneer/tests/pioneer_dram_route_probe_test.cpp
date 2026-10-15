/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstring>
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeDramRouteProbe.h"

struct MockDramRoute {
	uint32_t source[TT_DRAM_SOURCE_WORDS] = {0x6005};
	uint32_t id[TT_DRAM_ID_WORDS] = {0x1c611000, 0x80000, 0xe005, 0x312};
	uint32_t tlb[3] = {1, 2, 3};
	unsigned sourceReads = 0, idReads = 0, writes = 0, drop = 0, sourceChange = 0, idChange = 0;
	bool failRestore = false;
	uint32_t SourceRead(unsigned word) {
		assert(word < TT_DRAM_SOURCE_WORDS);
		sourceReads++;
		return source[word] ^ (sourceReads == sourceChange ? 1U : 0U);
	}
	uint32_t TlbRead(unsigned word) { assert(word < 3); return tlb[word]; }
	void TlbWrite(unsigned word, uint32_t value) {
		assert(word < 3);
		if (++writes != drop && !(failRestore && value == word + 1)) tlb[word] = value;
	}
	uint32_t DramIdRead(unsigned word) {
		assert(word < TT_DRAM_ID_WORDS);
		for (unsigned i = 0; i < 3; i++) assert(tlb[i] == BlackholeDramRouteTlbWord(i));
		return id[word] ^ (++idReads == idChange ? 1U : 0U);
	}
};
static tt_arc_telemetry_result telemetry()
{
	tt_arc_telemetry_result t = {}; t.version = 1; t.state = TT_ARC_TELEMETRY_OK;
	t.tlb_restored = 1; t.present_mask = 0xfff;
	t.values[0] = 0x12050001; t.values[6] = 0xf7; t.values[7] = 0x5515;
	return t;
}
static tt_tensix_reset_result core()
{
	tt_tensix_reset_result c = {}; c.version = 1; c.state = TT_TENSIX_RESET_HELD;
	c.tlb_restored = 1; c.niu_config = 0x6005;
	for (unsigned i = 0; i < 3; i++) c.reset[i] = 0x47800;
	return c;
}
int main()
{
	auto t = telemetry(); auto c = core(); tt_dram_route_result r;
	MockDramRoute io;
	assert(BlackholeDramRouteProbe(io, t, c, r));
	assert(r.state == TT_DRAM_ROUTE_PASSED && r.tlb_restored && !r.recovery_required);
	assert(io.sourceReads == 32 && io.idReads == 12 && io.writes == 6);
	for (unsigned i = 0; i < 3; i++) assert(io.tlb[i] == i + 1);
	io = {}; io.id[3] = 0; // measured 18.5 firmware: unset software label
	assert(BlackholeDramRouteProbe(io, t, c, r));
	assert(BlackholeDramRouteTlbWord(0) == 0x7fd && BlackholeDramRouteTlbWord(1) == 0x89189000);
	assert(BlackholeDramRouteTlbWord(2) == 0xa41 && BlackholeDramRouteTlbWord(3) == 0);
	assert(BlackholeDramSourceOffset(16) == UINT32_MAX && BlackholeDramIdOffset(4) == UINT32_MAX);
	for (unsigned drop = 1; drop <= 6; drop++) {
		io = {}; io.drop = drop;
		bool okay = BlackholeDramRouteProbe(io, t, c, r);
		assert(okay == (drop > 3));
		assert(r.tlb_restored && !r.recovery_required);
		if (drop <= 3) assert(io.idReads == 0);
		else assert(io.writes == 9);
	}
	io = {}; io.failRestore = true;
	assert(!BlackholeDramRouteProbe(io, t, c, r) && !r.tlb_restored && r.recovery_required && io.writes == 9);
	for (unsigned i = 17; i <= 32; i++) {
		io = {}; io.sourceChange = i;
		assert(!BlackholeDramRouteProbe(io, t, c, r) && r.tlb_restored && r.recovery_required);
	}
	for (unsigned i = 1; i <= 12; i++) {
		io = {}; io.idChange = i;
		assert(!BlackholeDramRouteProbe(io, t, c, r) && r.state == TT_DRAM_ROUTE_BAD_ID && r.tlb_restored);
	}
	io = {}; memset(io.id, 0, sizeof(io.id));
	assert(!BlackholeDramRouteProbe(io, t, c, r) && r.state == TT_DRAM_ROUTE_BAD_ID);
	for (unsigned word = 0; word < TT_DRAM_ID_WORDS; word++) {
		io = {}; io.id[word] = UINT32_MAX;
		assert(!BlackholeDramRouteProbe(io, t, c, r) && r.tlb_restored);
	}
	for (unsigned bit = 0; bit < 4; bit++) {
		io = {};
		if (bit == 0) io.source[0] &= ~0x4000U;
		if (bit == 1) io.source[15] |= 0x400;
		if (bit == 2) io.source[13] |= 1U << 18;
		if (bit == 3) io.source[14] |= 1U << 12;
		assert(!BlackholeDramRouteProbe(io, t, c, r) && io.writes == 0 && io.idReads == 0);
	}
	io = {}; io.source[4] = 1; // translated x=18 would reach physical x=1
	assert(!BlackholeDramRouteProbe(io, t, c, r) && io.writes == 0);
	io = {}; io.source[9] = 1; // translated y=12 would reach physical y=1
	assert(!BlackholeDramRouteProbe(io, t, c, r) && io.writes == 0);
	io = {}; t.values[6] = 0xff;
	assert(!BlackholeDramRouteProbe(io, t, c, r) && io.sourceReads == 0 && io.writes == 0);
	t = telemetry(); c.reset[1] = 0;
	assert(!BlackholeDramRouteProbe(io, t, c, r) && io.sourceReads == 0 && io.writes == 0);
}
