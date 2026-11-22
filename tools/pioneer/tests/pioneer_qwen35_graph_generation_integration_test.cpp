/* SPDX-License-Identifier: MIT */
#define Q35_GRAPH_SESSION_FIXTURE_ONLY
#include "pioneer_qwen35_graph_session_test.cpp"
#include "../PioneerQwen35DataflowGraphOwnedRunner.h"
#include "../PioneerQwen35DataflowGraphGeneration.h"
#include "../PioneerQwen35DataflowGraphFaultTeardown.h"
#include "../PioneerQwen35DataflowRecoveryRetry.h"

struct Timer {
	uint64_t now=0;
	uint64_t NowMicros() { const auto value=now;now+=10;return value; }
};
struct IntegratedOwner {
	DataflowGraphSession& session;DataflowProjectionAuthority*& live;
	DataflowServiceOwner& service;DataflowProjectionProgram& scratch;StreamClaim& claim;
	DataflowGraphOwnedRunner runner;unsigned calls=0,failPosition=~0U;
	IntegratedOwner(DataflowGraphSession& s,DataflowProjectionAuthority*& l,DataflowServiceOwner& serviceOwner,
		DataflowProjectionProgram& p,StreamClaim& c):session(s),live(l),service(serviceOwner),scratch(p),claim(c) {}
	DataflowGraphSessionResult Run(unsigned position,unsigned token,bool decode,unsigned polls,
		DataflowArgmaxResult& output,unsigned& stage) {
		SessionIO io(claim);io.previous=service.NextSequence();
		if (position==failPosition) io.failAudit=7;
		calls++;
		return runner.Run(session,live,position,token,decode,polls,scratch,io,output,stage);
	}
};

struct IntegratedRecoveryIO : GraphRecoveryIO {
	DataflowServiceOwner& service;MockDataflowIO& serviceIO;SessionIO& allocator;
	IntegratedRecoveryIO(StreamClaim& claim,DataflowServiceOwner& s,MockDataflowIO& io,SessionIO& memory)
		:GraphRecoveryIO{claim},service(s),serviceIO(io),allocator(memory) {}
	DataflowServiceResult CloseFaultedGraphService() { return service.Close(serviceIO); }
	bool GraphFleetRestoredAfterClose(const DataflowGraphSession& session,const DataflowGraphFaultAudit& audit) {
		const auto* run=session.RetainedRun();
		return service.Closed() && run && audit.SettledFor(run->publisher);
	}
	void DeleteGraphRun(DataflowGraphSessionRun* p) { allocator.DeleteGraphRun(p); }
	void DeleteGraphCandidate(DataflowProjectionAuthority* p) { allocator.DeleteGraphCandidate(p); }
};

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
	auto projection=std::make_unique<DataflowProjectionProgram>();
	IntegratedOwner owner{*session,live,*service,*projection,claim};
	const unsigned prompt[]={11,12};unsigned output[4]{},count=0,stage=0;
	uint64_t promptUs=0,decodeUs=0;Timer clock;
	assert(DataflowGraphGeneration::Run(prompt,2,3,99,32,3,owner,output,count,stage,clock,promptUs,decodeUs)
		==DataflowGenerationSequenceComplete && count==3 && output[0]==42 && output[1]==42 && output[2]==42
		&& promptUs==10 && decodeUs==10 && owner.calls==4 && service->NextSequence()==4
		&& live->ConvNextPosition(0)==4);
	Timer warmClock;
	assert(DataflowGraphGeneration::Run(prompt,2,3,42,32,3,owner,output,count,stage,warmClock,promptUs,decodeUs)
		==DataflowGenerationSequenceComplete && count==1 && output[0]==42 && !decodeUs
		&& owner.calls==6 && service->NextSequence()==6 && live->ConvNextPosition(0)==2);
	owner.failPosition=2;Timer failClock;
	assert(DataflowGraphGeneration::Run(prompt,2,3,99,32,3,owner,output,count,stage,failClock,promptUs,decodeUs)
		==DataflowGenerationSequenceDecoderFailed && count==1 && output[0]==42
		&& owner.runner.Retained() && service->Faulted()
		&& stage==(2U<<16|DataflowGraphFailureStage(DataflowGraphStageCompletion,DataflowGraphCompletionAuditFault,7)));
	SessionIO cleanup(claim);IntegratedRecoveryIO recovery(claim,*service,*serviceIO,cleanup);
	recovery.previous=8;auto audit=std::make_unique<DataflowGraphFaultAudit>();
	recovery.badCore=119;
	serviceIO->failResume=true;
	struct Retry {
		DataflowGraphSession& session;DataflowGraphOwnedRunner& runner;
		DataflowGraphFaultAudit& audit;DataflowProjectionAuthority* live;
		IntegratedRecoveryIO& io;unsigned attempts=0,delays=0;
		DataflowRecoveryAttemptResult Attempt(unsigned n) {
			assert(n==attempts++);
			if (n) assert(audit.ResetUnsettled());
			const auto result=DataflowGraphFaultTeardown::Run(session,runner,audit,live,3,io);
			if (result==DataflowServiceRecoveryPending) return DataflowRecoveryAttemptPending;
			return result==DataflowServiceClosed?DataflowRecoveryAttemptComplete:DataflowRecoveryAttemptStop;
		}
		bool Delay() { delays++;io.badCore=-1;io.audits=0;return true; }
	} retry{*session,owner.runner,*audit,live,recovery};
	assert(!DataflowRecoveryRetry(retry,4) && retry.attempts==2 && retry.delays==1
		&& owner.runner.Retained() && session->FaultClosePermitted(*audit)
		&& !cleanup.runDeletes && recovery.acks==120);
	const unsigned auditsAfterClose=recovery.audits,stopsAfterClose=serviceIO->stopCount;
	serviceIO->failResume=false;
	assert(DataflowGraphFaultTeardown::Run(*session,owner.runner,*audit,live,3,recovery)==DataflowServiceClosed
		&& service->Closed() && !owner.runner.Retained() && cleanup.runDeletes==1
		&& cleanup.candidateDeletes==1 && recovery.acks==120 && session->Faulted()
		&& recovery.audits==auditsAfterClose && serviceIO->stopCount==stopsAfterClose);
	delete live;
	puts("PASS graph generation integration: warm requests, prompt/decode timing, retained fault; SIMULATED ONLY");
	return 0;
}
