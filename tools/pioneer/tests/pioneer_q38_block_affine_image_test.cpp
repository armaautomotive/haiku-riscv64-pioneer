/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_DEFER_AFFINE_IMAGE_TEST_MODEL_ONLY
#include "pioneer_q38_defer_affine_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38BlockAffinePeerImage.h"
int main(int argc,char** argv) {
	assert(argc==3 || (argc==4 && (!strcmp(argv[3],"--q4-work-pilot") || !strcmp(argv[3],"--faults-only")))); setvbuf(stdout,nullptr,_IOLBF,0);
	const auto code=Read((std::string(argv[2])+"/text.bin").c_str());
	const auto prefix=Read((std::string(argv[2])+"/affine.bin").c_str());
	for (unsigned w=0;w<2048;w++) {
		assert(PioneerQ38BlockAffinePeerWord(w)==(w*4<code.size()?Word(code,w*4):0));
		unsigned at=0x70000+w*4;
		assert(PioneerQ38BlockAffinePeerPrefixWord(at)==(at>=0x70b00 && at-0x70b00<prefix.size()?Word(prefix,at-0x70b00):0));
	}
	assert(PioneerQ38BlockAffinePeerWord(2048)==UINT32_MAX);
	for (unsigned at:{0x6fffcU,0x72000U,0x70001U}) assert(PioneerQ38BlockAffinePeerPrefixWord(at)==UINT32_MAX);
	for (unsigned type:{8U,12U,14U}) for (unsigned k:{5120U,6144U,17408U})
	for (unsigned groups:{1U,3U,16U}) for (bool work:{false,true}) {
		if (argc==4 && (!strcmp(argv[3],"--faults-only") || type!=12 || k!=5120 || groups!=1 || !work)) continue;
		printf("BLOCK_CASE type%u K%u groups%u work%u\n",type,k,groups,unsigned(work));
		DeferredAffineImage old(argv[1],type,k,groups),next(argv[2],type,k,groups);
		old.PoisonDst(); next.PoisonDst(); assert(old.Boot() && next.Boot());
		old.memory[0x74038]=next.memory[0x74038]=unsigned(work);
		for (unsigned job=1;job<=2;job++) {
			if (job==2) { old.NextPackets(); next.NextPackets(); FreshInputs(old); FreshInputs(next); }
			old.Poison(); next.Poison(); unsigned before=old.steps,after=next.steps;
			printf("BLOCK_JOB %u OLD\n",job); assert(old.Job(job));
			printf("BLOCK_JOB %u NEXT\n",job); assert(next.Job(job));
			assert(old.requests==next.requests && old.responses==next.responses && old.matrixInstructions==next.matrixInstructions
				&& old.dotProbeReads==next.dotProbeReads && old.sumWrites==next.sumWrites && old.sumReads==next.sumReads
				&& old.pendingAccumulator.empty() && next.pendingAccumulator.empty());
			for (unsigned row=0;row<groups*16;row++) assert(old.buffers[4096+row]==next.buffers[4096+row]);
			if (work && type==12) {
				assert(next.blockInitializations==job*4 && next.blockImports==job*groups*(k/256)*16);
				assert(next.configWrites<old.configWrites && next.drains<old.drains);
				if (groups==16 && job==2) printf("OFFLINE_BLOCK_AFFINE K%u old_RV%u next_RV%u config%u/%u drains%u/%u;NOT_SILICON_OR_TOKEN_RATE\n",
					k,old.steps-before,next.steps-after,old.configWrites,next.configWrites,old.drains,next.drains);
			} else assert(!next.blockInitializations && !next.blockImports);
		}
		old.memory[0x74038]=next.memory[0x74038]=0;
		assert(old.Job(3,2) && next.Job(3,2));
	}
	if (argc==4 && !strcmp(argv[3],"--q4-work-pilot")) { puts("PASS PARTIAL ONLY block-affine Q4 Work K5120/group1;FULL suite SKIPPED"); return 0; }
	for (unsigned missing:{1U,8U,16U}) {
		DeferredAffineImage m(argv[2],12,5120,1); assert(m.Boot()); m.memory[0x74038]=1; m.Poison(); m.dropBlockImport=missing;
		bool caught=false; try { (void)m.Job(1); } catch (const DeferredAffineImage::BlockAffineHazard&) { caught=true; }
		assert(caught && !m.outputReads && !m.memory.at(0x74144));
	}
	{
		DeferredAffineImage m(argv[2],12,5120,1); assert(m.Boot()); m.memory[0x74038]=1; m.Poison(); m.omitBlockInitialization=true;
		bool caught=false; try { (void)m.Job(1); } catch (const DeferredAffineImage::BlockAffineHazard&) { caught=true; }
		assert(caught && !m.outputReads && !m.memory.at(0x74144));
	}
	if (argc==4) puts("PASS PARTIAL ONLY block-affine missing imports/padding initialization refused by independent observer;FULL suite SKIPPED");
	else puts("PASS OFFLINE ACTUAL block-affine peer:all formats/widths/groups/modes,repeated changed inputs bit EXACT,unchanged matrix/NoC/source/cache/STOP,disjoint initialized Dst128..191/owned scratch,missing first/middle/last imports and padding initialization detected by independent observer BEFORE publication;NO native/throughput qualification");
}
