/* Distributed under the MIT License. */
#include <cassert>
#include <cstring>
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeTensixResetProbe.h"

struct MockReset {
	uint32_t tlb[3] = {}, niu = 0, reset = 0x47800;
	unsigned writes = 0, reads = 0, drop = 0;
	bool change = false;
	uint32_t NiuRead() { return niu; }
	uint32_t TlbRead(unsigned i) { assert(i < 3); return tlb[i]; }
	void TlbWrite(unsigned i, uint32_t v) { assert(i < 3); if (++writes != drop) tlb[i] = v; }
	uint32_t ResetRead()
	{
		for (unsigned i = 0; i < 3; i++) assert(tlb[i] == BlackholeTensixResetTlbWord(i));
		reads++;
		return change && reads == 3 ? reset ^ 0x800 : reset;
	}
};

static tt_arc_telemetry_result GoodTelemetry()
{
	tt_arc_telemetry_result t = {};
	t.version = 1; t.state = TT_ARC_TELEMETRY_OK; t.tlb_restored = 1;
	t.present_mask = 0xfff; t.values[0] = 0x12050001; t.values[9] = 0xfff;
	return t;
}

int main()
{
	// Independent full-width decode of the three hardware words.
	__uint128_t v = (__uint128_t)BlackholeTensixResetTlbWord(0)
		| ((__uint128_t)BlackholeTensixResetTlbWord(1) << 32)
		| ((__uint128_t)BlackholeTensixResetTlbWord(2) << 64);
	assert((uint64_t)(v & (((__uint128_t)1 << 43) - 1)) << 21 == 0xffa00000);
	assert(((v >> 43) & 63) == 1 && ((v >> 49) & 63) == 2);
	assert(((v >> 55) & 63) == 1 && ((v >> 61) & 63) == 2);
	assert(((v >> 67) & 7) == 0 && ((v >> 70) & 3) == 1);
	assert(((v >> 73) & 1) == 1 && ((v >> 75) & 1) == 1 && (v >> 76) == 0);
	MockReset io;
	tt_tensix_reset_result r;
	auto t = GoodTelemetry();
	assert(BlackholeTensixResetProbe(io, t, r) && r.tlb_restored && io.writes == 6 && io.reads == 3);
	assert(memcmp(r.tlb_before, r.tlb_after, sizeof(r.tlb_before)) == 0);
	for (unsigned drop = 1; drop <= 6; drop++) {
		io = {}; io.drop = drop;
		assert(!BlackholeTensixResetProbe(io, t, r));
		if (drop <= 3) assert(r.tlb_restored && io.reads == 0);
		else assert(!r.tlb_restored && r.state == TT_TENSIX_RESET_RESTORE_FAILED);
	}
	io = {}; io.niu = 1 << 14;
	assert(BlackholeTensixResetProbe(io, t, r) && r.tlb_restored && io.reads == 3);
	io = {}; io.niu = UINT32_MAX;
	assert(!BlackholeTensixResetProbe(io, t, r) && io.writes == 0);
	io = {}; t.values[9] &= ~1;
	assert(!BlackholeTensixResetProbe(io, t, r) && io.writes == 0);
	t = GoodTelemetry(); io = {}; t.present_mask &= ~0x200;
	assert(!BlackholeTensixResetProbe(io, t, r) && io.writes == 0);
	t = GoodTelemetry(); io = {}; t.values[0]++;
	assert(!BlackholeTensixResetProbe(io, t, r) && io.writes == 0);
	t = GoodTelemetry(); io = {}; io.tlb[2] = 1 << 14;
	assert(!BlackholeTensixResetProbe(io, t, r) && io.writes == 0);
	io = {}; io.reset = UINT32_MAX;
	assert(!BlackholeTensixResetProbe(io, t, r) && r.tlb_restored);
	io = {}; io.reset = 0;
	assert(!BlackholeTensixResetProbe(io, t, r) && r.tlb_restored);
	io = {}; io.change = true;
	assert(!BlackholeTensixResetProbe(io, t, r) && r.tlb_restored);
	io = {}; io.tlb[0] = 11; io.tlb[1] = 12; io.tlb[2] = 13;
	assert(BlackholeTensixResetProbe(io, t, r) && r.tlb_restored);
	assert(io.tlb[0] == 11 && io.tlb[1] == 12 && io.tlb[2] == 13);
}
