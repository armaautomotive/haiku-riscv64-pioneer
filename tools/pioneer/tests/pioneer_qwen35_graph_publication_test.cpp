/* SPDX-License-Identifier: MIT */
#define main q35_publisher_fixture_main
#include "pioneer_qwen35_dataflow_stream_recorder_test.cpp"
#undef main
#include "../PioneerQwen35DataflowGraphPublisher.h"

struct PublisherIO {
	DataflowGraphRunDescriptor records[DataflowCoreCount]{};
	unsigned staged=0,checked=0,published=0;
	int failPreflight=-1,failStage=-1,failReadback=-1,failPublish=-1;
	bool ReadWorkerSnapshot(unsigned core,DataflowWorkerSnapshot& s) {
		if (int(core)==failPreflight) return false;
		s={4,4,4,DataflowWorkerReady,0,0,0,0};return true;
	}
	bool CoreReady(unsigned,const DataflowWorkerSnapshot&) { return true; }
	bool StageGraphRun(unsigned core,const DataflowGraphRunDescriptor& d) {
		assert(core==staged && published==0 && DataflowGraphRunValid(d,core));
		if (int(core)==failStage) return false;
		records[core]=d;staged++;return true;
	}
	bool GraphRunMatches(unsigned core,const DataflowGraphRunDescriptor& d) {
		assert(core==checked && staged==core+1 && published==0);
		if (int(core)==failReadback) return false;
		checked++;return std::memcmp(&records[core],&d,sizeof(d))==0;
	}
	bool PublishGraphCommand(unsigned core,unsigned trigger) {
		assert(staged==DataflowCoreCount && checked==DataflowCoreCount && core==published
			&& records[core].triggerSequence==trigger);
		if (int(core)==failPublish) return false;
		published++;return true;
	}
};
int main() {
	(void)&q35_publisher_fixture_main;
	auto f=std::make_unique<Fixture>();auto plan=std::make_unique<DataflowStreamPlan>();
	assert(DataflowStreamPlanBuilder::Build(f->packed,f->manifest,f->workspace,f->rope,f->routes,99,548,*plan));
	DataflowDynamicTokenContext token{};assert(DataflowDynamicTokenContext::Build(f->manifest,42,0,token));
	DataflowGraphRunTicket ticket{5,1001,548};DataflowGraphRunDescriptor scratch{};
	auto io=std::make_unique<PublisherIO>();DataflowGraphPublisher success;
	assert(success.Start(*plan,f->routes,token,ticket,5,10000,*io,scratch)==DataflowGraphPublished
		&& io->staged==120 && io->checked==120 && io->published==120 && !success.Faulted());
	assert(!success.Start(*plan,f->routes,token,ticket,6,10000,*io,scratch));
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		assert(DataflowGraphRunValid(io->records[core],core));
		auto forged=io->records[core];forged.token.token^=1;
		assert(!DataflowGraphRunValid(forged,core));
		forged=io->records[core];forged.reserved[0]=1;assert(!DataflowGraphRunValid(forged,core));
		assert(!DataflowGraphRunValid(io->records[core],(core+1)%DataflowCoreCount));
	}
	auto pre=std::make_unique<PublisherIO>();pre->failPreflight=8;DataflowGraphPublisher preOwner;
	assert(preOwner.Start(*plan,f->routes,token,ticket,5,10000,*pre,scratch)==DataflowGraphPublishPreflight
		&& !preOwner.Faulted() && !pre->staged && !pre->published);
	auto stage=std::make_unique<PublisherIO>();stage->failStage=7;DataflowGraphPublisher stageOwner;
	assert(stageOwner.Start(*plan,f->routes,token,ticket,5,10000,*stage,scratch)==DataflowGraphPublishStagingFault
		&& stageOwner.Faulted() && stageOwner.FailedCore()==7 && !stage->published);
	auto read=std::make_unique<PublisherIO>();read->failReadback=9;DataflowGraphPublisher readOwner;
	assert(readOwner.Start(*plan,f->routes,token,ticket,5,10000,*read,scratch)==DataflowGraphPublishStagingFault
		&& readOwner.Faulted() && readOwner.FailedCore()==9 && !read->published);
	auto partial=std::make_unique<PublisherIO>();partial->failPublish=49;DataflowGraphPublisher partialOwner;
	assert(partialOwner.Start(*plan,f->routes,token,ticket,5,10000,*partial,scratch)==DataflowGraphPublishCommandFault
		&& partialOwner.Faulted() && partialOwner.FailedCore()==49 && partial->published==49);
	assert(!partialOwner.Start(*plan,f->routes,token,ticket,5,10000,*partial,scratch));
	auto invalid=std::make_unique<PublisherIO>();DataflowGraphPublisher invalidOwner;
	ticket.firstSequence=DataflowWorkerSequenceLimit;
	assert(invalidOwner.Start(*plan,f->routes,token,ticket,5,10000,*invalid,scratch)==DataflowGraphPublishPreflight
		&& !invalid->staged && !invalid->published);
	puts("PASS graph run publication: verified descriptors on 120 workers before start; partial writes retained; SIMULATED ONLY");
}
