/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_ZERO_MINIMUM_IMAGE_TEST_MODEL_ONLY
#include "pioneer_q38_zero_minimum_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38NativeOperandPeerImage.h"
int main(int argc,char** argv)
{
	assert(argc==3); setvbuf(stdout,nullptr,_IOLBF,0);
	const auto code=Read((std::string(argv[2])+"/text.bin").c_str());
	const auto prefix=Read((std::string(argv[2])+"/affine.bin").c_str());
	for (unsigned word=0;word<2048;word++) {
		assert(PioneerQ38NativeOperandPeerWord(word)==(word*4<code.size()?Word(code,word*4):0));
		unsigned at=0x70000+word*4;
		assert(PioneerQ38NativeOperandPeerPrefixWord(at)==
			(at>=0x70b00 && at-0x70b00<prefix.size()?Word(prefix,at-0x70b00):0));
	}
	assert(PioneerQ38NativeOperandPeerWord(2048)==UINT32_MAX);
	for (unsigned at:{0x6fffcU,0x72000U,0x70001U}) assert(PioneerQ38NativeOperandPeerPrefixWord(at)==UINT32_MAX);
	for (unsigned type:{8U,12U,14U}) for (unsigned k:{5120U,6144U,17408U})
	for (unsigned groups:{1U,3U,16U}) for (bool work:{false,true}) {
		ZeroMinimumImage old(argv[1],type,k,groups),next(argv[2],type,k,groups);
		old.PoisonDst(); next.PoisonDst(); assert(old.Boot() && next.Boot());
		old.memory[0x74038]=next.memory[0x74038]=unsigned(work);
		for (unsigned job=1;job<=2;job++) {
			if (job==2) { old.NextPackets(); next.NextPackets(); FreshInputs(old); FreshInputs(next); }
			old.Poison(); next.Poison(); unsigned before=old.steps,after=next.steps;
			assert(old.Job(job) && next.Job(job));
			assert(!old.nativeStoreOps && next.nativeStoreOps==job*8*groups*k/16
				&& old.drains==next.drains && old.requests==next.requests && old.responses==next.responses
				&& old.matrixInstructions==next.matrixInstructions && old.dotProbeReads==next.dotProbeReads
				&& old.sumWrites==next.sumWrites && old.sumReads==next.sumReads && next.vectorOps<old.vectorOps);
			for (unsigned row=0;row<groups*16;row++) assert(old.buffers[4096+row]==next.buffers[4096+row]);
			if (work && groups==16 && job==2)
				printf("OFFLINE_NATIVE_STORE type%u K%u groups%u old_RV%u next_RV%u vector_ops%u/%u;NOT_SILICON_OR_TOKEN_RATE\n",
					type,k,groups,old.steps-before,next.steps-after,old.vectorOps,next.vectorOps);
		}
		old.memory[0x74038]=next.memory[0x74038]=0;
		assert(old.Job(3,2) && next.Job(3,2));
	}
	for (unsigned type:{8U,12U,14U}) {
		ZeroMinimumImage bad(argv[2],type,5120,1); assert(bad.Boot());
		bad.memory[0x74038]=1; bad.Poison(); bad.poisonNativeLow=true; bool rejected=false;
		try { (void)bad.Job(1); } catch (const ZeroMinimumImage::NativeStoreHazard&) { rejected=true; }
		assert(rejected && !bad.nativeStoreOps && !bad.matrixInstructions && !bad.outputReads);
		ZeroMinimumImage replay(argv[2],type,5120,1); assert(replay.Boot());
		replay.memory[0x74038]=1; replay.Poison(); replay.dropReplayRecord=true; rejected=false;
		try { (void)replay.Job(1); } catch (const ZeroMinimumImage::ReplayAccessFault&) { rejected=true; }
		assert(rejected && !replay.matrixInstructions && !replay.outputReads);
	}
	puts("PASS OFFLINE actual native-store peer:ALL formats/widths/groups/modes/repeated changed inputs EXACT vs old/RISC/FP64;poisoned Dst initialized,only original high physical halves modified,zero low halves mandatory,unchanged MOV/CSR/matrix/NoC/drains/STOP,undefined replay rejects;NO native/performance qualification");
}
