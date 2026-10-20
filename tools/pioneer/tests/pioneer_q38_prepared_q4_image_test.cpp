/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_DEFER_AFFINE_IMAGE_TEST_MODEL_ONLY
#include "pioneer_q38_defer_affine_image_test.cpp"
#include "../PioneerQ38PreparedQ4.h"
// The candidate consumes DIFFERENT transport bytes. Keep independent
// original-format FP32+FP64 references,not a self-reference to prepared data.
struct PreparedQ4Image:DeferredAffineImage {
	mutable DeferredAffineImage original;
	PreparedQ4Image(const char* reference,const char* candidate,unsigned type,unsigned k,unsigned groups)
		:DeferredAffineImage(candidate,type,k,groups),original(reference,type,k,groups) {
		if (type!=12) return;
		for (auto& bytes:tiles) {
			std::vector<uint8_t> expanded(PioneerQ38PreparedQ4::Bytes,0x5a);
			assert(PioneerQ38PreparedQ4::Prepare(bytes.data(),unsigned(bytes.size()),expanded.data(),unsigned(expanded.size())));
			bytes.swap(expanded);
		}
		tile=tiles[0]; memory[0x74008]=unsigned(tile.size());
		for (unsigned word=0;word<tile.size()/4;word++) memory[0x90000+word*4]=Word(tile,word*4);
		for (auto& p:packets) if (p.tile>=0) { p.address=base+unsigned(p.tile)*unsigned(tile.size()); p.bytes=unsigned(tile.size()); }
	}
	bool CorrectRows() const override {
		original.vector=vector; original.matrixInstructions=matrixInstructions;
		memcpy(original.buffers,buffers,sizeof(buffers));
		return original.CorrectRows(); // ORIGINAL quant bytes + independently decoded weights
	}
};
int main(int argc,char** argv)
{
	assert(argc==3 || (argc==4 && !strcmp(argv[3],"--q4-only"))); setvbuf(stdout,nullptr,_IOLBF,0);
	for (unsigned type:{8U,12U,14U}) for (unsigned k:{5120U,6144U,17408U})
	for (unsigned groups:{1U,3U,16U}) for (bool work:{false,true}) {
		if (argc==4 && type!=12) continue;
		DeferredAffineImage old(argv[1],type,k,groups);
		PreparedQ4Image next(argv[1],argv[2],type,k,groups);
		assert(old.Boot() && next.Boot()); old.memory[0x74038]=next.memory[0x74038]=unsigned(work);
		for (unsigned job=1;job<=2;job++) {
			if (job==2) { old.NextPackets(); next.NextPackets(); FreshInputs(old); FreshInputs(next); }
			old.Poison(); next.Poison(); unsigned before=old.steps,after=next.steps;
			bool first=old.Job(job),second=next.Job(job);
			if (!first || !second) fprintf(stderr,"PREPARED_IMAGE_DIAGNOSTIC type%u K%u groups%u work%u job%u old%u next%u pc%08x RA%08x requests%u matrix%u marker%08x\n",
				type,k,groups,unsigned(work),job,unsigned(first),unsigned(second),next.pc,next.regs[1],next.requests,next.matrixInstructions,next.memory.at(0x74148));
			assert(first && second);
			assert(old.pendingAccumulator.empty() && next.pendingAccumulator.empty()
				&& old.committedStores==next.committedStores && old.overlappedStores==next.overlappedStores
				&& old.vectorOps==next.vectorOps && old.drains==next.drains && old.rawReads==next.rawReads
				&& old.requests==next.requests && old.responses==next.responses
				&& old.matrixInstructions==next.matrixInstructions && old.dotProbeReads==next.dotProbeReads
				&& old.sumWrites==next.sumWrites && old.sumReads==next.sumReads);
			for (unsigned row=0;row<groups*16;row++) assert(old.buffers[4096+row]==next.buffers[4096+row]);
			if (work && groups==16 && job==2)
				printf("OFFLINE_PREPARED_Q4_IMAGE type%u K%u groups%u original_RV%u prepared_RV%u;DIFFERENT_PACKING_EXACT_OUTPUT_NOT_SILICON_OR_TOKEN_RATE\n",
					type,k,groups,old.steps-before,next.steps-after);
		}
		old.memory[0x74038]=next.memory[0x74038]=0;
		assert(old.Job(3,2) && next.Job(3,2));
	}
	for (unsigned word:{0U,1U,15U,16U,31U,32U,127U,128U,254U,255U}) for (bool work:{false,true}) {
		PreparedQ4Image bad(argv[1],argv[2],12,5120,1); assert(bad.Boot());
		bad.tiles[0][2048+word*3+2]|=0x80; bad.Poison(); bad.memory[0x74038]=unsigned(work);
		assert(!bad.Job(1) && !bad.memory.at(0x74144)); unsigned requests=bad.requests;
		for (unsigned step=0;step<1000;step++) bad.StepBlock(); assert(bad.requests==requests);
	}
	for (unsigned fault:{1U,5U,40U,208U}) {
		PreparedQ4Image bad(argv[1],argv[2],12,17408,3); assert(bad.Boot()); bad.Poison(); bad.faultAt=fault;
		assert(!bad.Job(1) && bad.pending && bad.requests==fault && bad.responses==fault-1 && !bad.memory.at(0x74144));
		unsigned requests=bad.requests; for (unsigned step=0;step<1000;step++) bad.StepBlock(); assert(bad.requests==requests);
	}
	if (argc==4) puts("PASS OFFLINE prepared-Q4-only image subset+faults;Q8/Q6 SKIPPED,NO native/performance qualification");
	else puts("PASS OFFLINE actual prepared-Q4 image:ALL formats/widths/groups/modes/two changed-input jobs EXACT against ORIGINAL-format FP32+independent FP64,unchanged quant operands/vector/matrix/drain/NoC request count/cache/STOP,20 malformed-metadata+4 packet faults inert before completion;NO native reader/registry/driver selection or performance claim");
}
