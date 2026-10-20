/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_TEST_UNSELECTED_QUEUED_ENTRY
#define PIONEER_Q38_VECTOR_PERSISTENT_TEST_MODEL_ONLY
#include "pioneer_q38_vector_persistent_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38InputSumCachePeerImage.h"
// Observe the ACTUAL RV32 loads/stores,not a second cache implementation.
// Only this job's group0 may define each sum;later groups may consume it
// only after the complete first group. Poison EVERY slot between jobs.
struct InputSumCache:VectorPersistent {
	using VectorPersistent::VectorPersistent;
	unsigned sumWrites=0,sumReads=0;
	bool corruptSum=false;
	int Expected(unsigned slot) const {
		int sum=0;
		for (unsigned k=0;k<16;k++) {
			unsigned value=vector.at(slot*16+k);
			sum+=int(value&1023)*((value&0x8000)?-1:1);
		}
		return sum;
	}
	void Poison() {
		sumWrites=sumReads=0;
		for (unsigned slot=0;slot<1088;slot++) memory[0x95000+slot*4]=0x7fffffff;
		memory[0x94ffc]=memory[0x96100]=0xdec0adfe;
	}
	void Store(uint32_t at,uint32_t value) override {
		if (Range(at,0x95000,4352)) {
			assert(memory.at(0x74038)==1 && groups>1 && !pending && drained && !(at&3));
			unsigned slot=(at-0x95000)/4;
			assert(slot==sumWrites && slot<inputs/16 && int(value)==Expected(slot));
			memory[at]=value; sumWrites++; return;
		}
		VectorPersistent::Store(at,value);
	}
	uint32_t Load(uint32_t at) override {
		if (Range(at,0x95000,4352)) {
			assert(memory.at(0x74038)==1 && groups>1 && !pending && sumWrites==inputs/16 && !(at&3));
			unsigned slot=(at-0x95000)/4;
			assert(slot==sumReads%(inputs/16) && int(memory.at(at))==Expected(slot));
			sumReads++; return corruptSum?0x7fffffff:memory.at(at);
		}
		return VectorPersistent::Load(at);
	}
};
#ifndef PIONEER_Q38_INPUT_SUM_CACHE_TEST_MODEL_ONLY
int main(int argc,char** argv)
{
	assert(argc==3);
	setvbuf(stdout,nullptr,_IOLBF,0);
	const auto code=Read((std::string(argv[2])+"/text.bin").c_str());
	const auto prefix=Read((std::string(argv[2])+"/affine.bin").c_str());
	for (unsigned word=0;word<2048;word++) {
		assert(PioneerQ38InputSumCachePeerWord(word)==(word*4<code.size()?Word(code,word*4):0));
		unsigned at=0x70000+word*4;
		assert(PioneerQ38InputSumCachePeerPrefixWord(at)==
			(at>=0x70b00 && at-0x70b00<prefix.size()?Word(prefix,at-0x70b00):0));
	}
	assert(PioneerQ38InputSumCachePeerWord(2048)==UINT32_MAX);
	for (unsigned at:{0x6fffcU,0x72000U,0x70001U}) assert(PioneerQ38InputSumCachePeerPrefixWord(at)==UINT32_MAX);
	for (unsigned type:{8U,12U,14U}) for (unsigned k:{5120U,6144U,17408U})
	for (unsigned groups:{1U,3U,16U}) for (bool work:{false,true}) {
		VectorPersistent old(argv[1],type,k,groups); InputSumCache next(argv[2],type,k,groups);
		assert(old.Boot() && next.Boot());
		old.memory[0x74038]=next.memory[0x74038]=unsigned(work);
		for (unsigned job=1;job<=2;job++) {
			if (job==2) { old.NextPackets(); next.NextPackets(); FreshInputs(old); FreshInputs(next); }
			next.Poison(); unsigned oldStart=old.steps,nextStart=next.steps;
			assert(old.Job(job) && next.Job(job));
			assert(next.sumWrites==unsigned(work && groups>1)*(k/16)
				&& next.sumReads==unsigned(work && groups>1)*(groups-1)*(k/16));
			for (unsigned slot=k/16;slot<1088;slot++) assert(next.memory.at(0x95000+slot*4)==0x7fffffff);
			assert(next.memory.at(0x94ffc)==0xdec0adfe && next.memory.at(0x96100)==0xdec0adfe);
			assert(old.requests==next.requests && old.responses==next.responses && old.drains==next.drains
				&& old.matrixInstructions==next.matrixInstructions && old.vectorOps==next.vectorOps
				&& old.dotProbeReads==next.dotProbeReads && !next.pending && !next.rawPending);
			for (unsigned row=0;row<groups*16;row++) assert(old.buffers[4096+row]==next.buffers[4096+row]);
			if (work && groups==16 && job==2)
				printf("OFFLINE_RV32_STEPS type%u K%u groups%u old%u next%u;NOT_SILICON_OR_TOKEN_RATE\n",
					type,k,groups,old.steps-oldStart,next.steps-nextStart);
		}
		unsigned reads=next.sumReads,writes=next.sumWrites;
		old.memory[0x74038]=next.memory[0x74038]=0;
		assert(old.Job(3,2) && next.Job(3,2) && reads==next.sumReads && writes==next.sumWrites);
	}
	for (unsigned type:{8U,12U,14U}) {
		InputSumCache bad(argv[2],type,5120,3); assert(bad.Boot());
		bad.memory[0x74038]=1; bad.Poison(); bad.corruptSum=true;
		assert(!bad.Job(1) && bad.sumWrites==5120/16 && bad.sumReads==1
			&& bad.requests==bad.inputReads+5120/bad.count+1 && !bad.pending);
		unsigned requests=bad.requests,reads=bad.sumReads,writes=bad.sumWrites;
		for (unsigned step=0;step<1000;step++) bad.StepBlock();
		assert(requests==bad.requests && reads==bad.sumReads && writes==bad.sumWrites);
	}
	puts("PASS OFFLINE actual cache image:ALL formats/widths/1,3,16 groups/two modes/two changed-input jobs EXACT;full fresh group0 before reads;bounded disjoint scratch;unchanged matrix/SFPU/NoC/drains/STOP;NO native/performance qualification");
}
#endif
