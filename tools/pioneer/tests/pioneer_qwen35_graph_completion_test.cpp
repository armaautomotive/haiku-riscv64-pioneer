/* SPDX-License-Identifier: MIT */
#define Q35_PUBLISHER_FIXTURE_ONLY
#include "pioneer_qwen35_graph_publication_test.cpp"
#include "../PioneerQwen35DataflowGraphCompletion.h"
#include "../PioneerQwen35DataflowGraphWait.h"

struct WaitIO {
	uint64_t now=0,increment=10000000;
	unsigned reads=0;
	DataflowWorkerSnapshot samples[8]{};
	unsigned count=0;
	bool alwaysAdvance=false;
	uint64_t NowMicros() const { return now; }
	bool ReadWorkerSnapshot(unsigned,DataflowWorkerSnapshot& out) {
		if (alwaysAdvance) {
			out={5,4,4,DataflowWorkerRunning,0,1,0x40000001U|(reads++<<4),0};
			return true;
		}
		if (!count) return false;
		out=samples[reads<count?reads:count-1];reads++;return true;
	}
	void Pause() { now+=increment; }
};
[[maybe_unused]] static DataflowWorkerSnapshot running(unsigned step,unsigned phase,unsigned command=5) {
	return {command,4,4,DataflowWorkerRunning,0,1,0x40000000U|(step<<4)|phase,0};
}

struct CompletionIO {
	const PublisherIO& published;
	unsigned waited=0,audited=0,acks=0,resultReads=0;
	int failWorker=-1,failAudit=-1,failAck=-1,failReady=-1;
	bool owner=true,badResult=false,unstableResult=false,terminalFailure=false;
	bool GraphOwnershipMatches(const DataflowStreamPlan&,const DataflowDynamicTokenContext&,
		const DataflowGraphRunTicket&,unsigned slot) { return owner && slot==0; }
	bool WaitForCompletion(unsigned core,unsigned trigger,DataflowWorkerSnapshot& r) {
		assert(core==waited++);
		if (int(core)==failWorker) {
			if (terminalFailure) {
				r={trigger,published.previous,trigger,DataflowWorkerFailed,6,5,0x80000015U,0x1234U};
				return true;
			}
			r={trigger,published.previous,published.previous,DataflowWorkerRunning,0,1,0,0};
			return false;
		}
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
	unsigned failedCore=0;DataflowWorkerSnapshot failedSnapshot{};
	assert(missingOwner.FailedWorkerSnapshot(failedCore,failedSnapshot)
		&& failedCore==7 && failedSnapshot.state==DataflowWorkerRunning
		&& failedSnapshot.commandSequence==5 && failedSnapshot.completedSequence==4);
	assert(!done.FailedWorkerSnapshot(failedCore,failedSnapshot));
	CompletionIO failed{*published};failed.failWorker=4;failed.terminalFailure=true;
	DataflowGraphCompletion failedOwner;
	assert(failedOwner.Finish(publisher,*plan,f->routes,token,failed,scratch,output)==DataflowGraphCompletionWorkerFault
		&& failedOwner.FailedWorkerSnapshot(failedCore,failedSnapshot)
		&& failedCore==4 && failedSnapshot.state==DataflowWorkerFailed
		&& failedSnapshot.failureDetail==0x80000015U && failedSnapshot.failureStatus==0x1234U);
	DataflowWorkerMailbox fault{};unsigned detail=0,status=0;
	fault.reserved[0]=DataflowGraphFaultMagic;fault.reserved[1]=DataflowGraphDeadlineFault;
	fault.reserved[2]=12;fault.reserved[3]=119;fault.reserved[4]=0x12345678;
	assert(DataflowGraphFaultDetail(fault,detail,status)
		&& (detail>>31)==1 && (detail&0xfU)==DataflowGraphDeadlineFault && ((detail>>4)&0x3ffU)==12
		&& ((detail>>14)&0x7fU)==119 && status==0x12345678);
	fault.reserved[3]=DataflowCoreCount+1;
	assert(!DataflowGraphFaultDetail(fault,detail,status));
	uint32_t progress=0;
	for (unsigned step: {0U,2U,DataflowGraphDecodeSteps-1U}) {
		for (unsigned phase=DataflowGraphProgressFetch;
			phase<=DataflowGraphProgressResetQueues;phase++) {
			progress=DataflowGraphProgressWord(step,phase);
			assert(DataflowGraphProgressDetail(progress,detail)
				&& detail==(0x40000000U|(step<<4)|phase));
		}
	}
	assert(!DataflowGraphProgressWord(DataflowGraphDecodeSteps,DataflowGraphProgressFetch));
	assert(!DataflowGraphProgressWord(0,0) && !DataflowGraphProgressWord(0,6));
	progress=0;assert(!DataflowGraphProgressDetail(progress,detail));
	progress=DataflowGraphProgressMagic|(DataflowGraphDecodeSteps<<3)|1;
	assert(!DataflowGraphProgressDetail(progress,detail));
	progress=DataflowGraphProgressMagic|6;
	assert(!DataflowGraphProgressDetail(progress,detail));
	WaitIO advancing;advancing.increment=20000000;advancing.count=4;
	advancing.samples[0]=running(0,DataflowGraphProgressFetch);
	advancing.samples[1]=running(0,DataflowGraphProgressExecute);
	advancing.samples[2]=running(1,DataflowGraphProgressFetch);
	advancing.samples[3]={5,4,5,DataflowWorkerComplete,0,2,0,0};
	DataflowWorkerSnapshot waited{};
	assert(DataflowGraphWaitForCompletion(advancing,0,5,0,waited)
		&& advancing.now==60000000 && waited.state==DataflowWorkerComplete);
	WaitIO stalled;stalled.count=1;stalled.samples[0]=running(2,DataflowGraphProgressExecute);
	assert(!DataflowGraphWaitForCompletion(stalled,0,5,0,waited)
		&& stalled.now==DataflowGraphWaitIdleMicros && waited.failureDetail==stalled.samples[0].failureDetail);
	WaitIO wrongSequence;wrongSequence.count=1;wrongSequence.samples[0]=running(2,DataflowGraphProgressExecute,4);
	assert(!DataflowGraphWaitForCompletion(wrongSequence,0,5,0,waited)
		&& wrongSequence.now==DataflowGraphWaitIdleMicros);
	WaitIO fatal;fatal.count=1;fatal.samples[0]={5,4,4,DataflowWorkerFatal,1,7,0,0};
	assert(!DataflowGraphWaitForCompletion(fatal,0,5,0,waited) && !fatal.now);
	WaitIO expired;expired.now=DataflowGraphWaitTotalMicros;
	expired.count=1;expired.samples[0]={5,4,5,DataflowWorkerComplete,0,2,0,0};
	assert(!DataflowGraphWaitForCompletion(expired,0,5,0,waited) && !expired.reads);
	WaitIO progressingTooLong;progressingTooLong.alwaysAdvance=true;
	assert(!DataflowGraphWaitForCompletion(progressingTooLong,0,5,0,waited)
		&& progressingTooLong.now==DataflowGraphWaitTotalMicros
		&& progressingTooLong.reads==60);
	WaitIO backwards;backwards.now=1;
	assert(!DataflowGraphWaitForCompletion(backwards,0,5,2,waited));
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
