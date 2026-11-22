/* SPDX-License-Identifier: MIT */
#define main q35_recording_fixture_main
#include "pioneer_qwen35_dataflow_stream_recorder_test.cpp"
#undef main
#include "../PioneerQwen35DataflowGraphMetadata.h"

static void Compare(const DataflowProjectionAuthority& a,const DataflowProjectionAuthority& b) {
	assert(a.Bound() && b.Bound() && !a.Pending() && !b.Pending());
	for (unsigned i=0;i<ActivationBuffers;i++) assert(a.ActivationRows(i)==b.ActivationRows(i));
	for (unsigned i=0;i<DataflowGraphWorkspaceBufferCount;i++) {
		assert(a.GraphWorkspaceReady(i)==b.GraphWorkspaceReady(i));
		assert(a.GraphWorkspaceLayer(i)==b.GraphWorkspaceLayer(i));
		assert(a.GraphWorkspaceRole(i)==b.GraphWorkspaceRole(i));
	}
	for (unsigned i=0;i<Layers;i++) {
		assert(a.ConvNextPosition(i)==b.ConvNextPosition(i));
		assert(a.DeltaNextPosition(i)==b.DeltaNextPosition(i));
	}
	assert(a.GraphResidualReady()==b.GraphResidualReady());
	assert(a.VocabularyReady()==b.VocabularyReady());
}

int main() {
	(void)&q35_recording_fixture_main;
	auto f=std::make_unique<Fixture>();auto live=std::make_unique<DataflowProjectionAuthority>();
	assert(live->BindValidatedModel(f->manifest,f->workspace,f->routes,f->rope));
	auto plan=std::make_unique<DataflowStreamPlan>();auto record=std::make_unique<DataflowStreamRecord>();
	auto scratch=std::make_unique<DataflowProjectionProgram>();unsigned stage=0;
	for (unsigned decode=0;decode<2;decode++) {
		auto recorded=std::make_unique<DataflowProjectionAuthority>();
		auto predicted=std::make_unique<DataflowProjectionAuthority>();Sink sink;
		const unsigned count=decode?DataflowGraphDecodeSteps:DataflowGraphPromptSteps;
		assert(DataflowStreamPlanBuilder::Build(f->packed,f->manifest,f->workspace,f->rope,f->routes,90+decode,count,*plan));
		assert(DataflowStreamRecording::Build(f->packed,*live,*recorded,*plan,f->routes,0,42,0,decode,
			sink,*record,*scratch,stage));
		assert(DataflowGraphMetadata::Prepare(*live,*predicted,0,42,0,decode,*scratch,stage));
		Compare(*recorded,*predicted);
		assert(!live->ActivationReady(0) && live->ConvNextPosition(0)==0);
		auto nextRecorded=std::make_unique<DataflowProjectionAuthority>();
		auto nextPredicted=std::make_unique<DataflowProjectionAuthority>();Sink nextSink;
		assert(DataflowStreamRecording::Build(f->packed,*recorded,*nextRecorded,*plan,f->routes,1,43,0,decode,
			nextSink,*record,*scratch,stage));
		assert(DataflowGraphMetadata::Prepare(*predicted,*nextPredicted,1,43,0,decode,*scratch,stage));
		Compare(*nextRecorded,*nextPredicted);
		assert(predicted->ConvNextPosition(0)==1 && nextPredicted->ConvNextPosition(0)==2);
	}
	auto invalid=std::make_unique<DataflowProjectionAuthority>();
	assert(!DataflowGraphMetadata::Prepare(*live,*invalid,f->manifest.context,42,0,true,*scratch,stage));
	assert(!invalid->Bound() && !live->ActivationReady(0));
	auto wrongPosition=std::make_unique<DataflowProjectionAuthority>();
	assert(!DataflowGraphMetadata::Prepare(*live,*wrongPosition,1,42,0,true,*scratch,stage));
	assert(!live->ActivationReady(0) && live->ConvNextPosition(0)==0);
	puts("PASS graph metadata replay matches recorded prompt and decode states; live owner unchanged; SIMULATED ONLY");
	return 0;
}
