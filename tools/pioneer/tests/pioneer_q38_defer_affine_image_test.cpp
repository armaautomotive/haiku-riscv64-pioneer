/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_ZERO_MINIMUM_IMAGE_TEST_MODEL_ONLY
#include "pioneer_q38_zero_minimum_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DeferredAffinePeerImage.h"
#include <map>
// Independent deferred SFPU STORE observer. Data stays invisible until an
// ACTUAL backend drain. Only disjoint SrcB-L1/input-sum/NIU staging may overlap it;
// configuration,accumulator loads and CPU Dst export MUST still wait.
struct DeferredAffineImage:ZeroMinimumImage {
	using ZeroMinimumImage::ZeroMinimumImage;
	using Cell=std::pair<unsigned,unsigned>;
	std::map<Cell,uint32_t> pendingAccumulator;
	unsigned overlappedStores=0,committedStores=0;
	bool loseFinalWait=false;
	struct DeferredHazard {};
	bool Boot() { PoisonDst(); Poison(); return ZeroMinimumImage::Boot(); }
	void NextPackets() { assert(pendingAccumulator.empty()); ZeroMinimumImage::NextPackets(); Poison(); }
	void Instruction(uint32_t op) override {
		if ((op&~2U)==0x7053006c) {
			unsigned parity=(op&2)!=0;
			for (unsigned row=108;row<112;row++) for (unsigned col=parity;col<16;col+=2)
				if (pendingAccumulator.count({row,col})) throw DeferredHazard{};
		}
		if ((op&~2U)==0x7203006c) {
			unsigned parity=(op&2)!=0; std::map<Cell,uint32_t> before;
			for (unsigned row=108;row<112;row++) for (unsigned col=parity;col<16;col+=2) {
				if (pendingAccumulator.count({row,col})) throw DeferredHazard{};
				before[{row,col}]=FloatDst(row,col);
			}
			ZeroMinimumImage::Instruction(op);
			for (const auto& cell:before) {
				pendingAccumulator[cell.first]=FloatDst(cell.first.first,cell.first.second);
				StoreFloatDst(cell.first.first,cell.first.second,cell.second);
			}
			assert(pendingAccumulator.size()<=64); return;
		}
		if (op==0x4500017e && !pendingAccumulator.empty()) {
			bool final=matrixInstructions && !(matrixInstructions%(MatrixPhasesPerPanel()*(inputs/16)));
			if (!(loseFinalWait && final)) {
				for (const auto& cell:pendingAccumulator) StoreFloatDst(cell.first.first,cell.first.second,cell.second);
				committedStores+=unsigned(pendingAccumulator.size()); pendingAccumulator.clear();
			}
		}
		ZeroMinimumImage::Instruction(op);
	}
	void Store(uint32_t at,uint32_t value) override {
		if (!pendingAccumulator.empty()) {
			if (Range(at,0xffef0000,224*4) || Range(at,0xffbd8000,0x4000)) throw DeferredHazard{};
			bool separate=Range(at,0x70920,32) || Range(at,0x95000,1088*4)
				|| at==0xffb20000 || at==0xffb20008
				|| at==0xffb2000c || at==0xffb20020 || at==0xffb20040;
			if (separate) {
				assert(!rawPending && phaseQueue.empty() && !recordRemaining);
				// Parent still validates exact L1 bounds/operand words,fresh
				// per-job integer sums and canaries,NIU
				// command/route/counters and NO overlapping NoC transaction.
				// Replace only its conservative ALL-TENSIX-idle precondition
				// with this explicitly observed SFPU-STORE-only condition.
				bool saved=drained; drained=true;
				try { ZeroMinimumImage::Store(at,value); } catch (...) { drained=saved; throw; }
				drained=saved; overlappedStores++; return;
			}
		}
		ZeroMinimumImage::Store(at,value);
	}
	uint32_t Load(uint32_t at) override {
		if (!pendingAccumulator.empty() && (Range(at,0xffef0000,224*4) || Range(at,0xffbd8000,0x4000)))
			throw DeferredHazard{};
		return ZeroMinimumImage::Load(at);
	}
};
#ifndef PIONEER_Q38_DEFER_AFFINE_IMAGE_TEST_MODEL_ONLY
int main(int argc,char** argv)
{
	assert(argc==3); setvbuf(stdout,nullptr,_IOLBF,0);
	const auto code=Read((std::string(argv[2])+"/text.bin").c_str());
	const auto prefix=Read((std::string(argv[2])+"/affine.bin").c_str());
	for (unsigned word=0;word<2048;word++) {
		assert(PioneerQ38DeferredAffinePeerWord(word)==(word*4<code.size()?Word(code,word*4):0));
		unsigned at=0x70000+word*4;
		assert(PioneerQ38DeferredAffinePeerPrefixWord(at)==
			(at>=0x70b00 && at-0x70b00<prefix.size()?Word(prefix,at-0x70b00):0));
	}
	assert(PioneerQ38DeferredAffinePeerWord(2048)==UINT32_MAX);
	for (unsigned at:{0x6fffcU,0x72000U,0x70001U}) assert(PioneerQ38DeferredAffinePeerPrefixWord(at)==UINT32_MAX);
	for (unsigned type:{8U,12U,14U}) for (unsigned k:{5120U,6144U,17408U})
	for (unsigned groups:{1U,3U,16U}) for (bool work:{false,true}) {
		DeferredAffineImage old(argv[1],type,k,groups),next(argv[2],type,k,groups);
		old.PoisonDst(); next.PoisonDst(); assert(old.Boot() && next.Boot());
		old.memory[0x74038]=next.memory[0x74038]=unsigned(work);
		for (unsigned job=1;job<=2;job++) {
			if (job==2) { old.NextPackets(); next.NextPackets(); FreshInputs(old); FreshInputs(next); }
			old.Poison(); next.Poison(); unsigned before=old.steps,after=next.steps;
			assert(old.Job(job) && next.Job(job));
			assert(old.pendingAccumulator.empty() && next.pendingAccumulator.empty() && old.committedStores==next.committedStores
				// Backend drains are counted separately from SFPU operations.
				&& old.vectorOps==next.vectorOps
				&& old.drains==next.drains+unsigned(work)*job*groups*(k/16-1)
				&& old.requests==next.requests && old.responses==next.responses
				&& old.matrixInstructions==next.matrixInstructions && old.dotProbeReads==next.dotProbeReads);
			assert(work?next.overlappedStores>0:next.overlappedStores==0);
			for (unsigned row=0;row<groups*16;row++) assert(old.buffers[4096+row]==next.buffers[4096+row]);
			if (work && groups==16 && job==2)
				printf("OFFLINE_DEFER_AFFINE type%u K%u groups%u old_RV%u next_RV%u drains%u/%u;NOT_SILICON_OR_TOKEN_RATE\n",
					type,k,groups,old.steps-before,next.steps-after,old.drains,next.drains);
		}
		old.memory[0x74038]=next.memory[0x74038]=0;
		assert(old.Job(3,2) && next.Job(3,2));
	}
	for (unsigned type:{8U,12U,14U}) {
		DeferredAffineImage bad(argv[2],type,5120,1); assert(bad.Boot());
		bad.memory[0x74038]=1; bad.Poison(); bad.loseFinalWait=true; bool rejected=false;
		try { (void)bad.Job(1); } catch (const DeferredAffineImage::DeferredHazard&) { rejected=true; }
		assert(rejected && !bad.pendingAccumulator.empty() && !bad.outputReads && !bad.memory.at(0x74144));
	}
	puts("PASS OFFLINE actual deferred-affine image:all formats/widths/groups/modes/repeated changed inputs EXACT,DEFERRED physical stores,only disjoint SrcB/sum-cache/NoC staging overlap,all config/Dst consumers behind real drains,mandatory final export wait,false ACK rejects,checked/STOP unchanged;NO native/performance qualification");
}
#endif
