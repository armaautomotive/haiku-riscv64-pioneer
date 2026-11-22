/* SPDX-License-Identifier: MIT */
#define Q35_COMPLETION_FIXTURE_ONLY
#include "pioneer_qwen35_graph_completion_test.cpp"
#include "../PioneerQwen35DataflowGraphModelTransaction.h"

struct ModelPublisherIO : PublisherIO {
	DataflowGraphModelTransaction& model;
	explicit ModelPublisherIO(DataflowGraphModelTransaction& m):model(m) {}
	bool GraphOwnershipMatches(const DataflowStreamPlan&,const DataflowDynamicTokenContext& t,
		const DataflowGraphRunTicket& ticket,unsigned slot) { return model.Matches(t,ticket,slot); }
};
struct ModelCompletionIO : CompletionIO {
	DataflowGraphModelTransaction& model;
	ModelCompletionIO(const PublisherIO& p,DataflowGraphModelTransaction& m):CompletionIO{p},model(m) {}
	bool GraphOwnershipMatches(const DataflowStreamPlan&,const DataflowDynamicTokenContext& t,
		const DataflowGraphRunTicket& ticket,unsigned slot) { return model.Matches(t,ticket,slot); }
};

int main() {
	(void)&q35_publisher_fixture_main;
	auto f=std::make_unique<Fixture>();auto plan=std::make_unique<DataflowStreamPlan>();
	assert(DataflowStreamPlanBuilder::Build(f->packed,f->manifest,f->workspace,f->rope,f->routes,99,548,*plan));
	auto liveOwner=std::make_unique<DataflowProjectionAuthority>();
	assert(liveOwner->BindValidatedModel(f->manifest,f->workspace,f->routes,f->rope));
	DataflowProjectionAuthority* live=liveOwner.get();
	auto scratch=std::make_unique<DataflowProjectionProgram>();unsigned stage=0;
	DataflowGraphRunIssuer issuer;
	for (unsigned position=0;position<2;position++) {
		auto after=std::make_unique<DataflowProjectionAuthority>();
		DataflowDynamicTokenContext token{};
		assert(DataflowDynamicTokenContext::Build(f->manifest,42+position,position,token));
		DataflowGraphRunTicket ticket{};assert(issuer.Issue(position?548:545,ticket));
		DataflowGraphPublisher publisher;DataflowGraphModelTransaction model;
		assert(model.Prepare(live,*after,publisher,ticket,token,0,*scratch,stage));
		assert(model.Matches(token,ticket,0) && live==liveOwner.get());
		auto changed=token;changed.token++;
		assert(!model.Matches(changed,ticket,0) && !model.Matches(token,ticket,1));
		auto badTicket=ticket;badTicket.epoch++;
		assert(!model.Matches(token,badTicket,0));
		auto io=std::make_unique<ModelPublisherIO>(model);DataflowGraphRunDescriptor descriptor{};
		assert(publisher.Start(*plan,f->routes,token,ticket,5,3,0,*io,descriptor)==DataflowGraphPublished);
		DataflowGraphCompletion completion;DataflowArgmaxResult result{};
		assert(!model.Commit(publisher,completion) && live==liveOwner.get() && !model.Retired());
		ModelCompletionIO done(*io,model);
		assert(completion.Finish(publisher,*plan,f->routes,token,done,descriptor,result)==DataflowGraphCompletionAudited);
		DataflowGraphPublisher other;
		assert(!model.Commit(other,completion) && live==liveOwner.get());
		assert(model.Commit(publisher,completion) && live==after.get() && model.Retired()==liveOwner.get());
		assert(!model.Commit(publisher,completion) && live->ConvNextPosition(0)==position+1);
		liveOwner.swap(after); // The audited session now owns the new live object.
	}
	DataflowGraphPublisher stalePublisher;DataflowGraphRunTicket ticket{};assert(issuer.Issue(548,ticket));
	DataflowDynamicTokenContext token{};assert(DataflowDynamicTokenContext::Build(f->manifest,50,2,token));
	auto forged=token;forged.tied.parts[0].address+=16;
	auto forgedCandidate=std::make_unique<DataflowProjectionAuthority>();DataflowGraphModelTransaction forgedModel;
	assert(!forgedModel.Prepare(live,*forgedCandidate,stalePublisher,ticket,forged,0,*scratch,stage)
		&& !forgedCandidate->Bound() && live==liveOwner.get());
	auto after=std::make_unique<DataflowProjectionAuthority>();
	DataflowGraphModelTransaction stale;
	assert(stale.Prepare(live,*after,stalePublisher,ticket,token,0,*scratch,stage));
	assert(after->ActivationReady(0) && after->InvalidateActivation(0));
	assert(!stale.Matches(token,ticket,0) && live==liveOwner.get());
	// A separate transaction verifies drift in the original authority too.
	auto otherCandidate=std::make_unique<DataflowProjectionAuthority>();
	DataflowGraphModelTransaction changedLive;
	assert(changedLive.Prepare(live,*otherCandidate,stalePublisher,ticket,token,0,*scratch,stage));
	assert(live->ActivationReady(0) && live->InvalidateActivation(0) && !live->Pending());
	assert(!changedLive.Matches(token,ticket,0) && live==liveOwner.get());
	DataflowGraphCompletion noAudit;
	assert(!changedLive.Commit(stalePublisher,noAudit) && !changedLive.Retired());
	puts("PASS graph model transaction: audited one-time state promotion, input binding and stale-state rejection; SIMULATED ONLY");
	return 0;
}
