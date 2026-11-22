/* SPDX-License-Identifier: MIT */
#define main q35_dynamic_fixture_main
#include "pioneer_qwen35_dataflow_stream_recorder_test.cpp"
#undef main
#include "../PioneerQwen35DataflowDynamicToken.h"
#include <vector>

struct CaptureSink {
	std::vector<DataflowStreamRecord> records;
	bool Begin(const DataflowStreamPlan& p) { records.reserve(size_t(p.count)*DataflowCoreCount);return true; }
	bool StageRecord(const DataflowStreamBinding&,const Range&,unsigned,const DataflowStreamRecord& r) {
		records.push_back(r);return true;
	}
	bool Seal(const DataflowStreamPlan& p) { return records.size()==size_t(p.count)*DataflowCoreCount; }
	void Abort() {}
};
struct CompareSink {
	const CaptureSink& captured;const DataflowDynamicTokenContext& run;
	size_t current=0;unsigned changed=0;
	bool Begin(const DataflowStreamPlan&) { return true; }
	bool StageRecord(const DataflowStreamBinding& b,const Range&,unsigned index,const DataflowStreamRecord& expected) {
		assert(current<captured.records.size());
		DataflowStreamRecord patched=captured.records[current++];
		if (std::memcmp(&patched,&expected,sizeof(patched))) changed++;
		assert(DataflowDynamicPatch(patched,run,b.epoch,index+1,b.core));
		if (std::memcmp(&patched,&expected,sizeof(patched))) {
			std::fprintf(stderr,"dynamic mismatch step=%u core=%u magic=%08x\n",index,b.core,expected.header[5]);
			assert(false);
		}
		return true;
	}
	bool Seal(const DataflowStreamPlan&) { return current==captured.records.size(); }
	void Abort() {}
};
struct FetchIO {
	const DataflowStreamRecord& record;
	bool ReadStream(unsigned,unsigned,unsigned,uint8_t* output,unsigned bytes) {
		if (bytes!=sizeof(record)) return false;
		std::memcpy(output,&record,bytes);return true;
	}
};
int main() {
	(void)&q35_dynamic_fixture_main;
	auto fixture=std::make_unique<Fixture>();auto live=std::make_unique<DataflowProjectionAuthority>();
	assert(live->BindValidatedModel(fixture->manifest,fixture->workspace,fixture->routes,fixture->rope));
	auto plan=std::make_unique<DataflowStreamPlan>();
	assert(DataflowStreamPlanBuilder::Build(fixture->packed,fixture->manifest,fixture->workspace,
		fixture->rope,fixture->routes,99,548,*plan));
	auto candidate0=std::make_unique<DataflowProjectionAuthority>();
	auto candidate1=std::make_unique<DataflowProjectionAuthority>();
	auto candidate2=std::make_unique<DataflowProjectionAuthority>();
	auto scratch=std::make_unique<DataflowStreamRecord>();auto projection=std::make_unique<DataflowProjectionProgram>();
	CaptureSink base;unsigned stage=0;
	assert(DataflowStreamRecording::Build(fixture->packed,*live,*candidate0,*plan,fixture->routes,
		0,42,0,true,base,*scratch,*projection,stage));
	const unsigned otherToken=fixture->manifest.tied.place.parts[0].rows+17;
	DataflowDynamicTokenContext dynamic1{};
	assert(DataflowDynamicTokenContext::Build(fixture->manifest,otherToken,1,dynamic1));
	CompareSink compare1{base,dynamic1};
	assert(DataflowStreamRecording::Build(fixture->packed,*candidate0,*candidate1,*plan,fixture->routes,
		1,otherToken,0,true,compare1,*scratch,*projection,stage));
	assert(compare1.changed>0);
	FetchIO read{base.records[0]};DataflowStreamRecord staging{};
	alignas(64) uint32_t program[DataflowProgramRegionBytes/4]{};
	assert(DataflowStreamFetchDynamic(read,plan->binding[0],plan->lease[0],fixture->routes,
		0,0,staging,program,sizeof(program),dynamic1));
	DataflowEmbeddingInvocation patchedEmbedding{};
	std::memcpy(&patchedEmbedding,program,sizeof(patchedEmbedding));
	assert(patchedEmbedding.Valid() && patchedEmbedding.token==otherToken && patchedEmbedding.lane==(otherToken&15));
	auto rejectedRun=dynamic1;rejectedRun.token=Vocabulary;
	program[0]=0xfeedface;program[1]=0xfacefeed;
	assert(!DataflowStreamFetchDynamic(read,plan->binding[0],plan->lease[0],fixture->routes,
		0,0,staging,program,sizeof(program),rejectedRun));
	assert(program[0]==0 && program[1]==0xfacefeed);
	DataflowDynamicTokenContext dynamic2{};
	assert(DataflowDynamicTokenContext::Build(fixture->manifest,Vocabulary-1,2,dynamic2));
	CompareSink compare2{base,dynamic2};
	assert(DataflowStreamRecording::Build(fixture->packed,*candidate1,*candidate2,*plan,fixture->routes,
		2,Vocabulary-1,0,true,compare2,*scratch,*projection,stage));
	assert(compare2.changed>0 && !live->ActivationReady(0));
	// Compare against the native typed builders at the final context position,
	// including both active and inactive cores and the last vocabulary token.
	DataflowDynamicTokenContext edge{};
	assert(DataflowDynamicTokenContext::Build(fixture->manifest,Vocabulary-1,fixture->manifest.context-1,edge));
	bool tested[4]{};
	for (unsigned step=0;step<548;step++) {
		const auto magic=base.records[size_t(step)*DataflowCoreCount].header[5];
		const unsigned which=magic==DataflowEmbeddingMagic?0:magic==DataflowConvMagic?1:
			magic==DataflowDeltaMagic?2:magic==DataflowDenseAttentionMagic?3:4;
		if (which==4 || tested[which]) continue;
		tested[which]=true;
		for (unsigned core:{0U,7U,119U}) {
			const auto& original=base.records[size_t(step)*DataflowCoreCount+core];
			DataflowStreamRecord expected{},patched=original;
			if (which==0) {
				DataflowEmbeddingInvocation old{},target{};DataflowDynamicRead(original,old);
				DataflowEmbeddingProgram p{};
				assert(DataflowEmbeddingProgramBuilder::Build(fixture->manifest,edge.token,0,fixture->routes,p)
					&& DataflowEmbeddingProgramBuilder::BindCore(p,core,target)
					&& DataflowStreamEncode(target,99,step+1,expected));
			} else if (which==1) {
				DataflowConvInvocation old{},target{};DataflowDynamicRead(original,old);DataflowConvProgram p{};
				assert(DataflowConvProgramBuilder::Build(fixture->manifest,fixture->workspace,old.layer,edge.position,fixture->routes,p)
					&& DataflowConvProgramBuilder::BindCore(p,core,target)
					&& DataflowStreamEncode(target,99,step+1,expected));
			} else if (which==2) {
				DataflowDeltaInvocation old{},target{};DataflowDynamicRead(original,old);DataflowDeltaProgram p{};
				assert(DataflowDeltaProgramBuilder::Build(fixture->manifest,fixture->workspace,old.program.layer,edge.position,fixture->routes,p)
					&& DataflowDeltaProgramBuilder::BindCore(p,core,target)
					&& DataflowStreamEncode(target,99,step+1,expected));
			} else {
				DataflowDenseAttentionInvocation old{},target{};DataflowDynamicRead(original,old);DataflowDenseAttentionProgram p{};
				assert(DataflowDenseAttentionProgramBuilder::Build(fixture->manifest,fixture->workspace,fixture->rope,
					old.program.layer,edge.position,old.program.mode,fixture->routes,p)
					&& DataflowDenseAttentionProgramBuilder::BindCore(p,core,target)
					&& DataflowStreamEncode(target,99,step+1,expected));
			}
			assert(DataflowDynamicPatch(patched,edge,99,step+1,core)
				&& !std::memcmp(&patched,&expected,sizeof(patched)));
		}
	}
	for (bool found:tested) assert(found);
	DataflowStreamRecord bad=base.records[0];bad.payload[3]^=1;
	assert(!DataflowDynamicPatch(bad,dynamic1,99,1,0));
	auto invalid=dynamic1;invalid.position=fixture->manifest.context;
	bad=base.records[0];assert(!DataflowDynamicPatch(bad,invalid,99,1,0));
	invalid=dynamic1;invalid.token=Vocabulary;
	assert(!DataflowDynamicPatch(bad,invalid,99,1,0));
	assert(!DataflowDynamicPatch(bad,dynamic1,100,1,0));
	puts("PASS dynamic token patch: every core and graph step matches fresh model-bound plans at positions 1 and 2; NO HARDWARE");
}
