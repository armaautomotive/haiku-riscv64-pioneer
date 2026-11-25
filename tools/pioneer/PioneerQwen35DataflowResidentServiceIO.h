/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_RESIDENT_SERVICE_IO_H
#define PIONEER_QWEN35_DATAFLOW_RESIDENT_SERVICE_IO_H
#if !defined(PIONEER_QWEN35_DEVICE_DISPATCH) || !defined(PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS)
#error Resident graph service requires the shared-role graph witness
#endif
#include "PioneerQwen35DataflowResidentWorker.h"
#include "PioneerQwen35DataflowResidentService.h"
#include "PioneerQwen35DataflowResidentReceipt.h"
#include "PioneerQwen35DataflowResidentSenderReceipt.h"
namespace PioneerQwen35 {
// Concrete device adapter. The containing worker loop owns startup, shutdown,
// model identity and the saved L1 template. This adapter does not establish a
// clean fleet, authorize inference or enable a driver entrypoint.
class DataflowResidentServiceIO : public DataflowResidentWorkerIO {
    DataflowWorkerMailbox& mailbox;
    DataflowQueueControl* controls;
    unsigned worker,failure=0;
public:
    DataflowResidentServiceIO(unsigned core,DataflowWorkerMailbox& box,
        const DataflowRoleMailbox& roles,DataflowQueueControl* queues)
        :DataflowResidentWorkerIO(core,box,roles,queues),mailbox(box),controls(queues),worker(core) {}
    DataflowBarrierReadResult ReadResidentRequest(unsigned completed,unsigned epoch,DataflowResidentCommand& out) {
        const auto* payload=reinterpret_cast<const volatile uint32_t*>(static_cast<uintptr_t>(DataflowBarrierReleasePayload));
        const auto* marker=reinterpret_cast<const volatile uint32_t*>(static_cast<uintptr_t>(DataflowBarrierReleaseCommit));
#ifdef PIONEER_QWEN35_TREE_SETTLEMENT
        if (!worker) return DataflowResidentRootCommandRead(payload,marker,completed,epoch,out);
#endif
        return DataflowResidentCommandRead(payload,marker,completed,epoch,out);
    }
    bool ExecuteResidentGraph(unsigned core,const DataflowGraphRunDescriptor& d) {
        if (core!=worker || !controls || !DataflowGraphRunValid(d,core)) return false;
        bool okay=qwen35_graph_run_descriptor(&d,&mailbox);
        for (unsigned i=0;i<DataflowCircularBufferCount;i++) {
            const unsigned current=DataflowQueueFailure(controls[i]);
            if (!failure && current) failure=current;
        }
        if (failure) okay=false;
        if (!okay && !failure) { DataflowSignalLocalFailure(DataflowFailureQueue);failure=DataflowFailureQueue; }
        return okay;
    }
    bool RecordResidentGraphResult(unsigned core,unsigned trigger,bool okay) {
        if (core!=worker || !trigger || (okay && failure)) return false;
        DataflowWorkerPublish(mailbox,trigger,okay?DataflowWorkerComplete:DataflowWorkerFailed,
            okay?0:(failure?failure:DataflowFailureQueue),okay?2:5);
        DataflowFence();return true;
    }
#ifdef PIONEER_QWEN35_TREE_SETTLEMENT
    DataflowBarrierReadResult ReadChildSettlement(unsigned child,unsigned epoch,unsigned trigger) {
        uint32_t first[4]{},second[4]{};
        if (!ReadResidentChildReceipt(child,first) || !ReadResidentChildReceipt(child,second))
            return DataflowBarrierInvalid;
        return DataflowResidentSenderReceiptReadResult(first,second,child,trigger,epoch);
    }
#endif
    bool PublishResidentSenderReceipt(unsigned core,const DataflowResidentCommand& command) {
        return core==worker && DataflowResidentSenderReceiptPublish(
            reinterpret_cast<volatile uint32_t*>(static_cast<uintptr_t>(DataflowResidentSenderReceiptAddress)),
            core,command.trigger,command.epoch);
    }
    bool PublishResidentCandidate(unsigned core,const DataflowResidentCommand& command) {
        if (core!=worker) return false;
        if (core) return true;
        if (command.count==DataflowGraphDecodeSteps) {
            DataflowArgmaxResult result{};
            if (!DataflowArgmaxReadResult(mailbox,command.firstSequence+command.count-1,result)) return false;
        }
        auto* record=reinterpret_cast<volatile uint32_t*>(static_cast<uintptr_t>(DataflowResidentCandidateAddress));
        return DataflowResidentCandidatePublish(record,command);
    }
};
}
#endif
