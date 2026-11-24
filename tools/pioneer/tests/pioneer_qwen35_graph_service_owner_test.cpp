/* SPDX-License-Identifier: MIT */
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define main q35_service_fixture_main
#include "pioneer_qwen35_dataflow_service_owner_test.cpp"
#undef main
#define Q35_COMPLETION_FIXTURE_ONLY
#include "pioneer_qwen35_graph_completion_test.cpp"
#include "../PioneerQwen35DataflowGraphModelTransaction.h"
#include <memory>

struct ModelCommit {
	bool allow=true;unsigned calls=0;
	DataflowGraphModelTransaction* transaction=nullptr;
	bool CommitGraphModelState(const DataflowGraphPublisher& p,const DataflowGraphCompletion& c) {
		calls++;return allow && c.AuditedFor(p) && (!transaction || transaction->Commit(p,c));
	}
};

struct CloseAuditIO {
	unsigned previous;
	int failedCore=-1;
	bool acknowledged=false;
	bool FenceGraphHostWrites() { return true; }
	void Pause() {}
	bool ReadWorkerSnapshot(unsigned core,DataflowWorkerSnapshot& s) {
		if (int(core)==failedCore) {
			const unsigned trigger=previous+1;
			s=acknowledged?DataflowWorkerSnapshot{trigger,trigger,trigger,DataflowWorkerReady,0,0,0,0}
				:DataflowWorkerSnapshot{trigger,previous,trigger,DataflowWorkerFailed,1,5,0,0};
			return true;
		}
		s={previous,previous,previous,DataflowWorkerReady,0,0,0,0};return true;
	}
	bool AuditGraphRecoveryCore(unsigned,const DataflowWorkerSnapshot&) { return true; }
	bool Acknowledge(unsigned core,unsigned sequence) {
		if (int(core)!=failedCore || sequence!=previous+1 || acknowledged) return false;
		acknowledged=true;return true;
	}
};

#if defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) && defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS)
struct PartialCloseIO : MockDataflowIO {
	DataflowServiceOwner* owner=nullptr;
	bool sawFaultStop=false,sawCleanStop=false;
	bool SharedStopAndHoldAll(unsigned& stage,unsigned& core) {
		sawFaultStop=owner && owner->SharedSnapshotFaultStopPending();
		sawCleanStop=owner && owner->SharedSnapshotCleanStopPending();
		return MockDataflowIO::SharedStopAndHoldAll(stage,core);
	}
};
#else
using PartialCloseIO=MockDataflowIO;
#endif

int main() {
	(void)&q35_service_fixture_main;(void)&q35_publisher_fixture_main;
	uint32_t code[96]{},dispatch[80]{};
	for (unsigned i=0;i<96;i++) code[i]=0x10000000+i;
	for (unsigned i=0;i<80;i++) dispatch[i]=0x20000000+i;
	DataflowServiceImageView image={DataflowServiceBootWord,code,96,dispatch,80};
	auto io=std::make_unique<MockDataflowIO>();auto service=std::make_unique<DataflowServiceOwner>();
	assert(service->Install(image,*io)==DataflowServiceComplete && service->NextSequence()==0);
	auto f=std::make_unique<Fixture>();auto plan=std::make_unique<DataflowStreamPlan>();
	assert(DataflowStreamPlanBuilder::Build(f->packed,f->manifest,f->workspace,f->rope,f->routes,99,548,*plan));
	DataflowDynamicTokenContext token{};assert(DataflowDynamicTokenContext::Build(f->manifest,42,0,token));
	DataflowGraphRunIssuer issuer;DataflowGraphRunDescriptor scratch{};ModelCommit model;
	auto liveOwner=std::make_unique<DataflowProjectionAuthority>();
	assert(liveOwner->BindValidatedModel(f->manifest,f->workspace,f->routes,f->rope));
	DataflowProjectionAuthority* live=liveOwner.get();
	auto projection=std::make_unique<DataflowProjectionProgram>();unsigned stage=0;
	unsigned trigger=0;
	DataflowGraphPublisher neverStarted;
	assert(service->BeginGraph(neverStarted,trigger) && trigger==1 && !service->Ready());
	DataflowGraphPublisher differentPublisher;
	assert(!service->CancelGraphBeforePublication(differentPublisher));
	assert(service->Close(*io)==DataflowServiceInvalidState);
	assert(service->CancelGraphBeforePublication(neverStarted) && service->Ready());
	for (unsigned run=0;run<2;run++) {
		DataflowGraphPublisher publisher;
		assert(service->BeginGraph(publisher,trigger) && trigger==run+1);
		DataflowGraphRunTicket ticket{};assert(issuer.Issue(run?548:545,ticket));
		assert(DataflowDynamicTokenContext::Build(f->manifest,42+run,run,token));
		auto candidate=std::make_unique<DataflowProjectionAuthority>();
		DataflowGraphModelTransaction transaction;
		assert(transaction.Prepare(live,*candidate,publisher,ticket,token,0,*projection,stage));
		model.transaction=&transaction;
		auto pubIO=std::make_unique<PublisherIO>();pubIO->previous=run;
		assert(publisher.Start(*plan,f->routes,token,ticket,trigger,3,0,*pubIO,scratch)==DataflowGraphPublished);
		assert(!service->CancelGraphBeforePublication(publisher));
		CompletionIO completionIO{*pubIO};DataflowGraphCompletion completion;DataflowArgmaxResult output{};
		assert(completion.Finish(publisher,*plan,f->routes,token,completionIO,scratch,output)==DataflowGraphCompletionAudited);
		assert(service->CommitGraph(publisher,completion,model) && service->Ready()
			&& service->NextSequence()==trigger && model.calls==run+1 && live==candidate.get()
			&& transaction.Retired()==liveOwner.get() && live->ConvNextPosition(0)==run+1);
		assert(!service->CommitGraph(publisher,completion,model) && model.calls==run+1);
		liveOwner.swap(candidate);model.transaction=nullptr;
	}
	DataflowGraphPublisher publisher;
	assert(service->BeginGraph(publisher,trigger) && trigger==3);
	DataflowGraphRunTicket ticket{};assert(issuer.Issue(548,ticket));
	auto pubIO=std::make_unique<PublisherIO>();pubIO->previous=2;
	assert(publisher.Start(*plan,f->routes,token,ticket,trigger,3,0,*pubIO,scratch)==DataflowGraphPublished);
	CompletionIO completionIO{*pubIO};DataflowGraphCompletion completion;DataflowArgmaxResult output{};
	assert(completion.Finish(publisher,*plan,f->routes,token,completionIO,scratch,output)==DataflowGraphCompletionAudited);
	model.allow=false;
	assert(!service->CommitGraph(publisher,completion,model) && service->Faulted()
		&& service->NextSequence()==2 && model.calls==3);
	DataflowGraphPublisher refused;
	assert(!service->BeginGraph(refused,trigger) && service->Close(*io)==DataflowServiceInvalidState);
	auto badIO=std::make_unique<MockDataflowIO>();auto badService=std::make_unique<DataflowServiceOwner>();
	DataflowGraphPublisher unpublished;DataflowGraphCompletion unaudited;ModelCommit mustNotRun;
	assert(badService->Install(image,*badIO)==DataflowServiceComplete && badService->BeginGraph(unpublished,trigger));
	assert(!badService->CommitGraph(unpublished,unaudited,mustNotRun)
		&& badService->Faulted() && !mustNotRun.calls && badService->NextSequence()==0);
	assert(badService->Close(*badIO)==DataflowServiceInvalidState);
	auto partialIO=std::make_unique<PartialCloseIO>();auto partialService=std::make_unique<DataflowServiceOwner>();
#if defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) && defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS)
	partialIO->owner=partialService.get();
#endif
	DataflowGraphPublisher partialPublisher;
	assert(partialService->Install(image,*partialIO)==DataflowServiceComplete && partialService->BeginGraph(partialPublisher,trigger));
	DataflowGraphRunTicket partialTicket{};assert(issuer.Issue(545,partialTicket));
	auto partialPubIO=std::make_unique<PublisherIO>();partialPubIO->previous=0;partialPubIO->failStage=3;
	assert(partialPublisher.Start(*plan,f->routes,token,partialTicket,trigger,3,0,*partialPubIO,scratch)
		==DataflowGraphPublishStagingFault && !partialService->CancelGraphBeforePublication(partialPublisher));
	partialService->RetainFaultedGraph();
	assert(partialService->OwnsRetainedFaultGraph(partialPublisher)
		&& !partialService->OwnsRetainedFaultGraph(publisher));
	assert(partialService->Faulted() && partialService->Close(*partialIO)==DataflowServiceInvalidState);
	DataflowGraphFaultAudit closeAudit,emptyAudit;
	assert(!partialService->PermitFaultedGraphClose(partialPublisher,emptyAudit));
	CloseAuditIO closeAuditIO{0};
	assert(closeAudit.Settle(partialPublisher,3,closeAuditIO)==DataflowGraphFaultAuditSettled);
	assert(!partialService->PermitFaultedGraphClose(publisher,closeAudit));
	assert(partialService->PermitFaultedGraphClose(partialPublisher,closeAudit));
	assert(partialService->OwnsRetainedFaultGraph(partialPublisher));
	assert(partialService->Faulted() && !partialService->Ready()
		&& !partialService->BeginGraph(refused,trigger));
	partialIO->failStopAudit=true;
	assert(partialService->Close(*partialIO)==DataflowServiceStopAuditFailed
		&& partialService->OwnsRetainedFaultGraph(partialPublisher));
#if defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) && defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS)
	assert(partialIO->sawFaultStop && !partialIO->sawCleanStop
		&& partialService->SharedSnapshotFaultStopPending());
#endif
	partialIO->failStopAudit=false;
	assert(partialService->Close(*partialIO)==DataflowServiceClosed && partialService->Closed());
	#if defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) && defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS)
	assert(partialIO->sawFaultStop && !partialIO->sawCleanStop
		&& !partialService->SharedSnapshotFaultStopPending());
	#endif
	assert(!partialService->OwnsRetainedFaultGraph(partialPublisher));
#if defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) && defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS)
	// Exercise a published graph with a failed worker, not just a failure
	// while staging: audit and acknowledge the terminal worker before close.
	auto faultIO=std::make_unique<PartialCloseIO>();
	auto faultService=std::make_unique<DataflowServiceOwner>();faultIO->owner=faultService.get();
	DataflowGraphPublisher faultPublisher;
	assert(faultService->Install(image,*faultIO)==DataflowServiceComplete
		&& faultService->BeginGraph(faultPublisher,trigger));
	DataflowGraphRunTicket faultTicket{};assert(issuer.Issue(545,faultTicket));
	auto faultPubIO=std::make_unique<PublisherIO>();faultPubIO->previous=0;
	assert(faultPublisher.Start(*plan,f->routes,token,faultTicket,trigger,3,0,*faultPubIO,scratch)
		==DataflowGraphPublished);
	faultService->RetainFaultedGraph();
	DataflowGraphFaultAudit faultAudit;
	CloseAuditIO faultAuditIO{0};faultAuditIO.failedCore=3;
	assert(faultAudit.Settle(faultPublisher,3,faultAuditIO)==DataflowGraphFaultAuditSettled
		&& faultAuditIO.acknowledged && faultService->PermitFaultedGraphClose(faultPublisher,faultAudit));
	assert(faultService->Close(*faultIO)==DataflowServiceClosed && faultIO->sawFaultStop
		&& !faultIO->sawCleanStop && !faultService->SharedSnapshotFaultStopPending());
#endif
	puts("PASS graph service lifecycle: exclusive execution, audited commit, sequence adoption and fault retention; SIMULATED ONLY");
	return 0;
}
