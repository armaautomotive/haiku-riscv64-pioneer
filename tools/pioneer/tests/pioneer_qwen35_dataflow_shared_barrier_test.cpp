/* SPDX-License-Identifier: MIT */
#define PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS 1
#include "../PioneerQwen35DataflowDeviceBarrier.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <memory>
using namespace PioneerQwen35;

struct Slot { DataflowBarrierPacket payload{},marker{}; };
struct Fabric {
	Slot arrival[DataflowBarrierWorkers],release[DataflowBarrierWorkers];
	unsigned writes=0,payloadWrites=0;int failAt=-1;
};
struct IO {
	Fabric& fabric;unsigned core;
	bool Send(Slot& slot,const DataflowBarrierPacket& p,const DataflowBarrierPacket& c) {
		return DataflowBarrierSendShared(p,c,[&](bool marker,const DataflowBarrierPacket& packet) {
			const unsigned write=fabric.writes++;
			if (int(write)==fabric.failAt) return false;
			if (marker) slot.marker=packet;
			else { fabric.payloadWrites++;slot.payload=packet; }
			return true;
		});
	}
	bool SendArrival(unsigned worker,const DataflowBarrierPacket& p,const DataflowBarrierPacket& c) {
		assert(worker==core && core);return Send(fabric.arrival[worker],p,c);
	}
	bool SendRelease(unsigned worker,const DataflowBarrierPacket& p,const DataflowBarrierPacket& c) {
		assert(DataflowBarrierReleaseEdge(core,worker));return Send(fabric.release[worker],p,c);
	}
	DataflowBarrierReadResult ReadArrival(unsigned worker,unsigned epoch,unsigned step,DataflowBarrierPacket& p) {
		assert(DataflowBarrierReleaseEdge(core,worker));auto& slot=fabric.arrival[worker];
		return DataflowBarrierReadShared(slot.payload.word,slot.marker.word,epoch,step,worker,p);
	}
	DataflowBarrierReadResult ReadRelease(unsigned worker,unsigned epoch,unsigned step,DataflowBarrierPacket& p) {
		assert(worker==core);auto& slot=fabric.release[worker];
		return DataflowBarrierReadShared(slot.payload.word,slot.marker.word,epoch,step,worker,p);
	}
};
using Barrier=DataflowDeviceBarrier<IO>;
struct Fleet {
	Fabric fabric;
	std::array<std::unique_ptr<IO>,DataflowBarrierWorkers> io;
	std::array<std::unique_ptr<Barrier>,DataflowBarrierWorkers> barrier;
	Fleet() { for (unsigned i=0;i<DataflowBarrierWorkers;i++) {
		io[i]=std::make_unique<IO>(IO{fabric,i});barrier[i]=std::make_unique<Barrier>(*io[i]);
	} }
	void Start(unsigned epoch,unsigned step,unsigned failedCore=DataflowBarrierWorkers) {
		for (unsigned i=0;i<DataflowBarrierWorkers;i++)
			assert(barrier[i]->Start(i,epoch,step,i==failedCore?7:0,0));
	}
	void Arrive() {
		for (unsigned n=DataflowBarrierWorkers;n>1;n--)
			assert(barrier[n-1]->Poll()==DataflowBarrierPending);
	}
};

int main()
{
	// A marker can arrive in any order. An old or partial marker must never
	// authenticate a new step, regardless of the stale payload contents.
	const auto old=DataflowBarrierCommit(17,1),fresh=DataflowBarrierCommit(17,2);
	Slot slot{};slot.payload=DataflowBarrierPayload(17,1,1,9,1);
	for (unsigned mask=0;mask<(1U<<16);mask++) {
		bool allChanged=true;
		for (unsigned i=0;i<16;i++) {
			slot.marker.word[i]=(mask&(1U<<i))?fresh.word[i]:old.word[i];
			if (old.word[i]!=fresh.word[i] && !(mask&(1U<<i))) allChanged=false;
		}
		DataflowBarrierPacket out{};
		const auto result=DataflowBarrierReadShared(slot.payload.word,slot.marker.word,17,2,1,out);
		assert(result==(allChanged?DataflowBarrierReady:DataflowBarrierWaiting));
		if (allChanged) assert(DataflowBarrierPayloadValid(out,17,2,1) && !out.word[5]);
	}
	const auto failure=DataflowBarrierPayload(19,3,1,7,1);
	const auto commit=DataflowBarrierCommit(19,3);
	Fabric f;IO sender{f,1};
	assert(sender.Send(f.arrival[1],failure,commit) && f.writes==2 && f.payloadWrites==1);
	DataflowBarrierPacket out{};
	assert(DataflowBarrierReadShared(f.arrival[1].payload.word,f.arrival[1].marker.word,19,3,1,out)
		==DataflowBarrierReady && out.word[5]==7 && out.word[6]==1);
	f.arrival[1].payload.word[5]=0;
	assert(DataflowBarrierReadShared(f.arrival[1].payload.word,f.arrival[1].marker.word,19,3,1,out)
		==DataflowBarrierInvalid);
	Fabric failedSend;failedSend.failAt=0;IO faulty{failedSend,1};
	assert(!faulty.Send(failedSend.arrival[1],failure,commit) && failedSend.writes==1);
	auto invalid=DataflowBarrierPayload(19,3,1,0,0);auto badCommit=commit;badCommit.word[7]^=1;
	assert(!faulty.Send(failedSend.arrival[1],invalid,badCommit) && failedSend.writes==1);
	Fabric failedMarker;failedMarker.failAt=1;IO partial{failedMarker,1};
	assert(!partial.Send(failedMarker.arrival[1],failure,commit) && failedMarker.writes==2);
	assert(DataflowBarrierReadShared(failedMarker.arrival[1].payload.word,
		failedMarker.arrival[1].marker.word,19,3,1,out)==DataflowBarrierWaiting);
	assert(DataflowBarrierReadShared(slot.payload.word,slot.marker.word,17,
		DataflowBarrierFailureBit|2,1,out)==DataflowBarrierInvalid);

	Fleet healthy;
	for (unsigned step=1;step<=548;step++) {
		healthy.Start(31,step);healthy.Arrive();
		assert(healthy.barrier[0]->Poll()==DataflowBarrierPassed);
		for (unsigned i=1;i<DataflowBarrierWorkers;i++)
			assert(healthy.barrier[i]->Poll()==DataflowBarrierPassed);
		for (auto& barrier:healthy.barrier) assert(barrier->NextStep());
	}
	assert(healthy.fabric.writes==548*2*119 && !healthy.fabric.payloadWrites);
	Fleet failed;failed.Start(32,1,89);failed.Arrive();
	assert(failed.barrier[0]->Poll()==DataflowBarrierFailed);
	for (unsigned i=1;i<DataflowBarrierWorkers;i++)
		assert(failed.barrier[i]->Poll()==DataflowBarrierFailed && failed.barrier[i]->FailureCore()==89);
	assert(failed.fabric.payloadWrites>0 && failed.fabric.writes>2*119);
	std::puts("PASS shared barrier: single-write success, 120 workers, 548 steps, torn markers and failure propagation; SIMULATED ONLY");
}
