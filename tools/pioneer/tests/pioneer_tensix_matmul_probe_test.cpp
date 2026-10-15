/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstring>
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeTensixMatmulProbe.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeClockedMatmulProbe.h"

struct MockMatmul {
	uint32_t source[TT_DRAM_SOURCE_WORDS] = {
		0x6005,0x0a418820,0x1ac4a166,0x127541ee,0x1e280440,0x1662b5c3,0x147,
		0x0a418820,0x16a4a0e6,0x06a12c20,0x0c72a089,0x0e548c20,2,0,3,0
	};
	uint32_t tlb[2][3] = {{1,2,3},{4,5,6}}, controls[3] = {};
	uint32_t memory[TT_UNPACK_IMAGE_WORDS], original[TT_UNPACK_IMAGE_WORDS], saved[TT_UNPACK_IMAGE_WORDS], reset = 0x47800;
	unsigned configWrites = 0, imageWrites = 0, resetWrites = 0, resetReads = 0, sourceReads = 0, polls = 0, pauses = 0;
	unsigned dropConfig = 0, dropImage = 0, dropReset = 0, changeResetAt = 0;
	bool timeout = false, badPC = false, badMath = false, failRestore = false, sourceChanges = false, failTlbRestore = false;
	bool everStarted = false, executed = false;
	bool configOnly = false;
	unsigned sfpuOnly = 0;
	const tt_tensix_tile_data* tileInput = nullptr;
	bool batching = false;
	unsigned jobsStarted = 0;
	unsigned timeoutJob = 0;
	bool allocationFails = false;
	uint32_t* MathSavedWords() { return allocationFails ? nullptr : saved; }
	unsigned clockEnter = 0, clockLeave = 0;
	bool enterFails = false, enterUncertain = false, leaveFails = false;
	bool EnterNormalClock(tt_arc_clock_guard_result& r) {
		clockEnter++; r = {}; r.version = 1; r.busy_requested = 1;
		r.state = TT_ARC_CLOCK_BUSY_VERIFIED; r.recovery_required = enterUncertain;
		return !enterFails && !enterUncertain;
	}
	bool LeaveNormalClock(tt_arc_clock_guard_result& r) {
		assert(!r.recovery_required); clockLeave++;
		r.restored = !leaveFails; r.recovery_required = leaveFails;
		r.state = leaveFails ? TT_ARC_CLOCK_RESTORE_FAILED : TT_ARC_CLOCK_RESTORED;
		return !leaveFails;
	}
	MockMatmul() { for (unsigned i = 0; i < TT_UNPACK_IMAGE_WORDS; i++) memory[i] = original[i] = 0xabcd0000 ^ i; }
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
		if (reset == 0x46800 && ++polls == 2 && !timeout && jobsStarted != timeoutJob) {
			memory[BlackholeTensixStatusIndex(sfpuOnly)] = BlackholeTensixFinalPhase(sfpuOnly);
			memory[BlackholeTensixStatusIndex(sfpuOnly)+1] = 2; memory[BlackholeTensixStatusIndex(sfpuOnly)+2] = 0x4d415431;
			for (unsigned i = 0; i < TT_TENSIX_OUTPUT_WORDS; i++) memory[1+BlackholeTensixCodeWords(sfpuOnly)+i] = (configOnly || sfpuOnly == 19 || (sfpuOnly != 0 && sfpuOnly != 7 && sfpuOnly < 10)) ? 0U : BlackholeMatmulStageExpected(sfpuOnly,i);
			if (batching) for (unsigned i = 0; i < TT_TENSIX_OUTPUT_WORDS; i++) memory[1+BlackholeTensixCodeWords(sfpuOnly)+i] = (jobsStarted<<16)|i;
			if (badMath) memory[1+BlackholeTensixCodeWords(sfpuOnly)] ^= 1;
			executed = true;
		}
		return reset;
	}
	uint32_t MathControlRead(unsigned word) { Windows(); assert(word < 3); return controls[word]; }
	uint32_t MathImageRead(unsigned word) { Windows(); assert(word < BlackholeTensixImageWords(sfpuOnly)); return memory[word]; }
	void MathImageWrite(unsigned word, uint32_t value) {
		Windows(); assert(word < BlackholeTensixImageWords(sfpuOnly) && reset == 0x47800);
		if (everStarted) assert(executed && !timeout && !badPC && !sourceChanges);
		if (batching && everStarted && value != original[word]) assert(BlackholeTileDataMutableIndex(word));
		if (++imageWrites != dropImage && !(failRestore && value == original[word])) memory[word] = value;
	}
	void MathResetWrite(uint32_t value) {
		Windows();
		if (value == 0x87ffff) {
			if (batching && reset == 0x47800) assert(!everStarted || executed);
			else assert(reset == (everStarted ? 0x46800U : 0x47800U));
		}
		else if (value == 0x46800) {
			assert(reset == 0x87ffff && (!everStarted || batching));
			for (unsigned i = 0; i < BlackholeTensixImageWords(sfpuOnly); i++) {
				if (batching && jobsStarted != 0 && !BlackholeTileDataMutableIndex(i)) continue;
				assert(memory[i] == (tileInput != nullptr ? BlackholeTileDataImageWord(i,tileInput[batching ? jobsStarted : 0]) : BlackholeTensixImageWord(i,configOnly,sfpuOnly)));
			}
			jobsStarted++; polls = 0; executed = false;
			everStarted = true;
		} else { assert(value == 0x47800 && reset == 0x87ffff && everStarted); }
		if (++resetWrites != dropReset) reset = value;
	}
	uint32_t MathPcRead() { Windows(); return badPC ? 0x70000 : BlackholeTensixParkPc(sfpuOnly); }
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
	assert(baseline.cores_checked == 120 && io.imageWrites == 2*TT_TENSIX_IMAGE_WORDS && io.resetWrites == 4);
	unsigned configWrites = io.configWrites;
	for (unsigned drop = 1; drop <= configWrites; drop++) {
		io = {}; baseline = io.Baseline(); io.dropConfig = drop;
		bool okay = BlackholeTensixMatmulProbe(io,baseline,r);
		assert(r.l1_restored && r.tlb_restored && !r.recovery_required && io.Restored());
		if (okay) assert(r.completed && r.stopped);
	}
	for (unsigned drop = 1; drop <= 2*TT_TENSIX_IMAGE_WORDS; drop++) {
		io = {}; baseline = io.Baseline(); io.dropImage = drop;
		bool okay = BlackholeTensixMatmulProbe(io,baseline,r);
		assert(r.l1_restored && r.tlb_restored && !r.recovery_required && io.Restored());
		if (drop <= TT_TENSIX_IMAGE_WORDS) assert(!okay && io.resetWrites == 0);
		else assert(okay && io.imageWrites == 3*TT_TENSIX_IMAGE_WORDS);
	}
	for (unsigned drop = 1; drop <= 4; drop++) {
		io = {}; baseline = io.Baseline(); io.dropReset = drop;
		assert(!BlackholeTensixMatmulProbe(io,baseline,r) && r.recovery_required && !r.l1_restored && io.imageWrites == TT_TENSIX_IMAGE_WORDS);
	}
	io = {}; baseline = io.Baseline(); io.timeout = true;
	assert(!BlackholeTensixMatmulProbe(io,baseline,r) && r.polls == 2000 && r.stopped && !r.completed && r.recovery_required);
	assert(io.imageWrites == TT_TENSIX_IMAGE_WORDS);
	io = {}; baseline = io.Baseline(); io.badPC = true;
	assert(!BlackholeTensixMatmulProbe(io,baseline,r) && r.stopped && !r.completed && r.recovery_required);
	io = {}; baseline = io.Baseline(); io.badMath = true;
	assert(!BlackholeTensixMatmulProbe(io,baseline,r) && r.completed && r.stopped && r.mismatches == 1 && io.Restored() && !r.recovery_required);
	io = {}; baseline = io.Baseline(); io.sourceChanges = true;
	assert(!BlackholeTensixMatmulProbe(io,baseline,r) && r.recovery_required && !r.stopped && io.resetWrites == 2 && io.imageWrites == TT_TENSIX_IMAGE_WORDS);
	io = {}; baseline = io.Baseline(); io.failRestore = true;
	assert(!BlackholeTensixMatmulProbe(io,baseline,r) && r.recovery_required && !r.l1_restored && io.imageWrites == 3*TT_TENSIX_IMAGE_WORDS);
	io = {}; baseline = io.Baseline(); io.failTlbRestore = true;
	assert(!BlackholeTensixMatmulProbe(io,baseline,r) && r.recovery_required && r.l1_restored && !r.tlb_restored);
	for (unsigned control = 0; control < 3; control++) {
		io = {}; baseline = io.Baseline(); io.controls[control] = 1;
		assert(!BlackholeTensixMatmulProbe(io,baseline,r) && io.imageWrites == 0 && io.resetWrites == 0 && r.tlb_restored);
	}
	io = {}; baseline = io.Baseline(); baseline.routes_verified = 6;
	assert(!BlackholeTensixMatmulProbe(io,baseline,r) && io.configWrites == 0);
	io = {}; baseline = io.Baseline(); io.configOnly = true;
	assert(BlackholeTensixMatmulProbe(io,baseline,r,true) && r.version == 2 && r.representative[0] == 0 && r.representative[1] == 0 && io.Restored());
	io = {}; baseline = io.Baseline(); io.configOnly = io.timeout = true;
	assert(!BlackholeTensixMatmulProbe(io,baseline,r,true) && r.recovery_required && !r.completed && r.stopped);
	io = {}; baseline = io.Baseline(); io.configOnly = io.badMath = true;
	assert(!BlackholeTensixMatmulProbe(io,baseline,r,true) && r.completed && r.stopped && r.mismatches == 1 && io.Restored());
	tt_arc_clock_guard_result clock = {};
	io = {}; baseline = io.Baseline(); io.sfpuOnly = 1;
	assert(BlackholeTensixMatmulProbe(io,baseline,r,false,true) && r.version == 3 && r.completed && io.Restored());
	io = {}; baseline = io.Baseline(); io.sfpuOnly = 1; io.timeout = true;
	assert(!BlackholeTensixMatmulProbe(io,baseline,r,false,true) && r.recovery_required && !r.completed);
	io = {}; baseline = io.Baseline(); io.sfpuOnly = 2;
	assert(BlackholeTensixMatmulProbe(io,baseline,r,false,2) && r.version == 4 && r.completed && io.Restored());
	io = {}; baseline = io.Baseline();
	assert(!BlackholeTensixMatmulProbe(io,baseline,r,false,25) && io.configWrites == 0);
	tt_tensix_tile_data input = {}; input.version = 1;
	uint32_t output[256] = {};
	io = {}; baseline = io.Baseline(); io.sfpuOnly = 24; io.tileInput = &input;
	assert(BlackholeTensixMatmulProbe(io,baseline,r,false,24,&input,output));
	assert(r.state == TT_MATMUL_DATA_EXECUTED && r.version == 26 && io.Restored() && !r.recovery_required);
	for (unsigned i = 0; i < 256; i++) assert(output[i] == BlackholeMatmulStageExpected(24,i));
	input.weights[0] = 0x3f83; input.activations[0] = 0xbf91;
	const unsigned inputWords[] = {1U+(0x70500-0x70000)/4,1U+(0x70710-0x70000)/4};
	for (unsigned word : inputWords) {
		io = {}; baseline = io.Baseline(); io.sfpuOnly = 24; io.tileInput = &input;
		io.dropImage = word; // image loop begins at index 1
		assert(!BlackholeTensixMatmulProbe(io,baseline,r,false,24,&input,output));
		assert(!r.started && r.l1_restored && r.tlb_restored && !r.recovery_required && io.Restored());
	}
	io = {}; baseline = io.Baseline(); io.sfpuOnly = 24; io.tileInput = &input; io.timeout = true;
	assert(!BlackholeTensixMatmulProbe(io,baseline,r,false,24,&input,output));
	assert(r.recovery_required && !r.completed && !r.l1_restored && io.imageWrites == TT_UNPACK_IMAGE_WORDS);
	io = {}; baseline = io.Baseline();
	assert(!BlackholeTensixMatmulProbe(io,baseline,r,false,24) && io.configWrites == 0);
	assert(!BlackholeTensixMatmulProbe(io,baseline,r,false,23,&input,output) && io.configWrites == 0);
	input.flags = 1;
	assert(!BlackholeTensixMatmulProbe(io,baseline,r,false,24,&input,output) && io.configWrites == 0);
	tt_tensix_tile_data batch[TT_DATA_TILE_BATCH_MAX] = {};
	uint32_t batchOutput[TT_DATA_TILE_BATCH_MAX*256] = {}, completedJobs = 99;
	for (unsigned j = 0; j < TT_DATA_TILE_BATCH_MAX; j++) {
		batch[j].version = 1; batch[j].weights[j] = 0x3f80+j; batch[j].activations[j+1] = 0xbf80+j;
	}
	io = {}; baseline = io.Baseline(); io.sfpuOnly = 24; io.tileInput = batch; io.batching = true;
	assert(BlackholeTensixMatmulProbe(io,baseline,r,false,24,batch,batchOutput,TT_DATA_TILE_BATCH_MAX,&completedJobs));
	assert(completedJobs == TT_DATA_TILE_BATCH_MAX && io.jobsStarted == TT_DATA_TILE_BATCH_MAX && io.Restored() && !r.recovery_required);
	assert(io.resetWrites == TT_DATA_TILE_BATCH_MAX*4 && baseline.cores_checked == 120);
	assert(io.imageWrites == 2*TT_UNPACK_IMAGE_WORDS+(TT_DATA_TILE_BATCH_MAX-1)*259);
	for (unsigned j = 0; j < TT_DATA_TILE_BATCH_MAX; j++) for (unsigned i = 0; i < 256; i++) assert(batchOutput[j*256+i] == ((j+1)<<16|i));
	io = {}; baseline = io.Baseline();
	assert(!BlackholeTensixMatmulProbe(io,baseline,r,false,24,batch,batchOutput,0,&completedJobs) && completedJobs == 0 && io.configWrites == 0);
	assert(!BlackholeTensixMatmulProbe(io,baseline,r,false,24,batch,batchOutput,TT_DATA_TILE_BATCH_MAX+1,&completedJobs) && io.configWrites == 0);
	batch[15].flags = 1;
	assert(!BlackholeTensixMatmulProbe(io,baseline,r,false,24,batch,batchOutput,TT_DATA_TILE_BATCH_MAX,&completedJobs) && io.configWrites == 0);
	batch[15].flags = 0;
	io = {}; baseline = io.Baseline(); io.sfpuOnly = 24; io.tileInput = batch; io.batching = true; io.timeoutJob = 2;
	assert(!BlackholeTensixMatmulProbe(io,baseline,r,false,24,batch,batchOutput,16,&completedJobs));
	assert(completedJobs == 1 && r.recovery_required && !r.l1_restored && !r.completed && r.stopped);
	io = {}; baseline = io.Baseline(); io.sfpuOnly = 24; io.tileInput = batch; io.batching = true; io.dropImage = TT_UNPACK_IMAGE_WORDS+1;
	assert(!BlackholeTensixMatmulProbe(io,baseline,r,false,24,batch,batchOutput,16,&completedJobs));
	assert(completedJobs == 1 && !r.started && !r.recovery_required && r.l1_restored && io.Restored());
	io = {}; baseline = io.Baseline();
	input.flags = 0; input.weights[255] = 0x7f80;
	assert(!BlackholeTensixMatmulProbe(io,baseline,r,false,24,&input,output) && io.configWrites == 0);
	input.weights[255] = 0; input.activations[0] = 0xffc0;
	assert(!BlackholeTensixMatmulProbe(io,baseline,r,false,24,&input,output) && io.configWrites == 0);
	for (unsigned stage = 3; stage <= 23; stage++) {
		io = {}; baseline = io.Baseline(); io.sfpuOnly = stage;
		assert(BlackholeTensixMatmulProbe(io,baseline,r,false,stage) && r.version == 2+stage && r.completed && io.Restored());
	}
	io = {}; baseline = io.Baseline(); io.sfpuOnly = 16; io.allocationFails = true;
	assert(!BlackholeTensixMatmulProbe(io,baseline,r,false,16) && io.configWrites == 0 && io.imageWrites == 0 && io.resetWrites == 0);
	io = {}; baseline = io.Baseline(); io.sfpuOnly = 16; io.timeout = true;
	assert(!BlackholeTensixMatmulProbe(io,baseline,r,false,16) && r.recovery_required && !r.completed && r.stopped && io.imageWrites == TT_UNPACK_IMAGE_WORDS);
	io = {}; baseline = io.Baseline(); r = {};
	assert(BlackholeClockedMatmulProbe(io,baseline,r,clock));
	assert(io.clockEnter == 1 && io.clockLeave == 1 && clock.restored && io.Restored());
	io = {}; baseline = io.Baseline(); r = {}; io.timeout = true;
	assert(!BlackholeClockedMatmulProbe(io,baseline,r,clock));
	assert(io.clockLeave == 0 && clock.recovery_required && r.recovery_required);
	io = {}; baseline = io.Baseline(); r = {}; io.badMath = true;
	assert(!BlackholeClockedMatmulProbe(io,baseline,r,clock));
	assert(io.clockLeave == 1 && clock.restored && !clock.recovery_required && r.completed && io.Restored());
	io = {}; baseline = io.Baseline(); r = {}; io.enterFails = true;
	assert(!BlackholeClockedMatmulProbe(io,baseline,r,clock));
	assert(io.clockLeave == 1 && clock.restored && io.imageWrites == 0 && io.resetWrites == 0);
	io = {}; baseline = io.Baseline(); r = {}; io.enterUncertain = true;
	assert(!BlackholeClockedMatmulProbe(io,baseline,r,clock));
	assert(io.clockLeave == 0 && clock.recovery_required && io.imageWrites == 0 && io.resetWrites == 0);
	io = {}; baseline = io.Baseline(); r = {}; io.leaveFails = true;
	assert(!BlackholeClockedMatmulProbe(io,baseline,r,clock));
	assert(io.clockLeave == 1 && clock.recovery_required && r.completed && io.Restored());
}
