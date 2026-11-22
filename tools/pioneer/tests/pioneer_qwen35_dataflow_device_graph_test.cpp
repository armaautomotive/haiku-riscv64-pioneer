/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowDeviceGraph.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>

using namespace PioneerQwen35;
struct Slot { DataflowBarrierPacket payload{},commit{}; };
struct Fabric { Slot arrival[DataflowBarrierWorkers],release[DataflowBarrierWorkers];
	int failArrival=-1,failRelease=-1;unsigned sends[DataflowBarrierWorkers]{}; };
struct Transport {
	Fabric& f;unsigned core;
	bool SendArrival(unsigned worker,const DataflowBarrierPacket& p,const DataflowBarrierPacket& c) {
		assert(core==worker && core);if (int(core)==f.failArrival) return false;
		f.arrival[core]={p,c};return true;
	}
	bool SendRelease(unsigned worker,const DataflowBarrierPacket& p,const DataflowBarrierPacket& c) {
		assert(DataflowBarrierReleaseEdge(core,worker));f.sends[core]++;
		if (int(worker)==f.failRelease) return false;
		f.release[worker]={p,c};return true;
	}
	DataflowBarrierReadResult ReadArrival(unsigned worker,unsigned epoch,unsigned step,DataflowBarrierPacket& out) {
		assert(DataflowBarrierReleaseEdge(core,worker));
		return DataflowBarrierReadCommitted(f.arrival[worker].payload.word,f.arrival[worker].commit.word,epoch,step,out);
	}
	DataflowBarrierReadResult ReadRelease(unsigned worker,unsigned epoch,unsigned step,DataflowBarrierPacket& out) {
		assert(worker==core);return DataflowBarrierReadCommitted(f.release[core].payload.word,f.release[core].commit.word,epoch,step,out);
	}
};
struct Execution {
	unsigned core,routes[2][Banks]{},fetches=0,executes=0,resets=0,firstSequence=1;
	DataflowStreamBinding binding{};Range lease{};
	DataflowStreamRecord record[2]{},staging{};
	alignas(64) uint32_t program[DataflowProgramRegionBytes/4]{};
	int failFetch=-1,failAuthority=-1,failExecute=-1,workerFailure=-1,failReset=-1;
	explicit Execution(unsigned worker):core(worker) {
		for (unsigned n=0;n<2;n++) for (unsigned b=0;b<Banks;b++) routes[n][b]=1+n*16+b;
		binding={DataflowStreamBindingMagic,DataflowStreamVersion,17,core,2,3,0x08000000,2};
		lease={2,0x08000000,4096};
		DataflowEmbeddingProgram p{};p.token=15;p.lane=15;p.outputSlot=0;
		p.weightBank=0;p.weightBase=BankBase;
		for (unsigned n=0;n<2;n++) for (unsigned b=0;b<Banks;b++) p.bankRoute[n][b]=routes[n][b];
		p.output.rows=Hidden;p.output.rowBytes=4;p.output.count=1;p.output.unit=16;
		p.output.parts[0]={1,0x10000000,0,Hidden};
		DataflowEmbeddingInvocation invocation{};
		assert(DataflowEmbeddingProgramBuilder::BindCore(p,core,invocation));
		for (unsigned i=0;i<2;i++) assert(DataflowStreamEncode(invocation,17,i+1,record[i]));
	}
	bool ReadStream(unsigned bank,unsigned address,unsigned route,uint8_t* output,unsigned bytes) {
		const unsigned index=(address-binding.base)/DataflowStreamRecordBytes;
		if (bank!=2 || route!=3 || index>=2 || bytes!=sizeof(record[0]) || int(index)==failFetch) return false;
		std::memcpy(output,&record[index],bytes);return true;
	}
	bool Fetch(unsigned index) { fetches++;return DataflowStreamFetch(*this,binding,lease,routes,core,index,staging,program,sizeof(program)); }
	bool Authorize(unsigned index) { return int(index)!=failAuthority && index<2
		&& std::memcmp(program,record[index].payload,record[index].header[6])==0; }
	bool Execute(unsigned sequence,unsigned& status,unsigned& detail) {
		assert(sequence==firstSequence+executes);
		executes++;if (int(sequence-firstSequence)==failExecute) { detail=0x80001234U;return false; }
		status=int(sequence-firstSequence)==workerFailure?77:0;detail=status?123:0;return true;
	}
	bool ResetQueues() { return int(resets++)!=failReset; }
};
using Graph=DataflowDeviceGraph<Execution,Transport>;
struct Fleet {
	Fabric fabric;
	std::array<std::unique_ptr<Execution>,DataflowBarrierWorkers> execution;
	std::array<std::unique_ptr<Transport>,DataflowBarrierWorkers> transport;
	std::array<std::unique_ptr<Graph>,DataflowBarrierWorkers> graph;
	explicit Fleet(unsigned budget=8,unsigned runEpoch=77,unsigned firstSequence=1,unsigned count=2) {
		for (unsigned i=0;i<DataflowBarrierWorkers;i++) {
			execution[i]=std::make_unique<Execution>(i);
			execution[i]->firstSequence=firstSequence;
			transport[i]=std::make_unique<Transport>(Transport{fabric,i});
			graph[i]=std::make_unique<Graph>(*execution[i],*transport[i]);
			assert(graph[i]->Start(execution[i]->binding,i,budget,runEpoch,firstSequence,count));
		}
	}
	void Run(unsigned rounds=10) {
		for (unsigned round=0;round<rounds;round++)
			for (auto& worker:graph) worker->Tick();
	}
};
int main() {
	DataflowGraphRunIssuer issuer;DataflowGraphRunTicket first{},second{};
	assert(!issuer.Issue(0,first) && !issuer.Issue(DataflowStreamMaxRecords+1,first));
	assert(issuer.Issue(548,first) && issuer.Issue(545,second));
	assert(first.epoch==1 && first.firstSequence==1 && first.count==548
		&& second.epoch==2 && second.firstSequence==549 && second.count==545);
	DataflowGraphRunIssuer exhausted;DataflowGraphRunTicket ticket{};
	for (unsigned n=0;n<DataflowWorkerSequenceLimit/DataflowStreamMaxRecords;n++)
		assert(exhausted.Issue(DataflowStreamMaxRecords,ticket));
	const unsigned remainder=DataflowWorkerSequenceLimit%DataflowStreamMaxRecords;
	assert(remainder && !exhausted.Issue(remainder+1,ticket) && exhausted.Issue(remainder,ticket)
		&& !exhausted.Issue(1,ticket));
	Fleet ok;ok.Run();
	assert(ok.fabric.sends[0]==2*DataflowBarrierReleaseFanout);
	for (unsigned i=0;i<DataflowBarrierWorkers;i++) {
		assert(ok.graph[i]->State()==DataflowGraphLocalComplete && ok.graph[i]->CompletedSteps()==2);
		assert(ok.execution[i]->fetches==2 && ok.execution[i]->executes==2 && ok.execution[i]->resets==1);
	}
	// Exercise out-of-order scheduling: every round visits all 120 workers,
	// but parents are often polled after children and must not lose releases.
	Fleet shuffled(256,80,3001);
	for (unsigned round=0;round<32;round++)
		for (unsigned i=0;i<DataflowBarrierWorkers;i++)
			shuffled.graph[(round*17+i*37)%DataflowBarrierWorkers]->Tick();
	for (const auto& graph:shuffled.graph)
		assert(graph->State()==DataflowGraphLocalComplete && graph->CompletedSteps()==2);
	Fleet fresh(8,78,1001);fresh.Run();
	for (auto& g:fresh.graph) assert(g->State()==DataflowGraphLocalComplete);
	assert(fresh.fabric.release[1].commit.word[0]==78 && fresh.fabric.release[1].commit.word[2]==2);
	Fleet prefix(8,79,2001,1);prefix.Run();
	for (unsigned i=0;i<DataflowBarrierWorkers;i++) {
		assert(prefix.graph[i]->State()==DataflowGraphLocalComplete && prefix.graph[i]->CompletedSteps()==1);
		assert(prefix.execution[i]->fetches==1 && prefix.execution[i]->executes==1 && !prefix.execution[i]->resets);
	}
	Fabric invalidFabric;Execution invalidExecution(0);Transport invalidTransport{invalidFabric,0};
	Graph invalid(invalidExecution,invalidTransport);
	assert(!invalid.Start(invalidExecution.binding,0,8,0,1,2)
		&& !invalid.Start(invalidExecution.binding,0,8,78,0,2)
		&& !invalid.Start(invalidExecution.binding,0,8,78,DataflowWorkerSequenceLimit,2)
		&& !invalid.Start(invalidExecution.binding,0,8,78,1,0)
		&& !invalid.Start(invalidExecution.binding,0,8,78,1,3));
	Fleet fetch;fetch.execution[7]->failFetch=0;fetch.Run();
	assert(fetch.graph[7]->Fault()==DataflowGraphFetchFault && fetch.execution[7]->executes==0);
	for (auto& g:fetch.graph) assert(g->State()==DataflowGraphRetain);
	Fleet authority;authority.execution[8]->failAuthority=0;authority.Run();
	assert(authority.graph[8]->Fault()==DataflowGraphAuthorityFault && authority.execution[8]->executes==0);
	Fleet uncertain;uncertain.execution[9]->failExecute=0;uncertain.Run();
	assert(uncertain.graph[9]->Fault()==DataflowGraphExecutionUncertain
		&& uncertain.graph[9]->FailureCore()==9 && uncertain.graph[9]->FailureStatus()==0x80001234U);
	Fleet failure;failure.execution[5]->workerFailure=0;failure.Run();
	for (auto& g:failure.graph) assert(g->Fault()==DataflowGraphWorkerFault && g->FailureCore()==5 && g->FailureStatus()==77);
	Fleet partial;partial.fabric.failRelease=12;partial.Run(30);
	assert(partial.graph[2]->Fault()==DataflowGraphBarrierFault);
	for (auto& g:partial.graph) assert(g->State()==DataflowGraphRetain);
	Fleet reset;reset.execution[7]->failReset=0;reset.Run();
	assert(reset.graph[7]->Fault()==DataflowGraphQueueFault && reset.execution[7]->fetches==1);
	assert(!reset.graph[7]->Start(reset.execution[7]->binding,7,8,78,3,2));
	puts("PASS device graph: 120 simulated workers, checked streams, barriers, retention and bounded deadlines; NO HARDWARE");
}
