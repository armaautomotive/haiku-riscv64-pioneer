/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_RESIDENT_TREE_AUDIT_H
#define PIONEER_QWEN35_DATAFLOW_RESIDENT_TREE_AUDIT_H
#ifndef PIONEER_QWEN35_TREE_SETTLEMENT
#error Root-only audit requires the tree-settlement witness
#endif
#include "PioneerQwen35DataflowResidentFleetAudit.h"

namespace PioneerQwen35 {
// The v2 candidate is meaningful only if IO proves that all installed worker
// images implement the settlement tree and that routes, firmware and external
// writers are excluded for its entire lifetime. This class does not supply
// that proof; a sampled root alone cannot establish it.
class DataflowResidentTreeAudit {
    enum Phase { Empty,Reading,Audited,PhaseRetained } phase=Empty;
    DataflowResidentFleetSample first{};
    DataflowResidentCommand request{};
    template<class IO> DataflowResidentFleetAuditResult Fail(IO& io) {
        phase=PhaseRetained;io.RetainResidentAudit();return DataflowResidentFleetAuditRetained;
    }
public:
    DataflowResidentTreeAudit()=default;
    DataflowResidentTreeAudit(const DataflowResidentTreeAudit&)=delete;
    DataflowResidentTreeAudit& operator=(const DataflowResidentTreeAudit&)=delete;
    bool Retained() const { return phase==PhaseRetained; }
    bool AuditedFor(const DataflowResidentCommand& command) const {
        return phase==Audited && DataflowResidentCommandValid(command) && !memcmp(&request,&command,sizeof(command));
    }
    template<class IO> DataflowResidentFleetAuditResult Audit(const DataflowResidentCommand& command,IO& io) {
        if (phase!=Empty || !DataflowResidentCommandValid(command) || command.trigger>DataflowResidentRunLimit
            || command.firstSequence>DataflowResidentRunLimit
            || command.count-1>DataflowResidentRunLimit-command.firstSequence) return DataflowResidentFleetAuditInvalid;
        if (!io.ResidentAuditAllowed(command) || !io.ResidentTreeAuditAllowed(command))
            return DataflowResidentFleetAuditPreflight;
        phase=Reading;request=command;
        if (!io.BeginResidentAudit(command) || !io.FenceResidentAudit(command)
            || !io.ResidentTreeAuditAllowed(command)) return Fail(io);
        for (unsigned pass=0;pass<2;pass++) {
            DataflowResidentFleetSample sample{},confirmed{};
            if (!io.ResidentAuditAllowed(command) || !io.ResidentTreeAuditAllowed(command)
                || !io.ReadStableResidentCore(0,sample)
                || !DataflowResidentFleetAudit::ValidSample(sample,0,command)
                || !DataflowResidentSenderReceiptMatches(sample.sender,0,command)
                || !io.AuditResidentCoreRuntime(0,command)
                || !io.ReadStableResidentCore(0,confirmed)
                || !DataflowResidentFleetAudit::SameSample(sample,confirmed)
                || !io.AuditResidentCoreRuntime(0,command) || !io.ResidentAuditAllowed(command)
                || !io.ResidentTreeAuditAllowed(command)) return Fail(io);
            if (!pass) memcpy(&first,&sample,sizeof(sample));
            else if (!DataflowResidentFleetAudit::SameSample(first,sample)) return Fail(io);
            if (!io.FenceResidentAudit(command) || !io.ResidentTreeAuditAllowed(command)) return Fail(io);
        }
        if (!io.EndResidentAudit(command) || !io.ResidentAuditAllowed(command)
            || !io.ResidentTreeAuditAllowed(command)) return Fail(io);
        phase=Audited;return DataflowResidentFleetAuditComplete;
    }
};
}
#endif
