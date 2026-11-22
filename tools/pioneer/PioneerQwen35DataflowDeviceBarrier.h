/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_DEVICE_BARRIER_H
#define PIONEER_QWEN35_DATAFLOW_DEVICE_BARRIER_H

#include "PioneerQwen35DataflowBlackholeIO.h"

namespace PioneerQwen35 {

enum : uint32_t { DataflowBarrierMagic=0x31425235,DataflowBarrierVersion=1 };
struct alignas(64) DataflowBarrierPacket { uint32_t word[16]; };
static_assert(sizeof(DataflowBarrierPacket)==DataflowBarrierPacketBytes,
	"each NoC barrier packet occupies exactly one 64-byte transfer");

inline DataflowBarrierPacket DataflowBarrierPayload(unsigned epoch,unsigned step,
	unsigned worker,unsigned status,unsigned detail)
{
	DataflowBarrierPacket p;
	volatile uint32_t* words=p.word;
	for (unsigned i=0;i<16;i++) words[i]=0;
	p.word[0]=DataflowBarrierMagic;p.word[1]=DataflowBarrierVersion;
	p.word[2]=epoch;p.word[3]=step;p.word[4]=worker;
	p.word[5]=status;p.word[6]=detail;return p;
}

inline DataflowBarrierPacket DataflowBarrierCommit(unsigned epoch,unsigned step)
{
	DataflowBarrierPacket p;
	for (unsigned i=0;i<16;i+=4) {
		p.word[i]=epoch;p.word[i+1]=~epoch;
		p.word[i+2]=step;p.word[i+3]=~step;
	}
	return p;
}

inline bool DataflowBarrierPayloadValid(const DataflowBarrierPacket& p,
	unsigned epoch,unsigned step,unsigned worker)
{
	if (p.word[0]!=DataflowBarrierMagic || p.word[1]!=DataflowBarrierVersion
		|| p.word[2]!=epoch || p.word[3]!=step || p.word[4]!=worker
		|| (!p.word[5] && p.word[6])) return false;
	for (unsigned i=7;i<16;i++) if (p.word[i]) return false;
	return true;
}

enum DataflowBarrierReadResult { DataflowBarrierWaiting=0,DataflowBarrierReady=1,
	DataflowBarrierInvalid=2 };
enum DataflowBarrierResult { DataflowBarrierPending=0,DataflowBarrierPassed=1,
	DataflowBarrierFailed=2,DataflowBarrierUnsafe=3 };

inline bool DataflowBarrierCommitMatches(const volatile uint32_t* source,unsigned epoch,unsigned step)
{
	const auto expected=DataflowBarrierCommit(epoch,step);
	for (unsigned i=0;i<16;i++) if (DataflowLoadAcquire(source[i])!=expected.word[i]) return false;
	return true;
}

inline DataflowBarrierReadResult DataflowBarrierReadCommitted(
	const volatile uint32_t* source,const volatile uint32_t* marker,
	unsigned epoch,unsigned step,DataflowBarrierPacket& payload)
{
	if (!DataflowBarrierCommitMatches(marker,epoch,step)) return DataflowBarrierWaiting;
	for (unsigned i=0;i<16;i++) payload.word[i]=DataflowLoadAcquire(source[i]);
	return DataflowBarrierCommitMatches(marker,epoch,step)?DataflowBarrierReady:DataflowBarrierInvalid;
}

// A single worker owns an instance. The coordinator is worker 0 and runs the
// same compute as the other workers. A new step may start only after every
// worker has published a receipt and received its release. No host action is
// needed per step. An uncertain send/receipt latches Unsafe and forbids retry.
// A release failure can leave earlier recipients executing the next step;
// the entire graph then remains uncertain, even if a peer reported Passed.
// The caller must impose a bounded poll deadline and retain device ownership
// after timeout or uncertainty; this class never resets or frees hardware.
template<class IO> class DataflowDeviceBarrier {
	IO& io;
	unsigned core=DataflowBarrierWorkers,epoch=0,step=0;
	unsigned firstFailureCore=DataflowBarrierWorkers,firstFailure=0;
	bool active=false,unsafe=false,done=false,failed=false;
	bool seen[DataflowBarrierWorkers];
public:
	explicit DataflowDeviceBarrier(IO& transport):io(transport) {
		volatile bool* slots=seen;
		for (unsigned i=0;i<DataflowBarrierWorkers;i++) slots[i]=false;
	}
	DataflowDeviceBarrier(const DataflowDeviceBarrier&)=delete;
	DataflowDeviceBarrier& operator=(const DataflowDeviceBarrier&)=delete;
	bool Start(unsigned worker,unsigned epochValue,unsigned stepValue,
		unsigned status,unsigned detail)
	{
		if (active || unsafe || done || worker>=DataflowBarrierWorkers
			|| !epochValue || !stepValue || (!status && detail)) return false;
		core=worker;epoch=epochValue;step=stepValue;active=true;
		const auto payload=DataflowBarrierPayload(epoch,step,core,status,detail);
		if (core==0) {
			seen[0]=true;
			if (status) { firstFailureCore=0;firstFailure=status; }
			return true;
		}
		if (!io.SendArrival(core,payload,DataflowBarrierCommit(epoch,step))) {
			unsafe=true;return false;
		}
		return true;
	}
	DataflowBarrierResult Poll()
	{
		if (unsafe) return DataflowBarrierUnsafe;
		if (done) return failed?DataflowBarrierFailed:DataflowBarrierPassed;
		if (!active) return DataflowBarrierUnsafe;
		if (core!=0) {
			DataflowBarrierPacket release;
			const auto result=io.ReadRelease(core,epoch,step,release);
			if (result==DataflowBarrierWaiting) return DataflowBarrierPending;
			if (result!=DataflowBarrierReady || !DataflowBarrierPayloadValid(release,epoch,step,core)
				|| (release.word[5] && release.word[6]>=DataflowBarrierWorkers)) {
				unsafe=true;return DataflowBarrierUnsafe;
			}
			failed=release.word[5]!=0;firstFailure=release.word[5];
			firstFailureCore=failed?release.word[6]:DataflowBarrierWorkers;
			active=false;done=true;return failed?DataflowBarrierFailed:DataflowBarrierPassed;
		}
		bool all=true;
		for (unsigned worker=1;worker<DataflowBarrierWorkers;worker++) {
			if (seen[worker]) continue;
			DataflowBarrierPacket arrival;
			const auto result=io.ReadArrival(worker,epoch,step,arrival);
			if (result==DataflowBarrierWaiting) { all=false;continue; }
			if (result!=DataflowBarrierReady || !DataflowBarrierPayloadValid(arrival,epoch,step,worker)) {
				unsafe=true;return DataflowBarrierUnsafe;
			}
			seen[worker]=true;
			if (arrival.word[5] && worker<firstFailureCore) {
				firstFailureCore=worker;firstFailure=arrival.word[5];
			}
		}
		if (!all) return DataflowBarrierPending;
		failed=firstFailureCore<DataflowBarrierWorkers;
		for (unsigned worker=1;worker<DataflowBarrierWorkers;worker++) {
			const auto release=DataflowBarrierPayload(epoch,step,worker,
				failed?firstFailure:0,failed?firstFailureCore:0);
			if (!io.SendRelease(worker,release,DataflowBarrierCommit(epoch,step))) {
				unsafe=true;return DataflowBarrierUnsafe;
			}
		}
		active=false;done=true;return failed?DataflowBarrierFailed:DataflowBarrierPassed;
	}
	unsigned FailureCore() const { return firstFailureCore; }
	unsigned Failure() const { return firstFailure; }
	void Expire() { if (active && !done) unsafe=true; }
};

// Concrete NoC1 transport. Payload and commit use distinct slots and two
// ordered nonposted writes. Repeating epoch/step across the commit packet
// keeps a torn marker from being accepted as complete. L1 must be initialized
// before start and the host must provide a fresh nonzero epoch per session.
class DataflowBlackholeBarrierIO {
	unsigned core;
	DataflowBlackholeNoc<1> noc;
	static DataflowBarrierReadResult Read(unsigned payloadAddress,unsigned commitAddress,
		unsigned epoch,unsigned step,DataflowBarrierPacket& payload)
	{
		const auto* marker=reinterpret_cast<const volatile uint32_t*>(static_cast<uintptr_t>(commitAddress));
		const auto* source=reinterpret_cast<const volatile uint32_t*>(static_cast<uintptr_t>(payloadAddress));
		return DataflowBarrierReadCommitted(source,marker,epoch,step,payload);
	}
	bool Send(unsigned destination,unsigned address,const DataflowBarrierPacket& packet)
	{
		auto* outbox=reinterpret_cast<volatile uint32_t*>(static_cast<uintptr_t>(DataflowBarrierOutbox));
		for (unsigned i=0;i<16;i++) outbox[i]=packet.word[i];
		DataflowFence();
		return noc.WriteBarrierPacket(core,destination,address,
			reinterpret_cast<const uint8_t*>(static_cast<uintptr_t>(DataflowBarrierOutbox)));
	}
public:
	explicit DataflowBlackholeBarrierIO(unsigned worker):core(worker) {}
	bool SendArrival(unsigned worker,const DataflowBarrierPacket& payload,const DataflowBarrierPacket& commit)
	{
		return worker==core && worker && worker<DataflowBarrierWorkers
			&& Send(0,DataflowBarrierArrivalAddress(worker,false),payload)
			&& Send(0,DataflowBarrierArrivalAddress(worker,true),commit);
	}
	bool SendRelease(unsigned worker,const DataflowBarrierPacket& payload,const DataflowBarrierPacket& commit)
	{
		return core==0 && worker && worker<DataflowBarrierWorkers
			&& Send(worker,DataflowBarrierReleasePayload,payload)
			&& Send(worker,DataflowBarrierReleaseCommit,commit);
	}
	DataflowBarrierReadResult ReadArrival(unsigned worker,unsigned epoch,unsigned step,DataflowBarrierPacket& packet)
	{
		return core==0 && worker && worker<DataflowBarrierWorkers
			?Read(DataflowBarrierArrivalAddress(worker,false),DataflowBarrierArrivalAddress(worker,true),epoch,step,packet)
			:DataflowBarrierInvalid;
	}
	DataflowBarrierReadResult ReadRelease(unsigned worker,unsigned epoch,unsigned step,DataflowBarrierPacket& packet)
	{
		return core==worker && core && core<DataflowBarrierWorkers
			?Read(DataflowBarrierReleasePayload,DataflowBarrierReleaseCommit,epoch,step,packet)
			:DataflowBarrierInvalid;
	}
};
}
#endif
