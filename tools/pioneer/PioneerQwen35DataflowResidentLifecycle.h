/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_RESIDENT_LIFECYCLE_H
#define PIONEER_QWEN35_DATAFLOW_RESIDENT_LIFECYCLE_H
#include "PioneerQwen35DataflowResidentDispatch.h"

namespace PioneerQwen35 {
// Graph steps, dispatch markers, the two end barriers and the final gather
// must have disjoint tags in the shared barrier slots. These limits still
// permit hundreds of thousands of full Qwen35 decode graphs per session.
enum : unsigned {
    DataflowResidentRunLimit=0x0fffffffU,
    DataflowResidentCompleteTag=0x10000000U,
    DataflowResidentReadyTag=0x20000000U,
    DataflowResidentGatherTag=0x30000000U
};
enum DataflowResidentLifecycleResult {
    DataflowResidentLifecyclePending=0,DataflowResidentLifecycleRun=1,
    DataflowResidentLifecycleGathered=2,DataflowResidentLifecycleRetained=3
};

// Per-worker state for one resident graph. IO is the actual fixed-tree NoC
// transport plus two local mailbox/queue operations. The caller executes the
// graph after Start returns Run and must report its result through GraphDone.
// A candidate gather is exposed only after (1) every graph finished, (2) all
// queues were reset/locally marked ready, and (3) every descendant consumed
// and forwarded the final release. The final gather uses arrival slots, not
// another release: otherwise a new token could overwrite a release which a
// delayed child had not read. A gather is a candidate completion, not a
// host-visible completion receipt or authority to reuse/release the fleet.
// No failed or uncertain operation is retried. A received message cannot by
// itself prove that the sender observed a successful NoC completion; physical
// fault recovery and clean release still require independent qualification.
template<class IO,class BarrierIO=IO> class DataflowResidentLifecycle {
    enum Phase { Empty,Running,Completing,Readying,Gathering,Gathered,Retained } phase=Empty;
    IO& io;
    DataflowDeviceBarrier<BarrierIO> barrier;
    unsigned core=DataflowBarrierWorkers,epoch=0,trigger=0,pollLimit=0,polls=0,seen=0;
    bool gatherSent=false;
    DataflowResidentLifecycleResult Fail() { phase=Retained;return DataflowResidentLifecycleRetained; }
    bool BeginBarrier(unsigned tag) {
        polls=0;
        return barrier.Start(core,epoch,tag|trigger,0,0);
    }
    DataflowResidentLifecycleResult Gather() {
        bool all=true;
        const unsigned first=core*DataflowBarrierReleaseFanout+1;
        for (unsigned child=first;child<first+DataflowBarrierReleaseFanout
            && child<DataflowBarrierWorkers;child++) {
            const unsigned bit=1U<<(child-first);
            if (seen&bit) continue;
            DataflowBarrierPacket packet{};
            const auto state=io.ReadArrival(child,epoch,DataflowResidentGatherTag|trigger,packet);
            if (state==DataflowBarrierWaiting) { all=false;continue; }
            if (state!=DataflowBarrierReady
                || !DataflowBarrierPayloadValid(packet,epoch,DataflowResidentGatherTag|trigger,child)
                || packet.word[5] || packet.word[6]) return Fail();
            seen|=bit;
        }
        if (!all) return DataflowResidentLifecyclePending;
        if (core && !gatherSent) {
            gatherSent=true; // before a potentially delivered send
            const auto payload=DataflowBarrierPayload(epoch,DataflowResidentGatherTag|trigger,core,0,0);
            const auto commit=DataflowBarrierCommit(epoch,DataflowResidentGatherTag|trigger);
            if (!io.SendArrival(core,payload,commit)) return Fail();
        }
        phase=Gathered;return DataflowResidentLifecycleGathered;
    }
public:
    explicit DataflowResidentLifecycle(IO& transport):io(transport),barrier(static_cast<BarrierIO&>(transport)) {}
    DataflowResidentLifecycle(const DataflowResidentLifecycle&)=delete;
    DataflowResidentLifecycle& operator=(const DataflowResidentLifecycle&)=delete;
    DataflowResidentLifecycleResult Start(unsigned worker,const DataflowResidentCommand& command,
        const DataflowGraphRunDescriptor& installed,unsigned completed,unsigned priorEpoch,
        unsigned priorLastStep,DataflowGraphRunDescriptor& expanded) {
        if (phase!=Empty || worker>=DataflowBarrierWorkers || command.trigger>DataflowResidentRunLimit
            || command.firstSequence>DataflowResidentRunLimit
            || command.count-1>DataflowResidentRunLimit-command.firstSequence
            || !DataflowResidentCommandExpand(command,installed,worker,completed,priorEpoch,priorLastStep,expanded))
            return Fail();
        core=worker;epoch=command.epoch;trigger=command.trigger;pollLimit=command.pollLimit;
        // Both operations may have taken effect even when they report failure.
        // The containing owner must retain all workers on either failure.
        if (!io.ResidentBegin(core,command,expanded)) return Fail();
        DataflowResidentFanout fanout;
        if (!fanout.Send(core,command,io)) return Fail();
        phase=Running;return DataflowResidentLifecycleRun;
    }
    DataflowResidentLifecycleResult GraphDone(bool success) {
        if (phase!=Running || !success || !BeginBarrier(DataflowResidentCompleteTag)) return Fail();
        phase=Completing;return DataflowResidentLifecyclePending;
    }
    DataflowResidentLifecycleResult Tick() {
        if (phase==Gathered) return DataflowResidentLifecycleGathered;
        if (phase==Running) return DataflowResidentLifecycleRun;
        if (phase==Retained || phase==Empty) return DataflowResidentLifecycleRetained;
        if (++polls>pollLimit) return Fail();
        if (phase==Gathering) return Gather();
        const auto result=barrier.Poll();
        if (result==DataflowBarrierPending) return DataflowResidentLifecyclePending;
        if (result!=DataflowBarrierPassed || !barrier.NextStep()) return Fail();
        if (phase==Completing) {
            // This callback checks the local successful receipt and role
            // queues, acknowledges the worker, resets queues and records
            // readiness. It must never discard a failed graph.
            if (!io.ResidentResetAndReady(core,trigger) || !BeginBarrier(DataflowResidentReadyTag)) return Fail();
            phase=Readying;return DataflowResidentLifecyclePending;
        }
        if (phase!=Readying) return Fail();
        phase=Gathering;polls=0;return Gather();
    }
};
}
#endif
