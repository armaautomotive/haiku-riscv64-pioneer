/* Distributed under the MIT License. */
#include <cassert>
#include <cstring>
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeArcTelemetryProbe.h"

static tt_arc_scratch_result GoodArc()
{
	tt_arc_scratch_result a = {};
	a.version = 1; a.state = TT_ARC_SCRATCH_READY; a.tlb_restored = a.telemetry_present = 1;
	a.telemetry_tags = 0x10001000; a.telemetry_data = 0x10002000;
	return a;
}

struct MockTelemetry {
	uint32_t tlb[3] = {};
	uint32_t version = 0x10000, count = TT_ARC_TELEMETRY_COUNT;
	uint32_t entries[128] = {};
	unsigned writes = 0, reads = 0, dataReads = 0, droppedWrite = 0, headerReads = 0;
	bool changing = false;
	MockTelemetry()
	{
		for (unsigned i = 0; i < TT_ARC_TELEMETRY_COUNT; i++)
			entries[i] = (i << 16) | BlackholeTelemetryTag(i);
		for (unsigned i = TT_ARC_TELEMETRY_COUNT; i < 128; i++)
			entries[i] = (i << 16) | (100 + i);
	}
	uint32_t TlbRead(unsigned i) { assert(i < 3); return tlb[i]; }
	void TlbWrite(unsigned i, uint32_t v) { assert(i < 3); if (++writes != droppedWrite) tlb[i] = v; }
	uint32_t CsmRead(uint32_t address)
	{
		assert(BlackholeTelemetryRange(address, 4));
		for (unsigned i = 0; i < 3; i++) assert(tlb[i] == BlackholeTelemetryTlbWord(i));
		reads++;
		if (address == 0x10001000) return changing && ++headerReads > 1 ? version ^ 1 : version;
		if (address == 0x10001004) return count;
		if (address >= 0x10001008 && address < 0x10001008 + 128 * 4)
			return entries[(address - 0x10001008) / 4];
		assert(address >= 0x10002000 && address < 0x10002000 + 128 * 4);
		dataReads++;
		return (address - 0x10002000) / 4 + 0x1234;
	}
};

int main()
{
	assert(BlackholeTelemetryRange(0x10000000, 4));
	assert(BlackholeTelemetryRange(0x1001fffc, 4));
	assert(!BlackholeTelemetryRange(0x1001fffc, 8));
	assert(!BlackholeTelemetryRange(0x10020000, 4));
	assert(!BlackholeTelemetryRange(0x0ffffffc, 4));
	assert(!BlackholeTelemetryRange(0xfffffffc, 4));
	assert(!BlackholeTelemetryRange(0x10000002, 4));
	assert(!BlackholeTelemetryRange(0x10000000, 0));
	assert(!BlackholeTelemetryRange(0x10000000, 0x20004));
	assert((uint64_t)BlackholeTelemetryTlbWord(0) << 21 == 0x10000000);
	MockTelemetry io;
	tt_arc_telemetry_result r;
	tt_arc_scratch_result arc = GoodArc();
	assert(BlackholeArcTelemetryProbe(io, arc, r));
	assert(r.state == TT_ARC_TELEMETRY_OK && r.tlb_restored && r.present_mask == 0xfff);
	assert(io.reads == 28 && io.dataReads == 12 && io.writes == 6);
	assert(memcmp(r.tlb_before, r.tlb_after, sizeof(r.tlb_before)) == 0);
	io = {}; io.count = 128;
	assert(BlackholeArcTelemetryProbe(io, arc, r) && r.tlb_restored && io.reads == 144);
	for (unsigned drop = 1; drop <= 6; drop++) {
		io = {}; io.droppedWrite = drop;
		assert(!BlackholeArcTelemetryProbe(io, arc, r));
		if (drop <= 3) assert(io.reads == 0 && r.tlb_restored);
		else assert(r.state == TT_ARC_TELEMETRY_RESTORE_FAILED);
	}
	io = {}; arc.telemetry_tags = 0x1001fffc;
	assert(!BlackholeArcTelemetryProbe(io, arc, r) && io.writes == 0 && io.reads == 0);
	arc = GoodArc(); io = {}; io.count = 129;
	assert(!BlackholeArcTelemetryProbe(io, arc, r) && r.tlb_restored && io.dataReads == 0);
	arc = GoodArc(); arc.telemetry_data = arc.telemetry_tags + 4; io = {};
	assert(!BlackholeArcTelemetryProbe(io, arc, r) && r.tlb_restored && io.dataReads == 0);
	arc = GoodArc();
	io = {}; io.count = 0;
	assert(!BlackholeArcTelemetryProbe(io, arc, r) && r.tlb_restored && io.dataReads == 0);
	io = {}; io.version = 0x20000;
	assert(!BlackholeArcTelemetryProbe(io, arc, r) && r.tlb_restored && io.dataReads == 0);
	io = {}; io.entries[1] = io.entries[0];
	assert(!BlackholeArcTelemetryProbe(io, arc, r) && r.tlb_restored && io.dataReads == 0 && r.present_mask == 0);
	io = {}; io.entries[1] = (0xffffU << 16) | BlackholeTelemetryTag(1);
	assert(!BlackholeArcTelemetryProbe(io, arc, r) && r.tlb_restored && io.dataReads == 0);
	io = {}; io.changing = true;
	assert(!BlackholeArcTelemetryProbe(io, arc, r) && r.state == TT_ARC_TELEMETRY_CHANGED && r.tlb_restored && r.present_mask == 0);
	io = {}; io.entries[0] = 0xffff;
	assert(!BlackholeArcTelemetryProbe(io, arc, r) && r.state == TT_ARC_TELEMETRY_INCOMPLETE && r.tlb_restored);
	io = {};
	for (unsigned i = 0; i < TT_ARC_TELEMETRY_COUNT; i++) io.entries[i] = (i << 16) | BlackholeTelemetryTag(i,true);
	assert(BlackholeArcTelemetryProbe(io,arc,r,true) && r.version == 2 && r.present_mask == 0xfff && r.tlb_restored);
	assert(io.reads == 28 && io.dataReads == 12);
	io.entries[0] = 0xffff;
	assert(!BlackholeArcTelemetryProbe(io,arc,r,true) && r.state == TT_ARC_TELEMETRY_INCOMPLETE && r.tlb_restored);
	assert(BlackholeTelemetryTag(12,true) == 0);
}
