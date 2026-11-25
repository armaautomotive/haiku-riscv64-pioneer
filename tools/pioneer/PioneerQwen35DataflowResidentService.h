/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_RESIDENT_SERVICE_H
#define PIONEER_QWEN35_DATAFLOW_RESIDENT_SERVICE_H
#include "PioneerQwen35DataflowResidentLifecycle.h"

namespace PioneerQwen35 {
enum DataflowResidentServiceResult {
    DataflowResidentServiceIdle=0,DataflowResidentServicePending=1,
    DataflowResidentServiceCandidate=2,DataflowResidentServiceRetained=3
};
// One token on one resident worker. The caller owns the immutable installed
// descriptor and must keep it and the model/session authority alive. The
// root's candidate is not a clean fleet receipt: the host must independently
// resolve sender faults before consuming it, submitting another token or
// releasing the service. An actual graph executes only through IO's device
// graph entrypoint; this class contains no inference or CPU fallback.
template<class IO,class BarrierIO=IO> class DataflowResidentServiceRun {
    enum Phase { Waiting,Executing,Finishing,Candidate,Retained } phase=Waiting;
    IO& io;
    DataflowResidentLifecycle<IO,BarrierIO> lifecycle;
    const DataflowGraphRunDescriptor& installed;
    DataflowGraphRunDescriptor expanded{};
    DataflowResidentCommand command{};
    unsigned core,completed,priorEpoch,priorLastStep;
    DataflowResidentServiceResult Fail() { phase=Retained;return DataflowResidentServiceRetained; }
public:
    DataflowResidentServiceRun(IO& transport,unsigned worker,const DataflowGraphRunDescriptor& descriptor,
        unsigned lastCompleted,unsigned lastEpoch,unsigned lastStep)
        :io(transport),lifecycle(transport),installed(descriptor),core(worker),completed(lastCompleted),
         priorEpoch(lastEpoch),priorLastStep(lastStep) {}
    DataflowResidentServiceRun(const DataflowResidentServiceRun&)=delete;
    DataflowResidentServiceRun& operator=(const DataflowResidentServiceRun&)=delete;
    unsigned CompletedSequence() const { return phase==Candidate?command.trigger:completed; }
    unsigned CompletedEpoch() const { return phase==Candidate?command.epoch:priorEpoch; }
    bool CompletedHasOutput() const { return phase==Candidate && command.count==DataflowGraphDecodeSteps; }
    unsigned CompletedLastStep() const {
        return phase==Candidate?command.firstSequence+command.count-1:priorLastStep;
    }
    DataflowResidentServiceResult Tick() {
        if (phase==Candidate) return DataflowResidentServiceCandidate;
        if (phase==Retained) return DataflowResidentServiceRetained;
        if (phase==Waiting) {
            const auto received=io.ReadResidentRequest(completed,priorEpoch,command);
            if (received==DataflowBarrierWaiting) return DataflowResidentServiceIdle;
            if (received!=DataflowBarrierReady
                || lifecycle.Start(core,command,installed,completed,priorEpoch,priorLastStep,expanded)
                    !=DataflowResidentLifecycleRun) return Fail();
            phase=Executing;return DataflowResidentServicePending;
        }
        if (phase==Executing) {
            const bool okay=io.ExecuteResidentGraph(core,expanded);
            if (!io.RecordResidentGraphResult(core,command.trigger,okay) || !okay
                || lifecycle.GraphDone(true)!=DataflowResidentLifecyclePending) return Fail();
            phase=Finishing;return DataflowResidentServicePending;
        }
        const auto result=lifecycle.Tick();
        if (result==DataflowResidentLifecyclePending) return DataflowResidentServicePending;
        // A gather write may arrive before its sender observes a failure.
        // Publish a local marker only after this worker's send returned
        // success; the host separately audits all 120 markers before ACK.
        if (result!=DataflowResidentLifecycleGathered || !io.PublishResidentSenderReceipt(core,command)
            || !io.PublishResidentCandidate(core,command)) return Fail();
        phase=Candidate;return DataflowResidentServiceCandidate;
    }
};
}
#endif
