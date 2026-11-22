/* SPDX-License-Identifier: MIT */
#define Q35_GRAPH_SESSION_FIXTURE_ONLY
#include "pioneer_qwen35_graph_session_test.cpp"
#include "../PioneerQwen35DataflowGraphOwnedRunner.h"

int main() {
	(void)&q35_session_service_fixture_main;(void)&q35_publisher_fixture_main;
	uint32_t code[96]{},dispatch[80]{};
	for (unsigned i=0;i<96;i++) code[i]=0x10000000+i;
	for (unsigned i=0;i<80;i++) dispatch[i]=0x20000000+i;
	DataflowServiceImageView image={DataflowServiceBootWord,code,96,dispatch,80};
	auto serviceIO=std::make_unique<MockDataflowIO>();auto service=std::make_unique<DataflowServiceOwner>();
	assert(service->Install(image,*serviceIO)==DataflowServiceComplete);
	auto f=std::make_unique<Fixture>();auto plan=std::make_unique<DataflowStreamPlan>();
	assert(DataflowStreamPlanBuilder::Build(f->packed,f->manifest,f->workspace,f->rope,f->routes,99,548,*plan));
	auto* live=new DataflowProjectionAuthority;
	assert(live->BindValidatedModel(f->manifest,f->workspace,f->routes,f->rope));
	StreamClaim claim;SessionIO bindIO(claim);auto session=std::make_unique<DataflowGraphSession>();
	assert(session->Bind(*plan,f->routes,0,live,*service,bindIO));
	auto projection=std::make_unique<DataflowProjectionProgram>();DataflowGraphOwnedRunner runner;
	DataflowArgmaxResult output{};unsigned stage=0;
	SessionIO noRun(claim);noRun.failRunAlloc=true;
	assert(runner.Run(*session,live,0,42,false,3,*projection,noRun,output,stage)==DataflowGraphSessionPreflight
		&& !noRun.candidateAllocations && service->Ready());
	SessionIO noCandidate(claim);noCandidate.failCandidateAlloc=true;
	assert(runner.Run(*session,live,0,42,false,3,*projection,noCandidate,output,stage)==DataflowGraphSessionPreflight
		&& noCandidate.runAllocations==1 && noCandidate.runDeletes==1 && service->Ready());
	SessionIO alias(claim);alias.aliasCandidate=live;
	assert(runner.Run(*session,live,0,42,false,3,*projection,alias,output,stage)==DataflowGraphSessionInvalid
		&& alias.runDeletes==1 && !alias.candidateDeletes && live->Bound());
	for (unsigned position=0;position<2;position++) {
		SessionIO io(claim);io.previous=position;
		assert(runner.Run(*session,live,position,42+position,position!=0,3,*projection,io,output,stage)
			==DataflowGraphSessionComplete && live->ConvNextPosition(0)==position+1
			&& io.runAllocations==1 && io.runDeletes==1 && io.candidateAllocations==1
			&& io.candidateDeletes==1 && !runner.Retained());
	}
	SessionIO failed(claim);failed.previous=2;failed.failStage=7;
	assert(runner.Run(*session,live,2,44,true,3,*projection,failed,output,stage)==DataflowGraphSessionRetained
		&& runner.Retained() && !failed.runDeletes && !failed.candidateDeletes
		&& stage==DataflowGraphFailureStage(DataflowGraphStagePublish,DataflowGraphPublishStagingFault,7)
		&& output.token==Vocabulary && live->ConvNextPosition(0)==2);
	assert(!runner.ReleaseRetainedAfterRestoration(live,failed) && runner.Retained());
	failed.restored=true; // Simulated restored-fleet proof only.
	assert(runner.ReleaseRetainedAfterRestoration(live,failed) && !runner.Retained()
		&& failed.runDeletes==1 && failed.candidateDeletes==1);
	delete live;
	// A partially published fleet must retain ownership and identify the
	// failed command and worker, rather than exposing a generic session code.
	auto serviceIO2=std::make_unique<MockDataflowIO>();auto service2=std::make_unique<DataflowServiceOwner>();
	assert(service2->Install(image,*serviceIO2)==DataflowServiceComplete);
	auto* live2=new DataflowProjectionAuthority;
	assert(live2->BindValidatedModel(f->manifest,f->workspace,f->routes,f->rope));
	StreamClaim claim2;SessionIO bindIO2(claim2);auto session2=std::make_unique<DataflowGraphSession>();
	assert(session2->Bind(*plan,f->routes,0,live2,*service2,bindIO2));
	DataflowGraphOwnedRunner runner2;SessionIO partial(claim2);partial.previous=0;partial.failPublish=49;
	assert(runner2.Run(*session2,live2,0,42,false,3,*projection,partial,output,stage)==DataflowGraphSessionRetained
		&& stage==DataflowGraphFailureStage(DataflowGraphStagePublish,DataflowGraphPublishCommandFault,49)
		&& partial.published==49 && runner2.Retained() && output.token==Vocabulary);
	partial.restored=true;
	assert(runner2.ReleaseRetainedAfterRestoration(live2,partial));
	delete live2;
	puts("PASS graph owned runner: allocation failures, promotion, retained fault and proof-gated release; SIMULATED ONLY");
	return 0;
}
