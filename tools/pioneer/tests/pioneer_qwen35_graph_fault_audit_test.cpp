/* SPDX-License-Identifier: MIT */
#define Q35_PUBLISHER_FIXTURE_ONLY
#include "pioneer_qwen35_graph_publication_test.cpp"
#include "../PioneerQwen35DataflowGraphFaultAudit.h"

struct FaultIO {
	DataflowWorkerSnapshot worker[DataflowCoreCount]{};
	unsigned reads[DataflowCoreCount]{},fences=0,acks=0,audits=0;
	int badCore=-1,changeCore=-1,ackCore=-1,stuckCore=-1,badFence=-1;
	FaultIO() {
		for (unsigned c=0;c<DataflowCoreCount;c++) {
			if (c<40) worker[c]={5,4,5,DataflowWorkerComplete,0,2,0,0};
			else if (c<80) worker[c]={5,4,5,DataflowWorkerFailed,1,5,0,0};
			else worker[c]={4,4,4,DataflowWorkerReady,0,0,0,0};
		}
	}
	bool FenceGraphHostWrites() { return int(fences++)!=badFence; }
	void Pause() {}
	bool ReadWorkerSnapshot(unsigned c,DataflowWorkerSnapshot& s) {
		reads[c]++;s=worker[c];
		if (int(c)==changeCore && reads[c]>=3) s.failureStatus=1;
		return true;
	}
	bool AuditGraphRecoveryCore(unsigned c,const DataflowWorkerSnapshot&) {
		audits++;return int(c)!=badCore;
	}
	bool Acknowledge(unsigned c,unsigned sequence) {
		assert(audits==2*DataflowCoreCount && sequence==5);
		if (int(c)==ackCore) return false;
		acks++;
		if (int(c)!=stuckCore) worker[c]={5,5,5,DataflowWorkerReady,0,0,0,0};
		return true;
	}
};

int main() {
	(void)&q35_publisher_fixture_main;
	auto f=std::make_unique<Fixture>();auto plan=std::make_unique<DataflowStreamPlan>();
	assert(DataflowStreamPlanBuilder::Build(f->packed,f->manifest,f->workspace,f->rope,f->routes,99,548,*plan));
	DataflowDynamicTokenContext token{};assert(DataflowDynamicTokenContext::Build(f->manifest,42,0,token));
	DataflowGraphRunTicket ticket{5,1001,548};DataflowGraphRunDescriptor scratch{};
	auto publish=std::make_unique<PublisherIO>();publish->failPublish=50;DataflowGraphPublisher p;
	assert(p.Start(*plan,f->routes,token,ticket,5,3,0,*publish,scratch)==DataflowGraphPublishCommandFault);
	FaultIO good;DataflowGraphFaultAudit audit;
	assert(audit.Settle(p,3,good)==DataflowGraphFaultAuditSettled && good.acks==80 && good.fences==2);
	assert(audit.Settle(p,3,good)==DataflowGraphFaultAuditInvalid);
	FaultIO fence;fence.badFence=0;DataflowGraphFaultAudit noFence;
	assert(noFence.Settle(p,3,fence)==DataflowGraphFaultAuditFenceFailed && !fence.acks);
	FaultIO unsettled;unsettled.worker[7].state=DataflowWorkerRunning;DataflowGraphFaultAudit timeout;
	assert(timeout.Settle(p,3,unsettled)==DataflowGraphFaultAuditUnsettled && timeout.FailedCore()==7 && !unsettled.acks);
	FaultIO unsafe;unsafe.badCore=119;DataflowGraphFaultAudit badCore;
	assert(badCore.Settle(p,3,unsafe)==DataflowGraphFaultAuditCoreUnsafe && badCore.FailedCore()==119 && !unsafe.acks);
	FaultIO changed;changed.changeCore=5;DataflowGraphFaultAudit drift;
	assert(drift.Settle(p,3,changed)==DataflowGraphFaultAuditChanged && drift.FailedCore()==5 && !changed.acks);
	FaultIO partial;partial.ackCore=9;DataflowGraphFaultAudit badAck;
	assert(badAck.Settle(p,3,partial)==DataflowGraphFaultAuditAckFailed && partial.acks==9);
	FaultIO stuck;stuck.stuckCore=8;DataflowGraphFaultAudit noReady;
	assert(noReady.Settle(p,3,stuck)==DataflowGraphFaultAuditReadyFailed && noReady.FailedCore()==8);
	FaultIO finalFence;finalFence.badFence=1;DataflowGraphFaultAudit noFinalFence;
	assert(noFinalFence.Settle(p,3,finalFence)==DataflowGraphFaultAuditFenceFailed && finalFence.acks==80);
	FaultIO wrong;wrong.worker[3]={6,4,6,DataflowWorkerComplete,0,2,0,0};DataflowGraphFaultAudit ahead;
	assert(ahead.Settle(p,3,wrong)==DataflowGraphFaultAuditUnsettled && !wrong.acks);
	puts("PASS graph fault audit: bounded fleet settlement, fencing and failure retention; SIMULATED ONLY");
}
