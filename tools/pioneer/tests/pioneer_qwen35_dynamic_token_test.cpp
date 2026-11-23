/* SPDX-License-Identifier: MIT */
#define main q35_dynamic_fixture_main
#include "pioneer_qwen35_dataflow_stream_recorder_test.cpp"
#undef main
#include "../PioneerQwen35DataflowDynamicToken.h"
#include "../PioneerQwen35DataflowGraphAuthorization.h"
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
	bool Seal(const DataflowStreamPlan& p) {
		return current==size_t(p.count)*DataflowCoreCount && current<=captured.records.size();
	}
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
	// Check the local post-fetch guard against every invocation family from
	// the real recorded graph, including an inactive worker where applicable.
	const unsigned magicTypes[]={DataflowEmbeddingMagic,DataflowProgramMagic,DataflowRmsMagic,
		DataflowSiluMagic,DataflowResidualMagic,DataflowConvMagic,DataflowDeltaMagic,
		DataflowDenseAttentionMagic,DataflowArgmaxMagic};
	bool localTested[9]{};DataflowGraphRunTicket localTicket{7,1001,548};
	alignas(64) uint32_t localProgram[DataflowProgramRegionBytes/4]{};
	for (unsigned step=0;step<548;step++) {
		const unsigned magic=base.records[size_t(step)*DataflowCoreCount].header[5];
		unsigned type=0;while (type<9 && magicTypes[type]!=magic) type++;
		assert(type<9);if (localTested[type]) continue;localTested[type]=true;
		for (unsigned core:{0U,119U}) {
			DataflowGraphRunDescriptor descriptor{};
			assert(DataflowGraphRunBuild(plan->binding[core],plan->lease[core],fixture->routes,
				dynamic1,localTicket,5,3,0,descriptor));
			const auto& original=base.records[size_t(step)*DataflowCoreCount+core];
			auto record=original;
			assert(DataflowDynamicPatch(record,dynamic1,99,step+1,core));
			FetchIO fastRead{original};DataflowStreamRecord fetched{};
			unsigned fetchStage=99;
			assert(DataflowStreamFetchDynamic(fastRead,plan->binding[core],plan->lease[core],
				fixture->routes,core,step,fetched,localProgram,sizeof(localProgram),dynamic1,&fetchStage)
				&& fetchStage==DataflowStreamFetchOkay);
			assert(!std::memcmp(&fetched,&record,sizeof(record))
				&& !std::memcmp(localProgram,record.payload,record.header[6]));
			assert(DataflowGraphFetchedCommandMatches(descriptor,step,record,localProgram,sizeof(localProgram)));
			localProgram[0]=0;
			assert(!DataflowGraphFetchedCommandMatches(descriptor,step,record,localProgram,sizeof(localProgram)));
		}
	}
	for (bool checked:localTested) assert(checked);
	// The prompt-only program must be byte-identical to the first 545
	// operations of the decode stream, including after a dynamic token patch.
	auto promptPlan=std::make_unique<DataflowStreamPlan>();
	assert(DataflowStreamPlanBuilder::Build(fixture->packed,fixture->manifest,fixture->workspace,
		fixture->rope,fixture->routes,99,545,*promptPlan));
	DataflowDynamicTokenContext dynamic0{};
	assert(DataflowDynamicTokenContext::Build(fixture->manifest,42,0,dynamic0));
	auto prompt0=std::make_unique<DataflowProjectionAuthority>();CompareSink comparePrompt0{base,dynamic0};
	assert(DataflowStreamRecording::Build(fixture->packed,*live,*prompt0,*promptPlan,fixture->routes,
		0,42,0,false,comparePrompt0,*scratch,*projection,stage));
	auto prompt1=std::make_unique<DataflowProjectionAuthority>();CompareSink comparePrompt1{base,dynamic1};
	assert(DataflowStreamRecording::Build(fixture->packed,*candidate0,*prompt1,*promptPlan,fixture->routes,
		1,otherToken,0,false,comparePrompt1,*scratch,*projection,stage));
	assert(comparePrompt0.current==545*DataflowCoreCount && comparePrompt1.current==545*DataflowCoreCount);
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
	DataflowStreamRecord corrupted=base.records[0];corrupted.payload[DataflowStreamPayloadWords-1]=1;
	FetchIO corruptRead{corrupted};unsigned corruptStage=99;
	program[0]=0xfeedface;program[1]=0xfacefeed;
	assert(!DataflowStreamFetchDynamic(corruptRead,plan->binding[0],plan->lease[0],fixture->routes,
		0,0,staging,program,sizeof(program),dynamic1,&corruptStage)
		&& corruptStage==DataflowStreamFetchRecord && program[0]==0 && program[1]==0xfacefeed);
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
