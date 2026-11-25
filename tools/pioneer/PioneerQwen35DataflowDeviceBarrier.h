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

#ifdef PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS
// A success needs no payload: the marker already identifies this barrier.
// Errors retain the ordered payload-then-marker protocol. The high step bit
// distinguishes an error marker; real graph steps are far below that bit.
enum : unsigned { DataflowBarrierFailureBit=0x80000000U };
template<class Send> inline bool DataflowBarrierSendShared(
	const DataflowBarrierPacket& payload,const DataflowBarrierPacket& commit,Send send)
{
	const unsigned epoch=payload.word[2],step=payload.word[3],worker=payload.word[4];
	if (!epoch || !step || (step&DataflowBarrierFailureBit)
		|| worker>=DataflowBarrierWorkers || !DataflowBarrierPayloadValid(payload,epoch,step,worker)) return false;
	const auto expected=DataflowBarrierCommit(epoch,step);
	for (unsigned i=0;i<16;i++) if (commit.word[i]!=expected.word[i]) return false;
	if (payload.word[5]) {
		const auto failed=DataflowBarrierCommit(epoch,step|DataflowBarrierFailureBit);
		return send(false,payload) && send(true,failed);
	}
	return send(true,commit);
}

inline DataflowBarrierReadResult DataflowBarrierReadShared(
	const volatile uint32_t* source,const volatile uint32_t* marker,
	unsigned epoch,unsigned step,unsigned worker,DataflowBarrierPacket& payload)
{
	if (!epoch || !step || (step&DataflowBarrierFailureBit) || worker>=DataflowBarrierWorkers)
		return DataflowBarrierInvalid;
	if (DataflowBarrierCommitMatches(marker,epoch,step)) {
		payload=DataflowBarrierPayload(epoch,step,worker,0,0);
		return DataflowBarrierCommitMatches(marker,epoch,step)?DataflowBarrierReady:DataflowBarrierInvalid;
	}
	const unsigned failedStep=step|DataflowBarrierFailureBit;
	if (!DataflowBarrierCommitMatches(marker,epoch,failedStep)) return DataflowBarrierWaiting;
	for (unsigned i=0;i<16;i++) payload.word[i]=DataflowLoadAcquire(source[i]);
	if (!DataflowBarrierCommitMatches(marker,epoch,failedStep) || !payload.word[5])
		return DataflowBarrierInvalid;
	return DataflowBarrierReady;
}
#endif

// A single worker owns an instance. The coordinator is worker 0 and runs the
// same compute as the other workers. Subtree receipts are aggregated upward;
// the coordinator decides only after every worker has published a receipt.
// Each worker may proceed after its
// own release and after forwarding that decision to its fixed children; some
// branches can advance while others are still receiving it. No host action is
// needed per step. An uncertain send/receipt latches Unsafe and forbids retry.
// A release failure can leave earlier recipients executing the next step;
// the entire graph then remains uncertain, even if a peer reported Passed.
// The caller must impose a bounded poll deadline and retain device ownership
// after timeout or uncertainty; this class never resets or frees hardware.
template<class IO> class DataflowDeviceBarrier {
	IO& io;
	unsigned core=DataflowBarrierWorkers,epoch=0,step=0;
	unsigned firstFailureCore=DataflowBarrierWorkers,firstFailure=0;
	bool active=false,unsafe=false,done=false,failed=false,arrivalSent=false;
	unsigned seen=0;
	__attribute__((noinline)) bool ReleaseChildren() {
		const unsigned first=core*DataflowBarrierReleaseFanout+1;
		for (unsigned worker=first;worker<first+DataflowBarrierReleaseFanout
			&& worker<DataflowBarrierWorkers;worker++) {
			const auto release=DataflowBarrierPayload(epoch,step,worker,
				failed?firstFailure:0,failed?firstFailureCore:0);
			if (!io.SendRelease(worker,release,DataflowBarrierCommit(epoch,step))) return false;
		}
		return true;
	}
public:
	explicit DataflowDeviceBarrier(IO& transport):io(transport) {}
	DataflowDeviceBarrier(const DataflowDeviceBarrier&)=delete;
	DataflowDeviceBarrier& operator=(const DataflowDeviceBarrier&)=delete;
	bool Start(unsigned worker,unsigned epochValue,unsigned stepValue,
		unsigned status,unsigned detail)
	{
		if (active || unsafe || done || worker>=DataflowBarrierWorkers
			|| !epochValue || !stepValue || (!status && detail)) return false;
#ifdef PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS
		if (stepValue&DataflowBarrierFailureBit) return false;
#endif
		core=worker;epoch=epochValue;step=stepValue;active=true;
		if (status) { firstFailureCore=core;firstFailure=status; }
		return true;
	}
	DataflowBarrierResult Poll()
	{
		if (unsafe) return DataflowBarrierUnsafe;
		if (done) return failed?DataflowBarrierFailed:DataflowBarrierPassed;
		if (!active) return DataflowBarrierUnsafe;
		if (!arrivalSent) {
			bool all=true;
			const unsigned first=core*DataflowBarrierReleaseFanout+1;
			for (unsigned child=first;child<first+DataflowBarrierReleaseFanout
				&& child<DataflowBarrierWorkers;child++) {
				const unsigned bit=1U<<(child-first);
				if (seen&bit) continue;
				DataflowBarrierPacket arrival;
				const auto result=io.ReadArrival(child,epoch,step,arrival);
				if (result==DataflowBarrierWaiting) { all=false;continue; }
				if (result!=DataflowBarrierReady || !DataflowBarrierPayloadValid(arrival,epoch,step,child)
					|| (arrival.word[5] && !DataflowBarrierSubtreeContains(child,arrival.word[6]))) {
					unsafe=true;return DataflowBarrierUnsafe;
				}
				seen|=bit;
				if (arrival.word[5] && arrival.word[6]<firstFailureCore) {
					firstFailureCore=arrival.word[6];firstFailure=arrival.word[5];
				}
			}
			if (!all) return DataflowBarrierPending;
			failed=firstFailureCore<DataflowBarrierWorkers;
			if (core==0) {
				if (!ReleaseChildren()) { unsafe=true;return DataflowBarrierUnsafe; }
				active=false;done=true;return failed?DataflowBarrierFailed:DataflowBarrierPassed;
			}
			const auto payload=DataflowBarrierPayload(epoch,step,core,failed?firstFailure:0,
				failed?firstFailureCore:0);
			if (!io.SendArrival(core,payload,DataflowBarrierCommit(epoch,step))) {
				unsafe=true;return DataflowBarrierUnsafe;
			}
			arrivalSent=true;
		}
		{
			DataflowBarrierPacket release;
			const auto result=io.ReadRelease(core,epoch,step,release);
			if (result==DataflowBarrierWaiting) return DataflowBarrierPending;
			if (result!=DataflowBarrierReady || !DataflowBarrierPayloadValid(release,epoch,step,core)
				|| (release.word[5] && release.word[6]>=DataflowBarrierWorkers)
				|| (failed && (!release.word[5] || release.word[6]>firstFailureCore
					|| (release.word[6]==firstFailureCore && release.word[5]!=firstFailure)))) {
				unsafe=true;return DataflowBarrierUnsafe;
			}
			failed=release.word[5]!=0;firstFailure=release.word[5];
			firstFailureCore=failed?release.word[6]:DataflowBarrierWorkers;
			if (!ReleaseChildren()) { unsafe=true;return DataflowBarrierUnsafe; }
			active=false;done=true;return failed?DataflowBarrierFailed:DataflowBarrierPassed;
		}
	}
	unsigned FailureCore() const { return firstFailureCore; }
	unsigned Failure() const { return firstFailure; }
	void Expire() { if (active && !done) unsafe=true; }
	// Reuse only a successful barrier. Failure or uncertainty keeps the graph
	// owned; it must not be made runnable by resetting this object.
	bool NextStep() {
		if (!done || failed || unsafe || active) return false;
		core=DataflowBarrierWorkers;epoch=step=firstFailure=0;
		firstFailureCore=DataflowBarrierWorkers;done=false;arrivalSent=false;seen=0;
		return true;
	}
};

// Concrete NoC1 transport. The shared-role build sends only a marker for a
// successful barrier, and an ordered payload then failure marker for errors.
// Repeating epoch/step keeps a torn marker from being accepted as complete.
// L1 must be initialized before start and each graph needs a fresh epoch.
class DataflowBlackholeBarrierIO {
	unsigned core;
	DataflowBlackholeNoc<1> noc;
	static DataflowBarrierReadResult Read(unsigned payloadAddress,unsigned commitAddress,
		unsigned epoch,unsigned step,unsigned worker,DataflowBarrierPacket& payload)
	{
		const auto* marker=reinterpret_cast<const volatile uint32_t*>(static_cast<uintptr_t>(commitAddress));
		const auto* source=reinterpret_cast<const volatile uint32_t*>(static_cast<uintptr_t>(payloadAddress));
#ifdef PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS
		return DataflowBarrierReadShared(source,marker,epoch,step,worker,payload);
#else
		(void)worker;
		return DataflowBarrierReadCommitted(source,marker,epoch,step,payload);
#endif
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
	// Resident graph requests carry data even on success, so unlike the
	// optimized graph barrier they always publish payload before commit.
	// The fixed parent/child route and existing two release slots are reused.
	bool SendResidentDispatch(unsigned source,unsigned worker,
		const DataflowBarrierPacket& payload,const DataflowBarrierPacket& commit) {
		return source==core && DataflowBarrierReleaseEdge(core,worker)
			&& Send(worker,DataflowBarrierReleasePayload,payload)
			&& Send(worker,DataflowBarrierReleaseCommit,commit);
	}
#ifdef PIONEER_QWEN35_TREE_SETTLEMENT
    bool ReadResidentChildReceipt(unsigned child,uint32_t (&words)[4]) {
        if (!DataflowBarrierReleaseEdge(core,child)) return false;
        auto* outbox=reinterpret_cast<volatile uint32_t*>(static_cast<uintptr_t>(DataflowBarrierOutbox));
        if (!noc.ReadBarrierReceipt(core,child,reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(DataflowBarrierOutbox))))
            return false;
        const unsigned offset=DataflowBarrierSenderReceiptOffset/4;
        for (unsigned i=0;i<4;i++) words[i]=DataflowLoadAcquire(outbox[offset+i]);
        return true;
    }
#endif
	bool SendArrival(unsigned worker,const DataflowBarrierPacket& payload,const DataflowBarrierPacket& commit)
	{
		const unsigned parent=DataflowBarrierReleaseParent(worker);
#ifdef PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS
		return worker==core && parent<DataflowBarrierWorkers
			&& DataflowBarrierSendShared(payload,commit,[&](bool marker,const DataflowBarrierPacket& p) {
				return Send(parent,DataflowBarrierArrivalAddress(worker,marker),p);
			});
#else
		return worker==core && parent<DataflowBarrierWorkers
			&& Send(parent,DataflowBarrierArrivalAddress(worker,false),payload)
			&& Send(parent,DataflowBarrierArrivalAddress(worker,true),commit);
#endif
	}
	bool SendRelease(unsigned worker,const DataflowBarrierPacket& payload,const DataflowBarrierPacket& commit)
	{
#ifdef PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS
		return DataflowBarrierReleaseEdge(core,worker)
			&& DataflowBarrierSendShared(payload,commit,[&](bool marker,const DataflowBarrierPacket& p) {
				return Send(worker,marker?DataflowBarrierReleaseCommit:DataflowBarrierReleasePayload,p);
			});
#else
		return DataflowBarrierReleaseEdge(core,worker)
			&& Send(worker,DataflowBarrierReleasePayload,payload)
			&& Send(worker,DataflowBarrierReleaseCommit,commit);
#endif
	}
	DataflowBarrierReadResult ReadArrival(unsigned worker,unsigned epoch,unsigned step,DataflowBarrierPacket& packet)
	{
		return DataflowBarrierReleaseEdge(core,worker)
			?Read(DataflowBarrierArrivalAddress(worker,false),DataflowBarrierArrivalAddress(worker,true),epoch,step,worker,packet)
			:DataflowBarrierInvalid;
	}
	DataflowBarrierReadResult ReadRelease(unsigned worker,unsigned epoch,unsigned step,DataflowBarrierPacket& packet)
	{
		return core==worker && core && core<DataflowBarrierWorkers
			?Read(DataflowBarrierReleasePayload,DataflowBarrierReleaseCommit,epoch,step,worker,packet)
			:DataflowBarrierInvalid;
	}
};
}
#endif
