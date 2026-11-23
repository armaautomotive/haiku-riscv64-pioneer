/* SPDX-License-Identifier: MIT */
#define Q35_PUBLISHER_FIXTURE_ONLY
#include "pioneer_qwen35_graph_publication_test.cpp"
#include "../PioneerQwen35DataflowStageLedger.h"

struct LedgerPublisherIO : PublisherIO {
	DataflowGraphStageLedger& ledger;
	bool requireAudit=false;
	explicit LedgerPublisherIO(DataflowGraphStageLedger& owner):ledger(owner) { previous=0; }
	bool StageGraphRun(unsigned core,const DataflowGraphRunDescriptor& d) {
		if (!ledger.CanStage(core,d)) return false;
		const auto* old=d.triggerSequence>1?ledger.TakeAudited(core,d.triggerSequence-1):nullptr;
		if (requireAudit && (!old || old->triggerSequence+1!=d.triggerSequence)) return false;
		return PublisherIO::StageGraphRun(core,d) && ledger.Record(core,d);
	}
	bool GraphRunMatches(unsigned core,const DataflowGraphRunDescriptor& d) {
		if (core!=checked || !ledger.Verify(core,d)) return false;
		checked++;return true;
	}
	bool PublishGraphCommand(unsigned core,unsigned sequence) {
		const auto* d=ledger.BeginPublish(core,sequence);
		return d && !std::memcmp(d,&records[core],sizeof(*d))
			&& PublisherIO::PublishGraphCommand(core,sequence);
	}
};

int main() {
	(void)&q35_publisher_fixture_main;
	auto f=std::make_unique<Fixture>();auto plan=std::make_unique<DataflowStreamPlan>();
	assert(DataflowStreamPlanBuilder::Build(f->packed,f->manifest,f->workspace,f->rope,f->routes,99,548,*plan));
	DataflowDynamicTokenContext token{};assert(DataflowDynamicTokenContext::Build(f->manifest,42,0,token));
	auto ledger=std::make_unique<DataflowGraphStageLedger>();
	DataflowGraphRunTicket ticket{1,1,548};DataflowGraphRunDescriptor descriptor{};
	assert(DataflowGraphRunBuild(plan->binding[0],plan->lease[0],f->routes,token,ticket,1,100,0,descriptor));
	assert(!ledger->BeginPublish(0,1) && !ledger->CanStage(1,descriptor));
	assert(ledger->CanStage(0,descriptor) && ledger->Record(0,descriptor));
	assert(!ledger->Record(0,descriptor) && !ledger->BeginPublish(0,1));
	auto forged=descriptor;forged.token.token++;
	assert(!ledger->Verify(0,forged) && ledger->Verify(0,descriptor));
	assert(!ledger->Verify(0,descriptor));
	assert(DataflowGraphRunBuild(plan->binding[1],plan->lease[1],f->routes,token,ticket,1,100,0,descriptor));
	forged=descriptor;forged.ticket.epoch++;forged.checksum=DataflowGraphRunChecksum(forged);
	assert(DataflowGraphRunValid(forged,1) && !ledger->CanStage(1,forged));
	for (unsigned core=1;core<DataflowCoreCount;core++) {
		assert(DataflowGraphRunBuild(plan->binding[core],plan->lease[core],f->routes,token,ticket,1,100,0,descriptor));
		assert(ledger->Record(core,descriptor));
		if (core==DataflowCoreCount-1) assert(!ledger->BeginPublish(0,1));
		assert(ledger->Verify(core,descriptor));
	}
	assert(!ledger->BeginPublish(1,1) && !ledger->BeginPublish(0,2));
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		const auto* owned=ledger->BeginPublish(core,1);
		assert(owned && owned->core==core && owned->triggerSequence==1 && DataflowGraphRunValid(*owned,core));
		if (core<DataflowCoreCount-1) assert(!ledger->RecordAudit(core,*owned));
		assert(!ledger->BeginPublish(core,1));
	}
	assert(!ledger->BeginPublish(0,1));
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		assert(DataflowGraphRunBuild(plan->binding[core],plan->lease[core],f->routes,token,ticket,1,100,0,descriptor));
		if (!core) {
			forged=descriptor;forged.checksum^=1;
			assert(!ledger->RecordAudit(core,forged));
		}
		assert(ledger->RecordAudit(core,descriptor) && !ledger->RecordAudit(core,descriptor));
	}
	assert(DataflowDynamicTokenContext::Build(f->manifest,43,1,token));ticket={2,549,545};
	assert(DataflowGraphRunBuild(plan->binding[0],plan->lease[0],f->routes,token,ticket,2,100,0,descriptor));
	assert(!ledger->TakeAudited(1,1) && !ledger->TakeAudited(0,2));
	const auto* consumed=ledger->TakeAudited(0,1);
	assert(consumed && !ledger->TakeAudited(0,1) && !ledger->RecordAudit(0,*consumed));
	assert(ledger->CanStage(0,descriptor) && ledger->Record(0,descriptor) && ledger->Verify(0,descriptor));
	assert(!ledger->BeginPublish(0,2)); // A partially staged fleet cannot launch.
	for (unsigned core=1;core<DataflowCoreCount;core++) {
		assert(DataflowGraphRunBuild(plan->binding[core],plan->lease[core],f->routes,token,ticket,2,100,0,descriptor));
		assert(ledger->TakeAudited(core,1));
		assert(ledger->Record(core,descriptor) && ledger->Verify(core,descriptor));
	}
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		const auto* owned=ledger->BeginPublish(core,2);
		assert(owned && owned->token.token==43 && owned->token.position==1 && owned->ticket.epoch==2);
	}
	assert(!ledger->CanStage(0,descriptor));
	// Exercise the actual publisher against the ledger for two consecutive
	// complete graphs and a partial staging failure.
	auto integrated=std::make_unique<DataflowGraphStageLedger>();
	auto firstIO=std::make_unique<LedgerPublisherIO>(*integrated);DataflowGraphPublisher firstPublisher;
	ticket={1,1,548};assert(DataflowDynamicTokenContext::Build(f->manifest,42,0,token));
	assert(firstPublisher.Start(*plan,f->routes,token,ticket,1,100,0,*firstIO,descriptor)==DataflowGraphPublished
		&& firstIO->staged==120 && firstIO->checked==120 && firstIO->published==120);
	for (unsigned core=0;core<DataflowCoreCount;core++)
		assert(integrated->RecordAudit(core,firstIO->records[core]));
	auto nextIO=std::make_unique<LedgerPublisherIO>(*integrated);nextIO->previous=1;DataflowGraphPublisher nextPublisher;
	nextIO->requireAudit=true;
	ticket={2,549,545};assert(DataflowDynamicTokenContext::Build(f->manifest,43,1,token));
	assert(nextPublisher.Start(*plan,f->routes,token,ticket,2,100,0,*nextIO,descriptor)==DataflowGraphPublished
		&& nextIO->published==120);
	auto failedLedger=std::make_unique<DataflowGraphStageLedger>();
	auto failedIO=std::make_unique<LedgerPublisherIO>(*failedLedger);failedIO->failStage=7;DataflowGraphPublisher failedPublisher;
	ticket={1,1,548};
	assert(failedPublisher.Start(*plan,f->routes,token,ticket,1,100,0,*failedIO,descriptor)==DataflowGraphPublishStagingFault
		&& failedPublisher.Faulted() && !failedIO->published && !failedLedger->BeginPublish(0,1));
	puts("PASS graph stage ledger: exact descriptors, all-core staging, ordered single-use publication; SIMULATED ONLY");
}
