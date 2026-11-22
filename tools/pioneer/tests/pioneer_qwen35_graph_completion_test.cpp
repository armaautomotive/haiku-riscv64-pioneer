/* SPDX-License-Identifier: MIT */
#define Q35_PUBLISHER_FIXTURE_ONLY
#include "pioneer_qwen35_graph_publication_test.cpp"
#include "../PioneerQwen35DataflowGraphCompletion.h"

struct CompletionIO {
	const PublisherIO& published;
	unsigned waited=0,audited=0,acks=0,resultReads=0;
	int failWorker=-1,failAudit=-1,failAck=-1,failReady=-1;
	bool owner=true,badResult=false,unstableResult=false;
	bool GraphOwnershipMatches(const DataflowStreamPlan&,const DataflowDynamicTokenContext&,
		const DataflowGraphRunTicket&,unsigned slot) { return owner && slot==0; }
	bool WaitForCompletion(unsigned core,unsigned trigger,DataflowWorkerSnapshot& r) {
		assert(core==waited++);
		if (int(core)==failWorker) return false;
		r={trigger,published.previous,trigger,DataflowWorkerComplete,0,2,0,0};return true;
	}
	bool AuditGraphCore(unsigned core,const DataflowGraphRunDescriptor& d,const DataflowWorkerSnapshot& r,bool decode) {
		assert(waited==DataflowCoreCount && core==audited++ && r.state==DataflowWorkerComplete
			&& decode==(d.ticket.count==DataflowGraphDecodeSteps));
		return int(core)!=failAudit && !std::memcmp(&d,&published.records[core],sizeof(d));
	}
	bool ReadArgmaxResult(unsigned sequence,DataflowArgmaxResult& out) {
		resultReads++;out={42,0x3f800000,sequence,0};
		if (unstableResult && resultReads>1) out.token++;
		out.checksum=DataflowArgmaxResultChecksum(out.token,out.scoreBits,sequence);
		if (badResult) out.checksum^=1;
		return true;
	}
	bool Acknowledge(unsigned core,unsigned) {
		assert(waited==DataflowCoreCount && audited==DataflowCoreCount && core==acks);
		if (int(core)==failAck) return false;
		acks++;return true;
	}
	bool ReadWorkerSnapshot(unsigned core,DataflowWorkerSnapshot& s) {
		if (int(core)==failReady) return false;
		const unsigned trigger=published.records[core].triggerSequence;
		s={trigger,trigger,trigger,DataflowWorkerReady,0,0,0,0};return true;
	}
	bool CoreReady(unsigned,const DataflowWorkerSnapshot&) { return true; }
};
#ifndef Q35_COMPLETION_FIXTURE_ONLY
int main() {
	(void)&q35_publisher_fixture_main;
	auto f=std::make_unique<Fixture>();auto plan=std::make_unique<DataflowStreamPlan>();
	assert(DataflowStreamPlanBuilder::Build(f->packed,f->manifest,f->workspace,f->rope,f->routes,99,548,*plan));
	DataflowDynamicTokenContext token{};assert(DataflowDynamicTokenContext::Build(f->manifest,42,0,token));
	DataflowGraphRunTicket ticket{5,1001,548};DataflowGraphRunDescriptor scratch{};
	auto published=std::make_unique<PublisherIO>();DataflowGraphPublisher publisher;
	assert(publisher.Start(*plan,f->routes,token,ticket,5,3,0,*published,scratch)==DataflowGraphPublished);
	DataflowArgmaxResult output{};CompletionIO good{*published};DataflowGraphCompletion done;
	assert(done.Finish(publisher,*plan,f->routes,token,good,scratch,output)==DataflowGraphCompletionAudited
		&& output.token==42 && output.sequence==1548 && good.acks==120 && good.resultReads==2);
	assert(done.Finish(publisher,*plan,f->routes,token,good,scratch,output)==DataflowGraphCompletionInvalid);
	CompletionIO noOwner{*published};noOwner.owner=false;DataflowGraphCompletion rejected;
	assert(rejected.Finish(publisher,*plan,f->routes,token,noOwner,scratch,output)==DataflowGraphCompletionOwnershipFault
		&& rejected.FaultedState() && !noOwner.waited && output.token==Vocabulary);
	CompletionIO missing{*published};missing.failWorker=7;DataflowGraphCompletion missingOwner;
	assert(missingOwner.Finish(publisher,*plan,f->routes,token,missing,scratch,output)==DataflowGraphCompletionWorkerFault
		&& missingOwner.FailedCore()==7 && !missing.acks && output.token==Vocabulary);
	CompletionIO badAudit{*published};badAudit.failAudit=8;DataflowGraphCompletion badAuditOwner;
	assert(badAuditOwner.Finish(publisher,*plan,f->routes,token,badAudit,scratch,output)==DataflowGraphCompletionAuditFault
		&& badAuditOwner.FailedCore()==8 && !badAudit.acks);
	CompletionIO badResult{*published};badResult.badResult=true;DataflowGraphCompletion badResultOwner;
	assert(badResultOwner.Finish(publisher,*plan,f->routes,token,badResult,scratch,output)==DataflowGraphCompletionResultFault
		&& !badResult.acks && output.token==Vocabulary);
	CompletionIO unstable{*published};unstable.unstableResult=true;DataflowGraphCompletion unstableOwner;
	assert(unstableOwner.Finish(publisher,*plan,f->routes,token,unstable,scratch,output)==DataflowGraphCompletionResultFault
		&& !unstable.acks);
	CompletionIO partial{*published};partial.failAck=9;DataflowGraphCompletion partialOwner;
	assert(partialOwner.Finish(publisher,*plan,f->routes,token,partial,scratch,output)==DataflowGraphCompletionAcknowledgeFault
		&& partial.acks==9 && partialOwner.FaultedState() && output.token==Vocabulary);
	CompletionIO unready{*published};unready.failReady=17;DataflowGraphCompletion unreadyOwner;
	assert(unreadyOwner.Finish(publisher,*plan,f->routes,token,unready,scratch,output)==DataflowGraphCompletionReadyFault
		&& unreadyOwner.FailedCore()==17 && unready.acks==120 && output.token==Vocabulary);
	// Prompt execution uses the first 545 operations of the same 548-record
	// sealed stream; it does not claim or upload a second stream.
	ticket={6,2000,545};auto promptPublished=std::make_unique<PublisherIO>();DataflowGraphPublisher promptPublisher;
	assert(promptPublisher.Start(*plan,f->routes,token,ticket,5,3,0,*promptPublished,scratch)==DataflowGraphPublished);
	assert(promptPublished->records[0].binding.count==548 && promptPublished->records[0].ticket.count==545);
	CompletionIO prompt{*promptPublished};DataflowGraphCompletion promptOwner;
	assert(promptOwner.Finish(promptPublisher,*plan,f->routes,token,prompt,scratch,output)==DataflowGraphCompletionAudited
		&& output.token==Vocabulary && !prompt.resultReads && prompt.acks==120);
	auto bad=std::make_unique<PublisherIO>();DataflowGraphPublisher badPublisher;
	ticket.count=546;
	assert(badPublisher.Start(*plan,f->routes,token,ticket,5,3,0,*bad,scratch)==DataflowGraphPublishInvalid
		&& !bad->staged && !bad->published);
	puts("PASS graph completion: all-core audit before acknowledgement, stable argmax, failure retention; SIMULATED ONLY");
}
#endif
