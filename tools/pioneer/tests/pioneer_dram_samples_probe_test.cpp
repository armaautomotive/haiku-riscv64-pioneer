/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstring>
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeDramSamplesProbe.h"

struct MockDramSamples {
	uint32_t source[TT_DRAM_SOURCE_WORDS] = {
		0x6005,0x0a418820,0x1ac4a166,0x127541ee,0x1e280440,0x1662b5c3,0x147,
		0x0a418820,0x16a4a0e6,0x06a12c20,0x0c72a089,0x0e548c20,2,0,3,0
	};
	uint32_t tlb[2][3] = {{1,2,3},{4,5,6}};
	uint32_t original[TT_DRAM_BANKS][TT_DRAM_REGIONS][TT_DRAM_WORDS];
	uint32_t memory[TT_DRAM_BANKS][TT_DRAM_REGIONS][TT_DRAM_WORDS];
	unsigned writes = 0, dataWrites = 0, dataReads = 0, resetReads = 0, sourceReads = 0, routeMask = 0;
	unsigned dropConfig = 0, dropData = 0, badIdBank = 99, changeCoreRead = 0;
	bool aliasBanks = false, aliasRegions = false, failDataRestore = false, failTlbRestore = false;
	bool sourceChanges = false, coreChangesAfterData = false;
	MockDramSamples() {
		for (unsigned b = 0; b < TT_DRAM_BANKS; b++) for (unsigned r = 0; r < TT_DRAM_REGIONS; r++)
			for (unsigned w = 0; w < TT_DRAM_WORDS; w++) original[b][r][w] = memory[b][r][w] = 0x98760000 ^ (b << 12) ^ (r << 6) ^ w;
	}
	uint32_t SourceRead(unsigned word) {
		assert(word < TT_DRAM_SOURCE_WORDS); sourceReads++;
		return source[word] ^ ((sourceChanges && sourceReads > 32) ? 1U : 0U);
	}
	uint32_t DramTlbRead(unsigned slot, unsigned word) { assert(slot < 2 && word < 3); return tlb[slot][word]; }
	void DramTlbWrite(unsigned slot, unsigned word, uint32_t value) {
		assert(slot < 2 && word < 3);
		if (++writes != dropConfig && !(failTlbRestore && value == slot * 3 + word + 1)) tlb[slot][word] = value;
	}
	bool Matches(unsigned slot, uint32_t base, unsigned x, unsigned y, bool ro) {
		for (unsigned w = 0; w < 3; w++) if (tlb[slot][w] != BlackholeDramFixedTlbWord(base,x,y,ro,w)) return false;
		return true;
	}
	unsigned RouteBank() {
		for (unsigned b = 0; b < TT_DRAM_BANKS; b++) {
			unsigned x = b < 3 ? 18 : 17;
			const unsigned ys[] = {12,15,18,12,15,18,21};
			if (Matches(1,0xffa00000,x,ys[b],true)) return b;
		}
		assert(false); return 0;
	}
	uint32_t DramIdRead(unsigned word) {
		assert(word < TT_DRAM_ID_WORDS);
		unsigned b = RouteBank(); routeMask |= 1U << b;
		if (b == badIdBank) return 0;
		if (word == 0) return 0x1c611000 | BlackholeDramPhysicalX(b) | (BlackholeDramPhysicalY(b) << 6);
		if (word == 1) return 0x80000 | BlackholeDramBank(b);
		return word == 2 ? 0xe005 : BlackholeDramPhysicalX(b) | (BlackholeDramPhysicalY(b) << 6);
	}
	uint32_t ResetRead() {
		const unsigned xs[] = {1,2,3,4,5,6,7,10,11,12,13,14};
		bool valid = false;
		for (unsigned x : xs) for (unsigned y = 2; y <= 11; y++) valid |= Matches(1,0xffa00000,x,y,true);
		assert(valid); resetReads++;
		return (resetReads == changeCoreRead || (coreChangesAfterData && dataWrites != 0)) ? 0 : 0x47800;
	}
	void DataSelection(unsigned& bank, unsigned& region) {
		assert(routeMask == 127 && resetReads >= 240);
		const unsigned ys[] = {12,15,18,12,15,18,21};
		for (unsigned b = 0; b < TT_DRAM_BANKS; b++) for (unsigned r = 0; r < TT_DRAM_REGIONS; r++)
			if (Matches(0,BlackholeDramSampleBase(r),b < 3 ? 18 : 17,ys[b],false)) {
				bank = aliasBanks && b == 6 ? 5 : b;
				region = aliasRegions && r == 4 ? 3 : r;
				return;
			}
		assert(false);
	}
	uint32_t DramRead(unsigned word) {
		assert(word < TT_DRAM_WORDS); unsigned b = 0,r = 0; DataSelection(b,r); dataReads++;
		return memory[b][r][word];
	}
	void DramWrite(unsigned word, uint32_t value) {
		assert(word < TT_DRAM_WORDS); unsigned b = 0,r = 0; DataSelection(b,r);
		assert(!coreChangesAfterData || dataWrites < 16); // no restore after loss of ownership
		if (++dataWrites != dropData && !(failDataRestore && value == original[b][r][word])) memory[b][r][word] = value;
	}
	bool Restored() { return memcmp(original,memory,sizeof(memory)) == 0; }
};
static tt_arc_telemetry_result telemetry()
{
	tt_arc_telemetry_result t = {}; t.version = 1; t.state = TT_ARC_TELEMETRY_OK;
	t.tlb_restored = 1; t.present_mask = 0xfff;
	t.values[0] = 0x12050001; t.values[6] = 0xf7; t.values[7] = 0x5515; t.values[9] = 0xfff;
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
	auto t = telemetry(); auto c = core(); tt_dram_samples_result r;
	MockDramSamples io;
	assert(BlackholeDramSamplesProbe(io,t,c,false,r));
	assert(r.state == TT_DRAM_SAMPLE_ROUTES_OK && r.routes_verified == 7 && r.cores_checked == 0);
	assert(r.data_restored && r.tlb_restored && !r.recovery_required && io.dataReads == 0 && io.dataWrites == 0);
	io = {};
	assert(BlackholeDramSamplesProbe(io,t,c,true,r));
	assert(r.state == TT_DRAM_SAMPLE_PASSED && r.routes_verified == 7 && r.cores_checked == 240);
	assert(r.saved_regions == 35 && r.modified_regions == 35 && r.passes == 2 && r.mismatches == 0);
	assert(r.data_restored && r.tlb_restored && !r.recovery_required && io.Restored());
	assert(io.dataWrites == 1680 && io.dataReads == 2240);
	unsigned configurationWrites = io.writes;
	assert(BlackholeDramFixedTlbWord(0xffa00000,18,12,true,1) == 0x89189000);
	assert(BlackholeDramFixedTlbWord(0xffa00000,18,12,true,2) == 0xa41);
	for (unsigned b = 0; b < TT_DRAM_BANKS; b++) {
		io = {}; io.badIdBank = b;
		assert(!BlackholeDramSamplesProbe(io,t,c,true,r) && io.dataWrites == 0 && io.dataReads == 0);
		assert(r.state == TT_DRAM_SAMPLE_BAD_ROUTE && r.tlb_restored && !r.recovery_required);
	}
	for (unsigned drop = 1; drop <= configurationWrites; drop++) {
		io = {}; io.dropConfig = drop;
		bool okay = BlackholeDramSamplesProbe(io,t,c,true,r);
		assert(r.tlb_restored);
		if (r.data_restored) assert(!r.recovery_required && io.Restored());
		else assert(r.recovery_required && !okay); // failed ownership re-proof: never restore blindly
		if (okay) assert(r.passes == 2);
	}
	for (unsigned drop = 1; drop <= 1120; drop += 13) {
		io = {}; io.dropData = drop;
		assert(!BlackholeDramSamplesProbe(io,t,c,true,r));
		assert(r.state == TT_DRAM_SAMPLE_DATA_MISMATCH && r.data_restored && r.tlb_restored && io.Restored());
	}
	io = {}; io.aliasBanks = true;
	assert(!BlackholeDramSamplesProbe(io,t,c,true,r) && r.mismatches != 0 && r.data_restored && io.Restored());
	io = {}; io.aliasRegions = true;
	assert(!BlackholeDramSamplesProbe(io,t,c,true,r) && r.mismatches != 0 && r.data_restored && io.Restored());
	io = {}; io.failDataRestore = true;
	assert(!BlackholeDramSamplesProbe(io,t,c,true,r) && !r.data_restored && r.recovery_required);
	io = {}; io.failTlbRestore = true;
	assert(!BlackholeDramSamplesProbe(io,t,c,true,r) && r.data_restored && !r.tlb_restored && r.recovery_required);
	io = {}; io.sourceChanges = true;
	assert(!BlackholeDramSamplesProbe(io,t,c,true,r) && !r.data_restored && r.recovery_required && io.dataWrites == 1120);
	io = {}; io.coreChangesAfterData = true;
	assert(!BlackholeDramSamplesProbe(io,t,c,true,r) && !r.data_restored && r.recovery_required && io.dataWrites == 16);
	io = {}; io.changeCoreRead = 101;
	assert(!BlackholeDramSamplesProbe(io,t,c,true,r) && io.dataReads == 0 && io.dataWrites == 0 && r.data_restored);
	io = {}; io.source[3] ^= 1U << 10; // corrupt enabled translated x=14
	assert(!BlackholeDramSamplesProbe(io,t,c,true,r) && io.writes == 0);
	io = {}; t.values[9] = 0x3fff;
	assert(!BlackholeDramSamplesProbe(io,t,c,true,r) && io.writes == 0);
}
