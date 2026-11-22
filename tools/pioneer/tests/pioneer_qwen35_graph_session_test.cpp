/* SPDX-License-Identifier: MIT */
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define main q35_session_service_fixture_main
#include "pioneer_qwen35_dataflow_service_owner_test.cpp"
#undef main
#define Q35_COMPLETION_FIXTURE_ONLY
#include "pioneer_qwen35_graph_completion_test.cpp"
#include "../PioneerQwen35DataflowGraphSession.h"
#include <memory>

struct StreamClaim { const DataflowGraphSession* owner=nullptr; };
struct SessionIO : PublisherIO {
	StreamClaim& claim;
	explicit SessionIO(StreamClaim& c):claim(c) {}
	unsigned waited=0,audited=0,acks=0,resultReads=0;
	unsigned runAllocations=0,runDeletes=0,candidateAllocations=0,candidateDeletes=0;
	int failAudit=-1;bool sealed=true,safe=true,restored=false,failRunAlloc=false,failCandidateAlloc=false;
	DataflowProjectionAuthority* aliasCandidate=nullptr;
	bool GraphSessionSafe(const DataflowServiceOwner&) { return safe; }
	DataflowGraphSessionRun* NewGraphRun() {
		if (failRunAlloc) return nullptr;
		runAllocations++;return new DataflowGraphSessionRun;
	}
	DataflowProjectionAuthority* NewGraphCandidate() {
		if (failCandidateAlloc) return nullptr;
		if (aliasCandidate) return aliasCandidate;
		candidateAllocations++;return new DataflowProjectionAuthority;
	}
	void DeleteGraphRun(DataflowGraphSessionRun* p) { if (p) { runDeletes++;delete p; } }
	void DeleteGraphCandidate(DataflowProjectionAuthority* p) { if (p) { candidateDeletes++;delete p; } }
	bool GraphFleetRestored() { return restored; }
	bool SealedStreamMatches(const DataflowStreamPlan& p,unsigned slot) {
		return sealed && p.ready && p.count==548 && slot==0;
	}
	bool ReserveGraphSession(const DataflowGraphSession* owner,const DataflowStreamPlan& p,unsigned slot) {
		if (!owner || claim.owner || !SealedStreamMatches(p,slot)) return false;
		claim.owner=owner;return true;
	}
	bool OwnsGraphSession(const DataflowGraphSession* owner,const DataflowStreamPlan& p,unsigned slot) {
		return owner && claim.owner==owner && SealedStreamMatches(p,slot);
	}
	bool ReadWorkerSnapshot(unsigned core,DataflowWorkerSnapshot& s) {
		if (acks==DataflowCoreCount) {
			const unsigned trigger=records[core].triggerSequence;
			s={trigger,trigger,trigger,DataflowWorkerReady,0,0,0,0};return true;
		}
		return PublisherIO::ReadWorkerSnapshot(core,s);
	}
	bool WaitForCompletion(unsigned core,unsigned trigger,DataflowWorkerSnapshot& s) {
		assert(published==DataflowCoreCount && core==waited++);
		s={trigger,previous,trigger,DataflowWorkerComplete,0,2,0,0};return true;
	}
	bool AuditGraphCore(unsigned core,const DataflowGraphRunDescriptor& d,const DataflowWorkerSnapshot& s,bool decode) {
		assert(waited==DataflowCoreCount && core==audited++ && s.state==DataflowWorkerComplete
			&& decode==(d.ticket.count==DataflowGraphDecodeSteps));
		return int(core)!=failAudit && !std::memcmp(&d,&records[core],sizeof(d));
	}
	bool ReadArgmaxResult(unsigned sequence,DataflowArgmaxResult& out) {
		resultReads++;out={42,0x3f800000,sequence,0};
		out.checksum=DataflowArgmaxResultChecksum(out.token,out.scoreBits,sequence);return true;
	}
	bool Acknowledge(unsigned core,unsigned) {
		assert(waited==DataflowCoreCount && audited==DataflowCoreCount && core==acks);acks++;return true;
	}
};

#ifndef Q35_GRAPH_SESSION_FIXTURE_ONLY
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
	auto liveOwner=std::make_unique<DataflowProjectionAuthority>();
	assert(liveOwner->BindValidatedModel(f->manifest,f->workspace,f->routes,f->rope));
	DataflowProjectionAuthority* live=liveOwner.get();
	StreamClaim claim;auto session=std::make_unique<DataflowGraphSession>();SessionIO bindIO(claim);
	bindIO.safe=false;
	assert(!session->Bind(*plan,f->routes,0,live,*service,bindIO));
	bindIO.safe=true;bindIO.sealed=false;
	assert(!session->Bind(*plan,f->routes,0,live,*service,bindIO));
	bindIO.sealed=true;
	assert(session->Bind(*plan,f->routes,0,live,*service,bindIO));
	auto duplicate=std::make_unique<DataflowGraphSession>();
	assert(!duplicate->Bind(*plan,f->routes,0,live,*service,bindIO));
	auto projection=std::make_unique<DataflowProjectionProgram>();
	for (unsigned position=0;position<2;position++) {
		auto run=std::make_unique<DataflowGraphSessionRun>();
		auto candidate=std::make_unique<DataflowProjectionAuthority>();
		auto io=std::make_unique<SessionIO>(claim);io->previous=position;
		DataflowArgmaxResult output{};DataflowProjectionAuthority* retired=nullptr;unsigned stage=0;
		const auto result=session->Run(*run,*candidate,position,42+position,position!=0,3,
			*projection,*io,output,retired,stage);
		assert(result==DataflowGraphSessionComplete && stage==DataflowDecoderSequenceComplete
			&& retired==liveOwner.get() && live==candidate.get() && live->ConvNextPosition(0)==position+1
			&& service->Ready() && service->NextSequence()==position+1 && io->acks==120);
		assert(position?output.token==42:output.token==Vocabulary);
		assert(io->resultReads==(position?2U:0U));
		liveOwner.swap(candidate);
	}
	auto refused=std::make_unique<DataflowGraphSessionRun>();auto unused=std::make_unique<DataflowProjectionAuthority>();
	SessionIO preflight(claim);preflight.previous=2;DataflowArgmaxResult output{};
	DataflowProjectionAuthority* retired=nullptr;unsigned stage=0;
	assert(session->Run(*refused,*unused,f->manifest.context,3,true,3,*projection,preflight,output,retired,stage)
		==DataflowGraphSessionPreflight && service->Ready() && !retired && output.token==Vocabulary);
	assert(session->Run(*refused,*unused,2,3,true,3,*projection,preflight,output,retired,stage)
		==DataflowGraphSessionInvalid);
	auto failed=std::make_unique<DataflowGraphSessionRun>();auto retainedCandidate=std::make_unique<DataflowProjectionAuthority>();
	SessionIO bad(claim);bad.previous=2;bad.failAudit=7;
	assert(session->Run(*failed,*retainedCandidate,2,44,true,3,*projection,bad,output,retired,stage)
		==DataflowGraphSessionRetained && session->Faulted() && session->RetainedRun()==failed.get()
		&& service->Faulted() && service->Close(*serviceIO)==DataflowServiceInvalidState
		&& live==liveOwner.get() && live->ConvNextPosition(0)==2 && !retired && output.token==Vocabulary && !bad.acks);
	puts("PASS graph session: sealed plan, fresh tickets, two audited state commits and retained fault; SIMULATED ONLY");
	return 0;
}
#endif
