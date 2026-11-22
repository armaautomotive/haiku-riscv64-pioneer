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
	int failArrival=-1,failRelease=-1; };
struct Transport {
	Fabric& f;unsigned core;
	bool SendArrival(unsigned worker,const DataflowBarrierPacket& p,const DataflowBarrierPacket& c) {
		assert(core==worker && core);if (int(core)==f.failArrival) return false;
		f.arrival[core]={p,c};return true;
	}
	bool SendRelease(unsigned worker,const DataflowBarrierPacket& p,const DataflowBarrierPacket& c) {
		assert(!core && worker);if (int(worker)==f.failRelease) return false;
		f.release[worker]={p,c};return true;
	}
	DataflowBarrierReadResult ReadArrival(unsigned worker,unsigned epoch,unsigned step,DataflowBarrierPacket& out) {
		return DataflowBarrierReadCommitted(f.arrival[worker].payload.word,f.arrival[worker].commit.word,epoch,step,out);
	}
	DataflowBarrierReadResult ReadRelease(unsigned worker,unsigned epoch,unsigned step,DataflowBarrierPacket& out) {
		assert(worker==core);return DataflowBarrierReadCommitted(f.release[core].payload.word,f.release[core].commit.word,epoch,step,out);
	}
};
struct Execution {
	unsigned core,routes[2][Banks]{},fetches=0,executes=0,resets=0;
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
		executes++;if (int(sequence-1)==failExecute) return false;
		status=int(sequence-1)==workerFailure?77:0;detail=status?123:0;return true;
	}
	bool ResetQueues() { return int(resets++)!=failReset; }
};
using Graph=DataflowDeviceGraph<Execution,Transport>;
struct Fleet {
	Fabric fabric;
	std::array<std::unique_ptr<Execution>,DataflowBarrierWorkers> execution;
	std::array<std::unique_ptr<Transport>,DataflowBarrierWorkers> transport;
	std::array<std::unique_ptr<Graph>,DataflowBarrierWorkers> graph;
	explicit Fleet(unsigned budget=8) {
		for (unsigned i=0;i<DataflowBarrierWorkers;i++) {
			execution[i]=std::make_unique<Execution>(i);
			transport[i]=std::make_unique<Transport>(Transport{fabric,i});
			graph[i]=std::make_unique<Graph>(*execution[i],*transport[i]);
			assert(graph[i]->Start(execution[i]->binding,i,budget));
		}
	}
	void Run(unsigned rounds=10) {
		for (unsigned round=0;round<rounds;round++)
			for (auto& worker:graph) worker->Tick();
	}
};
int main() {
	Fleet ok;ok.Run();
	for (unsigned i=0;i<DataflowBarrierWorkers;i++) {
		assert(ok.graph[i]->State()==DataflowGraphLocalComplete && ok.graph[i]->CompletedSteps()==2);
		assert(ok.execution[i]->fetches==2 && ok.execution[i]->executes==2 && ok.execution[i]->resets==1);
	}
	Fleet fetch;fetch.execution[7]->failFetch=0;fetch.Run();
	assert(fetch.graph[7]->Fault()==DataflowGraphFetchFault && fetch.execution[7]->executes==0);
	for (auto& g:fetch.graph) assert(g->State()==DataflowGraphRetain);
	Fleet authority;authority.execution[8]->failAuthority=0;authority.Run();
	assert(authority.graph[8]->Fault()==DataflowGraphAuthorityFault && authority.execution[8]->executes==0);
	Fleet uncertain;uncertain.execution[9]->failExecute=0;uncertain.Run();
	assert(uncertain.graph[9]->Fault()==DataflowGraphExecutionUncertain);
	Fleet failure;failure.execution[5]->workerFailure=0;failure.Run();
	for (auto& g:failure.graph) assert(g->Fault()==DataflowGraphWorkerFault && g->FailureCore()==5 && g->FailureStatus()==77);
	Fleet partial;partial.fabric.failRelease=12;partial.Run();
	assert(partial.graph[0]->Fault()==DataflowGraphBarrierFault);
	for (auto& g:partial.graph) assert(g->State()==DataflowGraphRetain);
	Fleet reset;reset.execution[7]->failReset=0;reset.Run();
	assert(reset.graph[7]->Fault()==DataflowGraphQueueFault && reset.execution[7]->fetches==1);
	assert(!reset.graph[7]->Start(reset.execution[7]->binding,7,8));
	puts("PASS device graph: 120 simulated workers, checked streams, barriers, retention and bounded deadlines; NO HARDWARE");
}
