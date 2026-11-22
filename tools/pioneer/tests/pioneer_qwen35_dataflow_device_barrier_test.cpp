/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowDeviceBarrier.h"
#include "../PioneerQwen35DataflowBarrierSetup.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <memory>

using namespace PioneerQwen35;

struct Slot { DataflowBarrierPacket payload{},commit{}; };
struct FakeBarrierSetup {
	uint32_t words[(DataflowBarrierArrivalEnd-DataflowBarrierOutbox)/4];
	unsigned writes=0,reads=0,failWrite=0;bool held=true,corruptRead=false;
	FakeBarrierSetup() { for (auto& word:words) word=0xabcdef01; }
	bool CoreHeld(unsigned core) { return core<DataflowBarrierWorkers && held; }
	bool WriteCoreMemory(unsigned,unsigned address,const void* input,unsigned bytes) {
		assert(address>=DataflowBarrierOutbox && address+bytes<=DataflowBarrierArrivalEnd && bytes==64);
		if (++writes==failWrite) return false;
		const auto* src=static_cast<const uint32_t*>(input);
		for (unsigned i=0;i<16;i++) words[(address-DataflowBarrierOutbox)/4+i]=src[i];
		return true;
	}
	bool ReadCoreMemory(unsigned,unsigned address,void* output,unsigned bytes) {
		assert(address>=DataflowBarrierOutbox && address+bytes<=DataflowBarrierArrivalEnd && bytes==64);
		reads++;auto* dst=static_cast<uint32_t*>(output);
		for (unsigned i=0;i<16;i++) dst[i]=words[(address-DataflowBarrierOutbox)/4+i];
		if (corruptRead) dst[3]=1;
		return true;
	}
};
struct Fabric {
	Slot arrival[DataflowBarrierWorkers],release[DataflowBarrierWorkers];
	int failArrival=-1,failRelease=-1;
	unsigned arrivals=0,releases=0;
};
struct FakeBarrierIO {
	Fabric& fabric;unsigned core;
	bool SendArrival(unsigned worker,const DataflowBarrierPacket& payload,const DataflowBarrierPacket& commit) {
		assert(core==worker && core>0 && core<DataflowBarrierWorkers);
		fabric.arrivals++;if (int(core)==fabric.failArrival) return false;
		fabric.arrival[core]={payload,commit};return true;
	}
	bool SendRelease(unsigned worker,const DataflowBarrierPacket& payload,const DataflowBarrierPacket& commit) {
		assert(DataflowBarrierReleaseEdge(core,worker));
		fabric.releases++;if (int(worker)==fabric.failRelease) return false;
		fabric.release[worker]={payload,commit};return true;
	}
	static DataflowBarrierReadResult Read(Slot& slot,unsigned epoch,unsigned step,DataflowBarrierPacket& out) {
		return DataflowBarrierReadCommitted(slot.payload.word,slot.commit.word,epoch,step,out);
	}
	DataflowBarrierReadResult ReadArrival(unsigned worker,unsigned epoch,unsigned step,DataflowBarrierPacket& out) {
		assert(core==0 && worker>0 && worker<DataflowBarrierWorkers);
		return Read(fabric.arrival[worker],epoch,step,out);
	}
	DataflowBarrierReadResult ReadRelease(unsigned worker,unsigned epoch,unsigned step,DataflowBarrierPacket& out) {
		assert(core==worker && core>0 && core<DataflowBarrierWorkers);
		return Read(fabric.release[core],epoch,step,out);
	}
};

using Barrier=DataflowDeviceBarrier<FakeBarrierIO>;
struct Fleet {
	Fabric& fabric;
	std::array<std::unique_ptr<FakeBarrierIO>,DataflowBarrierWorkers> io;
	std::array<std::unique_ptr<Barrier>,DataflowBarrierWorkers> barrier;
	explicit Fleet(Fabric& f):fabric(f) {
		for (unsigned i=0;i<DataflowBarrierWorkers;i++) {
			io[i]=std::make_unique<FakeBarrierIO>(FakeBarrierIO{f,i});
			barrier[i]=std::make_unique<Barrier>(*io[i]);
		}
	}
};

int main()
{
	FakeBarrierSetup rootSetup;
	assert(DataflowBarrierPrepareHeldCore(rootSetup,0)
		&& rootSetup.writes==3+2*DataflowBarrierWorkers && rootSetup.reads==rootSetup.writes);
	assert(rootSetup.words[(DataflowBarrierArrivalEnd-DataflowBarrierOutbox)/4-1]==0
		&& rootSetup.words[(DataflowBarrierReleaseCommit+64-DataflowBarrierOutbox)/4]==0xabcdef01);
	FakeBarrierSetup peerSetup;
	assert(DataflowBarrierPrepareHeldCore(peerSetup,5) && peerSetup.writes==3
		&& peerSetup.words[(DataflowBarrierArrivalBase-DataflowBarrierOutbox)/4]==0xabcdef01);
	FakeBarrierSetup lostSetup;lostSetup.failWrite=2;
	assert(!DataflowBarrierPrepareHeldCore(lostSetup,0) && lostSetup.writes==2 && lostSetup.reads==1);
	FakeBarrierSetup badSetup;badSetup.corruptRead=true;
	assert(!DataflowBarrierPrepareHeldCore(badSetup,0) && badSetup.writes==1 && badSetup.reads==1);
	FakeBarrierSetup unheld;unheld.held=false;
	assert(!DataflowBarrierPrepareHeldCore(unheld,0) && !unheld.writes);
	for (unsigned worker=1;worker<DataflowBarrierWorkers;worker++) {
		assert(DataflowNocRouteMatchesWorker(DataflowBarrierWorkerRoute(worker),worker));
		for (unsigned phase=0;phase<2;phase++) {
			const unsigned arrival=DataflowBarrierArrivalAddress(worker,phase);
			const unsigned release=phase?DataflowBarrierReleaseCommit:DataflowBarrierReleasePayload;
			assert(DataflowBarrierPeerWriteAllowed(worker,0,DataflowBarrierWorkerRoute(0),arrival,DataflowBarrierOutbox,64));
			const unsigned parent=DataflowBarrierReleaseParent(worker);
			assert(DataflowBarrierReleaseEdge(parent,worker) && parent<worker);
			assert(DataflowBarrierPeerWriteAllowed(parent,worker,DataflowBarrierWorkerRoute(worker),release,DataflowBarrierOutbox,64));
			assert(!DataflowBarrierPeerWriteAllowed(worker,0,DataflowBarrierWorkerRoute(0),arrival+4,DataflowBarrierOutbox,64));
			assert(!DataflowBarrierPeerWriteAllowed(worker,0,DataflowBarrierWorkerRoute(0),arrival,DataflowBarrierOutbox,32));
			assert(!DataflowBarrierPeerWriteAllowed(worker,0,DataflowBarrierWorkerRoute(worker),arrival,DataflowBarrierOutbox,64));
		}
	}
	for (unsigned source=0;source<DataflowBarrierWorkers;source++)
		for (unsigned destination=1;destination<DataflowBarrierWorkers;destination++)
			assert(DataflowBarrierPeerWriteAllowed(source,destination,DataflowBarrierWorkerRoute(destination),
				DataflowBarrierReleasePayload,DataflowBarrierOutbox,64)
				==DataflowBarrierReleaseEdge(source,destination));
	assert(!DataflowBarrierPeerWriteAllowed(1,2,DataflowBarrierWorkerRoute(2),DataflowBarrierReleasePayload,DataflowBarrierOutbox,64));
	assert(DataflowBarrierArrivalAddress(120,false)==UINT32_MAX);
	Fabric fabric;Fleet success(fabric);
	assert(success.barrier[0]->Start(0,17,1,0,0));
	assert(success.barrier[0]->Poll()==DataflowBarrierPending);
	for (unsigned i=1;i<DataflowBarrierWorkers;i++) assert(success.barrier[i]->Start(i,17,1,0,0));
	assert(success.barrier[0]->Poll()==DataflowBarrierPassed);
	assert(fabric.releases==DataflowBarrierReleaseFanout);
	for (unsigned i=1;i<DataflowBarrierWorkers;i++) assert(success.barrier[i]->Poll()==DataflowBarrierPassed);
	assert(fabric.arrivals==119 && fabric.releases==119);
	for (unsigned i=0;i<DataflowBarrierWorkers;i++) {
		assert(success.barrier[i]->NextStep());
		assert(!success.barrier[i]->NextStep());
	}

	// Old markers from a prior step must never release a new one.
	Fleet next(fabric);assert(next.barrier[0]->Start(0,17,2,0,0));
	for (unsigned i=1;i<DataflowBarrierWorkers;i++) {
		assert(next.barrier[i]->Start(i,17,2,0,0));
		assert(next.barrier[i]->Poll()==DataflowBarrierPending);
	}
	assert(next.barrier[0]->Poll()==DataflowBarrierPassed);
	for (unsigned i=1;i<DataflowBarrierWorkers;i++) assert(next.barrier[i]->Poll()==DataflowBarrierPassed);

	Fabric failures;Fleet failed(failures);assert(failed.barrier[0]->Start(0,91,1,0,0));
	for (unsigned i=1;i<DataflowBarrierWorkers;i++)
		assert(failed.barrier[i]->Start(i,91,1,i==5?7U:i==89?9U:0U,i==5?123U:i==89?456U:0U));
	assert(failed.barrier[0]->Poll()==DataflowBarrierFailed
		&& failed.barrier[0]->FailureCore()==5 && failed.barrier[0]->Failure()==7);
	assert(!failed.barrier[0]->NextStep());
	for (unsigned i=1;i<DataflowBarrierWorkers;i++)
		assert(failed.barrier[i]->Poll()==DataflowBarrierFailed && failed.barrier[i]->FailureCore()==5);

	Fabric bad;Fleet corrupted(bad);assert(corrupted.barrier[0]->Start(0,31,1,0,0));
	for (unsigned i=1;i<DataflowBarrierWorkers;i++) assert(corrupted.barrier[i]->Start(i,31,1,0,0));
	bad.arrival[17].payload.word[4]=18;
	assert(corrupted.barrier[0]->Poll()==DataflowBarrierUnsafe && !bad.releases);
	assert(!corrupted.barrier[0]->NextStep());
	assert(corrupted.barrier[0]->Poll()==DataflowBarrierUnsafe);

	// A relay cannot forward a stale, torn or misaddressed parent release.
	Fabric relayFabric;Fleet relay(relayFabric);assert(relay.barrier[0]->Start(0,33,1,0,0));
	for (unsigned i=1;i<DataflowBarrierWorkers;i++) assert(relay.barrier[i]->Start(i,33,1,0,0));
	assert(relay.barrier[0]->Poll()==DataflowBarrierPassed && relayFabric.releases==4);
	relayFabric.release[1].commit.word[15]^=1;
	assert(relay.barrier[1]->Poll()==DataflowBarrierPending && relayFabric.releases==4);
	relayFabric.release[1].commit.word[15]^=1;relayFabric.release[1].payload.word[4]=2;
	assert(relay.barrier[1]->Poll()==DataflowBarrierUnsafe && relayFabric.releases==4);

	Fabric lost;Fleet partial(lost);assert(partial.barrier[0]->Start(0,41,1,0,0));
	for (unsigned i=1;i<DataflowBarrierWorkers;i++) assert(partial.barrier[i]->Start(i,41,1,0,0));
	lost.arrival[7].commit.word[15]^=1;
	assert(partial.barrier[0]->Poll()==DataflowBarrierPending && !lost.releases);
	partial.barrier[0]->Expire();assert(partial.barrier[0]->Poll()==DataflowBarrierUnsafe);

	Fabric sendFailure;sendFailure.failRelease=12;Fleet broken(sendFailure);
	assert(broken.barrier[0]->Start(0,51,1,0,0));
	for (unsigned i=1;i<DataflowBarrierWorkers;i++) assert(broken.barrier[i]->Start(i,51,1,0,0));
	assert(broken.barrier[0]->Poll()==DataflowBarrierPassed && sendFailure.releases==4);
	assert(broken.barrier[1]->Poll()==DataflowBarrierPassed && sendFailure.releases==8);
	assert(broken.barrier[2]->Poll()==DataflowBarrierUnsafe && sendFailure.releases==12);
	assert(broken.barrier[2]->Poll()==DataflowBarrierUnsafe && sendFailure.releases==12);
	assert(broken.barrier[12]->Poll()==DataflowBarrierPending);
	broken.barrier[12]->Expire();assert(broken.barrier[12]->Poll()==DataflowBarrierUnsafe);
	Fabric arrivalFailure;arrivalFailure.failArrival=7;Fleet unsent(arrivalFailure);
	assert(unsent.barrier[0]->Start(0,61,1,0,0));
	assert(!unsent.barrier[7]->Start(7,61,1,0,0));
	assert(unsent.barrier[7]->Poll()==DataflowBarrierUnsafe && arrivalFailure.arrivals==1);
	assert(!unsent.barrier[7]->Start(7,61,1,0,0) && arrivalFailure.arrivals==1);
	puts("PASS device fleet barrier: all 120, failures, stale/torn commits, timeout, no retry; SIMULATED ONLY");
}
