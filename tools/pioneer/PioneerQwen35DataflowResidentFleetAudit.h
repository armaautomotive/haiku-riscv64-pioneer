/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_RESIDENT_FLEET_AUDIT_H
#define PIONEER_QWEN35_DATAFLOW_RESIDENT_FLEET_AUDIT_H

#include "PioneerQwen35DataflowResidentReceipt.h"
#include "PioneerQwen35DataflowResidentLifecycle.h"
#include "PioneerQwen35DataflowRoleMailbox.h"
#include <string.h>

namespace PioneerQwen35 {
enum DataflowResidentFleetAuditResult { DataflowResidentFleetAuditInvalid=0,
    DataflowResidentFleetAuditPreflight=1,DataflowResidentFleetAuditRetained=2,
    DataflowResidentFleetAuditComplete=3 };
struct DataflowResidentFleetSample {
    DataflowWorkerMailbox worker{};
    DataflowRoleMailbox roles{};
    DataflowQueueControl queues[DataflowCircularBufferCount]{};
};

// A 120-core stable snapshot is deliberately stored in the retained owner,
// not on the kernel stack. Sampling cannot rule out late sender failures or
// external writes by itself. IO must independently prove the exact session,
// images, NoC routes, sender settlement and exclusion throughout both passes,
// and restore the root route before authorizing the host acknowledgement.
class DataflowResidentFleetAudit {
    enum Phase { Empty,Reading,Audited,PhaseRetained } phase=Empty;
    DataflowResidentFleetSample first[DataflowCoreCount]{};
    DataflowResidentCommand request{};
    unsigned failedCore=DataflowCoreCount;
    template<class IO> DataflowResidentFleetAuditResult Fail(IO& io) {
        phase=PhaseRetained;io.RetainResidentAudit();return DataflowResidentFleetAuditRetained;
    }
    static bool ValidSample(const DataflowResidentFleetSample& sample,unsigned core,
        const DataflowResidentCommand& command) {
        const auto& box=sample.worker;
        if (box.magic!=DataflowWorkerMailboxMagic || box.version!=DataflowWorkerMailboxVersion
            || box.logicalCore!=core || box.commandSequence!=command.trigger
            || box.acknowledgedSequence!=command.trigger || box.completedSequence!=command.trigger
            || box.state!=DataflowWorkerReady || box.failure || box.phase || box.shutdownSequence
            || !DataflowRoleMailboxIdle(sample.roles)) return false;
        for (const auto& queue:sample.queues)
            if (DataflowLoadAcquire(queue.produced) || DataflowLoadAcquire(queue.consumed)
                || DataflowQueueFailure(queue) || queue.reserved[0] || queue.reserved[1]) return false;
        if (!core && command.count==DataflowGraphDecodeSteps) {
            DataflowArgmaxResult result{};
            return DataflowArgmaxReadResult(box,command.firstSequence+command.count-1,result);
        }
        for (unsigned i=0;i<6;i++) if (box.reserved[i]) return false;
        return true;
    }
    static bool SameSample(const DataflowResidentFleetSample& a,const DataflowResidentFleetSample& b) {
        return !memcmp(&a.worker,&b.worker,sizeof(a.worker)) && !memcmp(&a.roles,&b.roles,sizeof(a.roles))
            && !memcmp(&a.queues,&b.queues,sizeof(a.queues));
    }
public:
    DataflowResidentFleetAudit()=default;
    DataflowResidentFleetAudit(const DataflowResidentFleetAudit&)=delete;
    DataflowResidentFleetAudit& operator=(const DataflowResidentFleetAudit&)=delete;
    bool Retained() const { return phase==PhaseRetained; }
    unsigned FailedCore() const { return failedCore; }
    bool AuditedFor(const DataflowResidentCommand& command) const {
        return phase==Audited && DataflowResidentCommandValid(command) && !memcmp(&request,&command,sizeof(command));
    }
    template<class IO> DataflowResidentFleetAuditResult Audit(const DataflowResidentCommand& command,IO& io) {
        if (phase!=Empty || !DataflowResidentCommandValid(command) || command.trigger>DataflowResidentRunLimit
            || command.firstSequence>DataflowResidentRunLimit
            || command.count-1>DataflowResidentRunLimit-command.firstSequence) return DataflowResidentFleetAuditInvalid;
        if (!io.ResidentAuditAllowed(command)) return DataflowResidentFleetAuditPreflight;
        // Begin may already have changed the route even if it reports failure.
        // Consume this one-shot audit before entering that transport.
        phase=Reading;request=command;
        if (!io.BeginResidentAudit(command)) return Fail(io);
        if (!io.FenceResidentAudit(command)) return Fail(io);
        for (unsigned pass=0;pass<2;pass++) {
            for (unsigned core=0;core<DataflowCoreCount;core++) {
                failedCore=core;DataflowResidentFleetSample sample{};
                if (!io.ResidentAuditAllowed(command) || !io.AuditResidentCoreRuntime(core,command)
                    || !io.ReadStableResidentCore(core,sample) || !ValidSample(sample,core,command)
                    || !io.AuditResidentCoreRuntime(core,command) || !io.ResidentAuditAllowed(command)) return Fail(io);
                if (!pass) memcpy(&first[core],&sample,sizeof(sample));
                else if (!SameSample(first[core],sample)) return Fail(io);
            }
            if (!io.FenceResidentAudit(command) || !io.ResidentAuditAllowed(command)) return Fail(io);
        }
        failedCore=DataflowCoreCount;
        if (!io.EndResidentAudit(command) || !io.ResidentAuditAllowed(command)) return Fail(io);
        phase=Audited;return DataflowResidentFleetAuditComplete;
    }
};
}
#endif
