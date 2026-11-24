/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_RESIDENT_WORKER_H
#define PIONEER_QWEN35_DATAFLOW_RESIDENT_WORKER_H
#if !defined(PIONEER_QWEN35_DEVICE_DISPATCH) || !defined(PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS)
#error Resident graph worker requires the shared-role graph witness
#endif
#include "PioneerQwen35DataflowBlackholeWorker.h"
#include "PioneerQwen35DataflowResidentLifecycle.h"
namespace PioneerQwen35 {
// These checks and transitions operate only on the local worker's mailbox and
// queues. They do not establish root completion, physical ownership or NoC
// health. Failures leave the worker/graph owned for the outer fault path.
inline bool DataflowResidentQueuesClean(const DataflowQueueControl* queues,bool zero) {
    if (!queues) return false;
    for (unsigned n=0;n<DataflowCircularBufferCount;n++) {
        const auto& q=queues[n];
        const unsigned produced=DataflowLoadAcquire(q.produced),consumed=DataflowLoadAcquire(q.consumed);
        if (produced!=consumed || (zero && produced) || DataflowQueueFailure(q)
            || q.reserved[0] || q.reserved[1]) return false;
    }
    return true;
}
inline bool DataflowResidentMailboxBegin(DataflowWorkerMailbox& box,const DataflowRoleMailbox& roles,
    const DataflowQueueControl* queues,unsigned core,const DataflowResidentCommand& command,
    const DataflowGraphRunDescriptor& descriptor,unsigned& previous) {
    const unsigned completed=DataflowLoadAcquire(box.completedSequence);
    if (box.magic!=DataflowWorkerMailboxMagic || box.version!=DataflowWorkerMailboxVersion
        || core>=DataflowCoreCount || box.logicalCore!=core || descriptor.core!=core
        || descriptor.triggerSequence!=command.trigger || !DataflowResidentCommandValid(command)
        || descriptor.ticket.epoch!=command.epoch || descriptor.ticket.firstSequence!=command.firstSequence
        || descriptor.ticket.count!=command.count || descriptor.pollLimit!=command.pollLimit
        || descriptor.inputSlot!=command.inputSlot || descriptor.token.token!=command.token
        || descriptor.token.position!=command.position
        || !DataflowGraphRunValid(descriptor,core) || DataflowLoadAcquire(box.commandSequence)!=completed
        || !DataflowWorkerCommandSequenceValid(completed,DataflowLoadAcquire(box.acknowledgedSequence),command.trigger)
        || DataflowLoadAcquire(box.shutdownSequence) || DataflowLoadAcquire(box.state)!=DataflowWorkerReady
        || DataflowLoadAcquire(box.failure) || DataflowLoadAcquire(box.phase)
        || !DataflowRoleMailboxIdle(roles) || !DataflowResidentQueuesClean(queues,true)) return false;
    for (unsigned i=0;i<6;i++) if (DataflowLoadAcquire(box.reserved[i])) return false;
    previous=completed;
    DataflowStoreRelease(box.commandSequence,command.trigger);
    DataflowWorkerPublish(box,completed,DataflowWorkerRunning,0,1);
    DataflowFence();return true;
}
inline bool DataflowResidentMailboxReady(DataflowWorkerMailbox& box,const DataflowRoleMailbox& roles,
    DataflowQueueControl* queues,unsigned core,unsigned trigger,unsigned previous,
    bool preserveOutput,unsigned resultSequence) {
    if (box.magic!=DataflowWorkerMailboxMagic || box.version!=DataflowWorkerMailboxVersion
        || core>=DataflowCoreCount || box.logicalCore!=core || !trigger || trigger<=previous
        || DataflowLoadAcquire(box.commandSequence)!=trigger || DataflowLoadAcquire(box.completedSequence)!=trigger
        || DataflowLoadAcquire(box.acknowledgedSequence)!=previous || DataflowLoadAcquire(box.shutdownSequence)
        || DataflowLoadAcquire(box.state)!=DataflowWorkerComplete || DataflowLoadAcquire(box.failure)
        || DataflowLoadAcquire(box.phase)!=2 || !DataflowRoleMailboxIdle(roles)
        || !DataflowResidentQueuesClean(queues,false)) return false;
    if (preserveOutput) {
        if (core || !resultSequence) return false;
    } else {
        for (unsigned i=0;i<6;i++) if (DataflowLoadAcquire(box.reserved[i])) return false;
    }
    if (!DataflowWorkerResetQueueRecords(queues)) return false;
    DataflowStoreRelease(box.acknowledgedSequence,trigger);
    if (!preserveOutput) DataflowWorkerClearReceipt(box);
    DataflowWorkerPublish(box,trigger,DataflowWorkerReady,0,0);
    DataflowFence();return true;
}
class DataflowResidentWorkerIO : public DataflowBlackholeBarrierIO {
    DataflowWorkerMailbox& box;
    const DataflowRoleMailbox& roles;
    DataflowQueueControl* queues;
    unsigned previous=0;
    bool begun=false,preserveOutput=false;
    unsigned resultSequence=0;
public:
    DataflowResidentWorkerIO(unsigned core,DataflowWorkerMailbox& mailbox,
        const DataflowRoleMailbox& roleMailbox,DataflowQueueControl* controls)
        :DataflowBlackholeBarrierIO(core),box(mailbox),roles(roleMailbox),queues(controls) {}
    bool ResidentBegin(unsigned core,const DataflowResidentCommand& command,const DataflowGraphRunDescriptor& d) {
        if (begun || !DataflowResidentMailboxBegin(box,roles,queues,core,command,d,previous)) return false;
        preserveOutput=core==0 && command.count==DataflowGraphDecodeSteps;
        resultSequence=command.firstSequence+command.count-1;
        begun=true;return true;
    }
    bool ResidentResetAndReady(unsigned core,unsigned trigger) {
        return begun && DataflowResidentMailboxReady(box,roles,queues,core,trigger,previous,preserveOutput,resultSequence);
    }
};
}
#endif
