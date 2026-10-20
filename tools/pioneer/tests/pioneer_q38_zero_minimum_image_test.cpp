/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_INPUT_SUM_CACHE_TEST_MODEL_ONLY
#include "pioneer_q38_input_sum_cache_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ZeroMinimumPeerImage.h"
struct ZeroMinimumImage:InputSumCache {
	using InputSumCache::InputSumCache;
	bool dropAccumulatorClear=false;
	unsigned droppedClears=0;
	void Instruction(uint32_t op) override {
		if (dropAccumulatorClear && (op==0x7277006c || op==0x7277006e)) { droppedClears++; return; }
		InputSumCache::Instruction(op);
	}
	void PoisonDst() {
		// Stale physical halves include negative-zero accumulators. The real
		// program must initialize them; a host default-zero model must not.
		for (unsigned row=0;row<1024;row++) {
			defined[row]=true;
			for (unsigned lane=0;lane<16;lane++) { physical[row][lane]=uint16_t(0x8000+row*17+lane); wrote[row][lane]=true; }
		}
	}
};
#ifndef PIONEER_Q38_ZERO_MINIMUM_IMAGE_TEST_MODEL_ONLY
int main(int argc,char** argv)
{
	assert(argc==3); setvbuf(stdout,nullptr,_IOLBF,0);
	const auto code=Read((std::string(argv[2])+"/text.bin").c_str());
	const auto prefix=Read((std::string(argv[2])+"/affine.bin").c_str());
	for (unsigned word=0;word<2048;word++) {
		assert(PioneerQ38ZeroMinimumPeerWord(word)==(word*4<code.size()?Word(code,word*4):0));
		unsigned at=0x70000+word*4;
		assert(PioneerQ38ZeroMinimumPeerPrefixWord(at)==
			(at>=0x70b00 && at-0x70b00<prefix.size()?Word(prefix,at-0x70b00):0));
	}
	assert(PioneerQ38ZeroMinimumPeerWord(2048)==UINT32_MAX);
	for (unsigned at:{0x6fffcU,0x72000U,0x70001U}) assert(PioneerQ38ZeroMinimumPeerPrefixWord(at)==UINT32_MAX);
	for (unsigned type:{8U,12U,14U}) for (unsigned k:{5120U,6144U,17408U})
	for (unsigned groups:{1U,3U,16U}) for (bool work:{false,true}) {
		InputSumCache old(argv[1],type,k,groups); ZeroMinimumImage next(argv[2],type,k,groups);
		next.PoisonDst(); assert(old.Boot() && next.Boot());
		old.memory[0x74038]=next.memory[0x74038]=unsigned(work);
		for (unsigned job=1;job<=2;job++) {
			if (job==2) { old.NextPackets(); next.NextPackets(); FreshInputs(old); FreshInputs(next); }
			old.Poison(); next.Poison(); unsigned first=old.steps,second=next.steps;
			assert(old.Job(job) && next.Job(job));
			assert(old.requests==next.requests && old.responses==next.responses && old.matrixInstructions==next.matrixInstructions
				&& next.sumWrites==(type==12?old.sumWrites:0) && next.sumReads==(type==12?old.sumReads:0)
				&& old.dotProbeReads==next.dotProbeReads);
			assert(old.drains-next.drains==unsigned(work && type==8)*job*groups*(k/32));
			assert(next.memory.at(0x94ffc)==0xdec0adfe && next.memory.at(0x96100)==0xdec0adfe);
			if (type!=12) for (unsigned slot=0;slot<1088;slot++) assert(next.memory.at(0x95000+slot*4)==0x7fffffff);
			assert(work && type!=12?next.vectorOps<old.vectorOps:next.vectorOps==old.vectorOps);
			for (unsigned row=0;row<groups*16;row++) assert(old.buffers[4096+row]==next.buffers[4096+row]);
			if (work && groups==16 && job==2)
				printf("OFFLINE_ZERO_MINIMUM type%u K%u groups%u old_RV%u next_RV%u vector_ops%u/%u;NOT_SILICON_OR_TOKEN_RATE\n",
					type,k,groups,old.steps-first,next.steps-second,old.vectorOps,next.vectorOps);
		}
		old.memory[0x74038]=next.memory[0x74038]=0;
		assert(old.Job(3,2) && next.Job(3,2));
	}
	for (unsigned type:{8U,14U}) {
		ZeroMinimumImage bad(argv[2],type,5120,3); assert(bad.Boot());
		bad.memory[0x74038]=1; bad.Poison(); bad.dropAccumulatorClear=true;
		assert(!bad.Job(1) && bad.droppedClears); // clearing EACH group is NECESSARY
	}
	{
		ZeroMinimumImage bad(argv[2],12,5120,3); assert(bad.Boot());
		bad.memory[0x74038]=1; bad.Poison(); bad.corruptSum=true;
		assert(!bad.Job(1) && bad.sumReads==1 && !bad.pending);
		unsigned requests=bad.requests; for (unsigned step=0;step<1000;step++) bad.StepBlock();
		assert(requests==bad.requests); // Q4's original sum bound is STILL mandatory
	}
	puts("PASS OFFLINE actual zero-minimum Work image:ALL3 formats/3 widths/1,3,16 groups/two modes/two changed-input jobs bit EXACT vs cached original+independent references;poisoned physical Dst initialized,per-group clear negative control,unchanged matrix/NoC/cache/STOP,checked/Q4 retain full arithmetic;NO native/performance qualification");
}
#endif
