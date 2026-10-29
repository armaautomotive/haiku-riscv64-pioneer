/* Distributed under the MIT License. */
#include <cassert>
#include <cstring>
#include "../../../src/add-ons/kernel/drivers/misc/BlackholePioneerMmioProbe.h"

static tt_pioneer_profile GoodProfile()
{
	tt_pioneer_profile p = {};
	p.version = 1; p.stage = 6; p.ready = 1; p.card_bus = 3; p.root_bus = 2;
	p.root_buses = 0x003f0100; p.root_memory = 0x0000fff0; p.root_prefetch = 0x0001fff1;
	p.controller = 0x7062000000; p.config = 0x4800000000; p.config_length = 0x1000;
	p.ranges[0] = {0x4810800000, 0x800000, 0x400000, 2, 0};
	p.ranges[1] = {0x4890000000, 0x90000000, 0x30000000, 1, 8};
	p.ranges[2] = {0x48c0000000, 0xc0000000, 0x10000000, 1, 0};
	p.ranges[3] = {0x4900000000, 0x4900000000, 0x200000000, 1, 12};
	p.ranges[4] = {0x4b00000000, 0x4b00000000, 0x100000000, 1, 4};
	p.bar0_host_start = 0x4900000000; p.bar0_host_end = 0x491fffffff;
	p.bar2_host_start = 0x4920000000; p.bar2_host_end = 0x49200fffff;
	return p;
}

struct MockIO {
	tt_pioneer_profile profile = GoodProfile();
	uint32_t cardID = 0xb1401e52, rootID = 0x20421e30, subsystem = 0x00431e52;
	uint32_t bars[6] = {12, 0, 12, 0, 12, 0};
	uint32_t bridge[3] = {0x0001fff1, 0, 0};
	uint32_t command[2] = {};
	uint32_t rom = 0, msiControl = 0, msixControl = 0, pmControl = 0, capPointer = 0x40;
	bool capCycle = false;
	bool hookFails = false;
	uint64_t sizes[3] = {0x20000000, 0x100000, UINT64_C(0x800000000)};
	uint32_t samples[3] = {2, 2, 2};
	unsigned writes = 0, droppedWrite = 0, mmioReads = 0, mapCalls = 0;
	bool quiet = true, mapFails = false, unmapFails = false, mapped = false;
	bool partialMapFails = false;
	bool disableCardFails = false, disableRootFails = false;
	const tt_pioneer_profile& Profile() { return profile; }
	bool Quiet() { return quiet; }
	bool AfterNodeId(uint32_t) { return !hookFails; }
	uint64_t Size(unsigned bar) { return sizes[bar / 2]; }
	uint32_t CardRead(unsigned offset, unsigned width)
	{
		if (offset == 0) { assert(width == 4); return cardID; }
		if (offset == 0x2c) { assert(width == 4); return subsystem; }
		if (offset == 4) { assert(width == 2 || width == 4); return command[0] | (width == 4 ? 0x00100000 : 0); }
		if (offset == 0x30) { assert(width == 4); return rom; }
		if (offset == 0x34) { assert(width == 1); return capPointer; }
		if (offset == 0x40) { assert(width == 2); return 0x5005; }
		if (offset == 0x42) { assert(width == 2); return msiControl; }
		if (offset == 0x50) { assert(width == 2); return capCycle ? 0x4011 : 0x6011; }
		if (offset == 0x52) { assert(width == 2); return msixControl; }
		if (offset == 0x60) { assert(width == 2); return 1; }
		if (offset == 0x64) { assert(width == 2); return pmControl; }
		assert(width == 4 && offset >= 0x10 && offset <= 0x24 && offset % 4 == 0);
		return bars[(offset - 0x10) / 4];
	}
	uint32_t RootRead(unsigned offset, unsigned width)
	{
		if (offset == 0) { assert(width == 4); return rootID; }
		if (offset == 8) { assert(width == 4); return 0x06040000; }
		if (offset == 4) { assert(width == 2); return command[1]; }
		if (offset == 0x18) { assert(width == 4); return profile.root_buses; }
		if (offset == 0x20) { assert(width == 4); return profile.root_memory; }
		assert(width == 4 && offset >= 0x24 && offset <= 0x2c && offset % 4 == 0);
		return bridge[(offset - 0x24) / 4];
	}
	void CardWrite(unsigned offset, unsigned width, uint32_t value)
	{
		if (++writes == droppedWrite) return;
		if (offset == 4) {
			assert(width == 2 && (value == 0 || value == 0x402) && (value & 4) == 0);
			if (value == 0x402) assert(mapped && command[1] == 0x402);
			if (!(disableCardFails && value == 0 && command[0] != 0)) command[0] = value;
			return;
		}
		assert(command[0] == 0); // Never rewrite a live BAR.
		assert(width == 4 && offset >= 0x10 && offset <= 0x24 && offset % 4 == 0);
		bars[(offset - 0x10) / 4] = value;
	}
	void RootWrite(unsigned offset, unsigned width, uint32_t value)
	{
		if (++writes == droppedWrite) return;
		if (offset == 4) {
			assert(width == 2 && (value == 0 || value == 0x402) && (value & 4) == 0);
			if (value == 0x402) assert(mapped);
			if (!(disableRootFails && value == 0 && command[1] != 0)) command[1] = value;
			return;
		}
		assert(command[1] == 0); // Never rewrite a live bridge window.
		assert(width == 4 && offset >= 0x24 && offset <= 0x2c && offset % 4 == 0);
		bridge[(offset - 0x24) / 4] = value;
	}
	bool Map()
	{
		mapCalls++;
		const uint32_t trial[] = {12, 0x49, 0x2000000c, 0x49, 12, 0xfffffff8};
		assert(memcmp(bars, trial, sizeof(bars)) == 0);
		assert(bridge[0] == 0x20010001 && bridge[1] == 0x49 && bridge[2] == 0x49);
		assert(command[0] == 0 && command[1] == 0);
		mapped = !mapFails;
		if (partialMapFails) { mapped = true; return false; }
		return mapped;
	}
	bool Unmap() { if (unmapFails) return false; mapped = false; return true; }
	uint32_t ReadNodeId()
	{
		assert(mapped && command[0] == 0x402 && command[1] == 0x402 && mmioReads < 3);
		return samples[mmioReads++];
	}
};

#ifndef PIONEER_MMIO_PROBE_MODEL_ONLY
static void Refused(MockIO io)
{
	tt_pioneer_mmio_result r;
	BlackholePioneerMmioProbe(io, r);
	assert(r.state == TT_PIONEER_MMIO_REFUSED && io.writes == 0 && io.mmioReads == 0);
}

int main()
{
	MockIO io;
	tt_pioneer_mmio_result r;
	tt_pioneer_mmio_transaction transaction={};
	assert(BlackholePioneerMmioBegin(io,r,transaction));
	assert(io.mapped && io.command[0]==0x402 && io.command[1]==0x402 && !r.restored && io.writes==11);
	assert(transaction.config_attempted && transaction.map_attempted && io.mmioReads==3);
	BlackholePioneerMmioEnd(io,r,transaction);
	assert(r.restored && !io.mapped && io.writes==22);
	BlackholePioneerMmioEnd(io,r,transaction);
	assert(io.writes==22);
	io={};
	BlackholePioneerMmioProbe(io, r);
	assert(r.state == TT_PIONEER_MMIO_READ_OK && r.restored && r.stage == 5);
	assert(io.mmioReads == 3 && !io.mapped && io.writes == 22);
	assert(memcmp(r.bars_before, r.bars_after, sizeof(r.bars_before)) == 0);
	assert(memcmp(r.bridge_before, r.bridge_after, sizeof(r.bridge_before)) == 0);
	assert(io.command[0] == 0 && io.command[1] == 0);
	for (unsigned drop = 1; drop <= 22; drop++) {
		io = {}; io.droppedWrite = drop;
		BlackholePioneerMmioProbe(io, r);
		// A dropped idempotent write may be harmless; every non-restored
		// transaction must explicitly report a restoration failure.
		if (!r.restored) assert(r.state == TT_PIONEER_MMIO_RESTORE_FAILED);
		assert((io.command[0] & 4) == 0 && (io.command[1] & 4) == 0);
	}
	io = {}; io.mapFails = true; BlackholePioneerMmioProbe(io, r);
	assert(r.state == TT_PIONEER_MMIO_MAP_FAILED && r.restored && io.mmioReads == 0);
	io = {}; io.partialMapFails = true; BlackholePioneerMmioProbe(io, r);
	assert(r.state == TT_PIONEER_MMIO_MAP_FAILED && r.restored && !io.mapped && io.mmioReads == 0);
	io = {}; io.hookFails = true; BlackholePioneerMmioProbe(io, r);
	assert(r.state == TT_PIONEER_MMIO_READ_BAD && r.restored);
	io = {}; io.unmapFails = true; BlackholePioneerMmioProbe(io, r);
	assert(r.state == TT_PIONEER_MMIO_RESTORE_FAILED && !r.restored);
	io = {}; io.disableCardFails = true; BlackholePioneerMmioProbe(io, r);
	assert(r.state == TT_PIONEER_MMIO_RESTORE_FAILED && io.command[1] == 0);
	io = {}; io.disableRootFails = true; BlackholePioneerMmioProbe(io, r);
	assert(r.state == TT_PIONEER_MMIO_RESTORE_FAILED && io.command[0] == 0);
	io = {}; io.samples[1] = 11; BlackholePioneerMmioProbe(io, r);
	assert(r.state == TT_PIONEER_MMIO_READ_BAD && r.restored);
	io = {}; io.samples[0] = io.samples[1] = io.samples[2] = 0xffffffff; BlackholePioneerMmioProbe(io, r);
	assert(r.state == TT_PIONEER_MMIO_READ_BAD && r.restored);
	io = {}; io.samples[0] = io.samples[1] = io.samples[2] = 11; BlackholePioneerMmioProbe(io, r);
	assert(r.state == TT_PIONEER_MMIO_READ_OK && r.restored);
	io = {}; io.samples[0] = io.samples[1] = io.samples[2] = 0x18611002; BlackholePioneerMmioProbe(io, r);
	assert(r.state == TT_PIONEER_MMIO_READ_OK && r.restored);
	io = {}; io.samples[0] = io.samples[1] = io.samples[2] = 0x18611042; BlackholePioneerMmioProbe(io, r);
	assert(r.state == TT_PIONEER_MMIO_READ_BAD && r.restored);
	io = {}; io.rom = 1; Refused(io);
	io = {}; io.msiControl = 1; Refused(io);
	io = {}; io.msixControl = 0x8000; Refused(io);
	io = {}; io.pmControl = 3; Refused(io);
	io = {}; io.capPointer = 0x41; Refused(io);
	io = {}; io.capPointer = 0; Refused(io);
	io = {}; io.capCycle = true; Refused(io);
	io = {}; io.quiet = false; Refused(io);
	io = {}; io.cardID ^= 1; Refused(io);
	io = {}; io.rootID ^= 1; Refused(io);
	io = {}; io.subsystem ^= 1; Refused(io);
	io = {}; io.command[0] = 2; Refused(io);
	io = {}; io.command[1] = 4; Refused(io);
	for (unsigned i = 0; i < 6; i++) { io = {}; io.bars[i] ^= 0x10; Refused(io); }
	for (unsigned i = 0; i < 3; i++) { io = {}; io.sizes[i] = 0; Refused(io); }
	for (unsigned i = 0; i < TT_PIONEER_RANGE_COUNT; i++) {
		io = {}; io.profile.ranges[i].host ^= 0x1000; Refused(io);
		io = {}; io.profile.ranges[i].pci ^= 0x1000; Refused(io);
		io = {}; io.profile.ranges[i].size ^= 0x1000; Refused(io);
		io = {}; io.profile.ranges[i].type ^= 1; Refused(io);
		io = {}; io.profile.ranges[i].flags ^= 1; Refused(io);
	}
	io = {}; io.profile.bar0_host_end--; Refused(io);
	io = {}; io.profile.bar2_host_start++; Refused(io);
	io = {}; io.profile.controller++; Refused(io);
	io = {}; io.profile.root_bus++; Refused(io);
	io = {}; io.profile.card_bus++; Refused(io);
	io = {}; io.profile.root_command = 2; Refused(io);
	io = {}; io.profile.status = -1; Refused(io);
	io = {}; io.profile.ready = 0; Refused(io);
}
#endif
