/* SPDX-License-Identifier: MIT */
// OFFLINE actual RV32/Tensix execution. Observe every configuration access;
// inlining must not replace ordered volatile write/readback with a cached value.
#define PIONEER_Q38_DEFER_AFFINE_IMAGE_TEST_MODEL_ONLY
#include "pioneer_q38_defer_affine_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38InlineConfigPeerImage.h"
#include <tuple>
struct InlineConfigImage:DeferredAffineImage {
	using DeferredAffineImage::DeferredAffineImage;
	using Access=std::tuple<bool,uint32_t,uint32_t>;
	std::vector<Access> configuration;
	unsigned reads=0,failRead=0;
	bool injected=false;
	void ClearTrace() { configuration.clear(); reads=0; }
	void Store(uint32_t at,uint32_t value) override {
		DeferredAffineImage::Store(at,value);
		if (Range(at,0xffef0000,224*4)) configuration.emplace_back(true,at,value);
	}
	uint32_t Load(uint32_t at) override {
		uint32_t value=DeferredAffineImage::Load(at);
		if (Range(at,0xffef0000,224*4)) {
			configuration.emplace_back(false,at,value);
			if (++reads==failRead) { injected=true; return value^1; }
		}
		return value;
	}
};
int main(int argc,char** argv)
{
	assert(argc==3 || (argc==4 && !strcmp(argv[3],"--faults-only"))); setvbuf(stdout,nullptr,_IOLBF,0);
	const auto code=Read((std::string(argv[2])+"/text.bin").c_str());
	const auto prefix=Read((std::string(argv[2])+"/affine.bin").c_str());
	for (unsigned word=0;word<2048;word++) {
		assert(PioneerQ38InlineConfigPeerWord(word)==(word*4<code.size()?Word(code,word*4):0));
		unsigned at=0x70000+word*4;
		assert(PioneerQ38InlineConfigPeerPrefixWord(at)==
			(at>=0x70b00 && at-0x70b00<prefix.size()?Word(prefix,at-0x70b00):0));
	}
	assert(PioneerQ38InlineConfigPeerWord(2048)==UINT32_MAX);
	for (unsigned at:{0x6fffcU,0x72000U,0x70001U}) assert(PioneerQ38InlineConfigPeerPrefixWord(at)==UINT32_MAX);
	if (argc==3) for (unsigned type:{8U,12U,14U}) for (unsigned k:{5120U,6144U,17408U})
	for (unsigned groups:{1U,3U,16U}) for (bool work:{false,true}) {
		InlineConfigImage old(argv[1],type,k,groups),next(argv[2],type,k,groups);
		assert(old.Boot() && next.Boot() && old.configuration==next.configuration);
		old.memory[0x74038]=next.memory[0x74038]=unsigned(work);
		for (unsigned job=1;job<=2;job++) {
			if (job==2) { old.NextPackets(); next.NextPackets(); FreshInputs(old); FreshInputs(next); }
			old.Poison(); next.Poison(); old.ClearTrace(); next.ClearTrace();
			unsigned before=old.steps,after=next.steps;
			assert(old.Job(job) && next.Job(job));
			assert(!old.configuration.empty() && old.configuration==next.configuration && old.reads==next.reads);
			assert(old.pendingAccumulator.empty() && next.pendingAccumulator.empty()
				&& old.committedStores==next.committedStores && old.overlappedStores==next.overlappedStores
				&& old.vectorOps==next.vectorOps && old.drains==next.drains
				&& old.requests==next.requests && old.responses==next.responses
				&& old.matrixInstructions==next.matrixInstructions && old.dotProbeReads==next.dotProbeReads
				&& old.sumWrites==next.sumWrites && old.sumReads==next.sumReads);
			for (unsigned row=0;row<groups*16;row++) assert(old.buffers[4096+row]==next.buffers[4096+row]);
			if (work && groups==16 && job==2)
				printf("OFFLINE_INLINE_CONFIG type%u K%u groups%u old_RV%u next_RV%u config_reads%u;EXACT_TRACE_NOT_SILICON_OR_TOKEN_RATE\n",
					type,k,groups,old.steps-before,next.steps-after,next.reads);
		}
		old.memory[0x74038]=next.memory[0x74038]=0;
		old.ClearTrace(); next.ClearTrace(); assert(old.Job(3,2) && next.Job(3,2));
		assert(old.configuration==next.configuration);
	}
	for (unsigned type:{8U,12U,14U}) for (bool work:{false,true}) for (unsigned read:{1U,20U,60U}) {
		InlineConfigImage bad(argv[2],type,5120,1); assert(bad.Boot());
		bad.ClearTrace(); bad.failRead=read; bad.memory[0x74038]=unsigned(work); bad.Poison();
		bool completed=bad.Job(1);
		printf("OFFLINE_INLINE_CONFIG_FAULT type%u work%u read%u completed%u injected%u reads%u outputReads%u completion%u\n",
			type,unsigned(work),read,unsigned(completed),unsigned(bad.injected),bad.reads,bad.outputReads,bad.memory.at(0x74144));
		// Checked mode's ORIGINAL integer-dot diagnostic reads are not an
		// output publication. Work has none;both must withhold completion.
		assert(!completed && bad.injected && bad.reads==read && (!work || !bad.outputReads) && !bad.memory.at(0x74144));
		unsigned requests=bad.requests,accesses=unsigned(bad.configuration.size());
		for (unsigned step=0;step<1000;step++) bad.StepBlock();
		assert(bad.requests==requests && bad.configuration.size()==accesses);
	}
	if (argc==4) puts("PASS OFFLINE ONLY18 actual inline-config readback-fault cases;successful-job equivalence SKIPPED;NO native/performance qualification");
	else puts("PASS OFFLINE actual inline-config image:all formats/widths/groups/modes/repeated changed inputs EXACT,EVERY ordered volatile CSR write/readback same address/value/order,unchanged matrix/vector/drain/NoC/cache/STOP,18 readback faults inert park before publication;NO native/performance qualification");
}
