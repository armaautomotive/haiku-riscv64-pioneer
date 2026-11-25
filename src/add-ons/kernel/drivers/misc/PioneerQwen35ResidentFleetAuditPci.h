/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_RESIDENT_FLEET_AUDIT_PCI_H
#define PIONEER_QWEN35_RESIDENT_FLEET_AUDIT_PCI_H
#if !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || !defined(PIONEER_QWEN35_DEVICE_DISPATCH) \
    || !defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS) || defined(PIONEER_QWEN35_RECOVERY_QUALIFY)
#error Resident fleet PCI audit is a non-loadable shared-role witness only
#endif
#include "../../../../../tools/pioneer/PioneerQwen35DataflowResidentFleetAudit.h"

// Actual session IO adapter. The scope must prove session/route authority,
// sender settlement, firmware exclusion and both NoC/role runtime conditions;
// this adapter only performs bounded reads and restores the root mapping.
// Scope predicates must not change the selected route or panel mapping.
// Route selection may have changed hardware even if it reports failure: the
// outer one-shot audit retains the session on any failed operation.
template<class Raw,class Scope> class PioneerQwen35ResidentFleetAuditPci {
    using Command=PioneerQwen35::DataflowResidentCommand;
    using Sample=PioneerQwen35::DataflowResidentFleetSample;
    Raw& io;Scope& scope;const Command* active=nullptr;
    bool Select(unsigned core) {
        return active && scope.ResidentAuditAllowed(io,*active) && io.SelectRuntimeCore(core)
            && io.MapQ38Panel() && io.DataflowPanelMappingStable()
            && scope.ResidentAuditReadAllowed(io,*active,core);
    }
    bool Read(unsigned core,unsigned address,void* output,unsigned bytes) {
        return active && scope.ResidentAuditReadAllowed(io,*active,core)
            && io.ReadCoreMemory(core,address,output,bytes) && io.DataflowPanelMappingStable()
            && scope.ResidentAuditReadAllowed(io,*active,core);
    }
    bool ReadSample(unsigned core,Sample& sample) {
        using namespace PioneerQwen35;
        return Read(core,DataflowWorkerMailboxAddress,&sample.worker,sizeof(sample.worker))
            && Read(core,DataflowRoleMailboxAddress,&sample.roles,sizeof(sample.roles))
            && Read(core,DataflowQueueStateAddress,&sample.queues,sizeof(sample.queues))
            && Read(core,DataflowResidentSenderReceiptAddress,&sample.sender,sizeof(sample.sender));
    }
public:
    PioneerQwen35ResidentFleetAuditPci(Raw& raw,Scope& owner):io(raw),scope(owner) {}
    PioneerQwen35ResidentFleetAuditPci(const PioneerQwen35ResidentFleetAuditPci&)=delete;
    PioneerQwen35ResidentFleetAuditPci& operator=(const PioneerQwen35ResidentFleetAuditPci&)=delete;
    bool ResidentAuditAllowed(const Command& command) { return scope.ResidentAuditAllowed(io,command); }
#ifdef PIONEER_QWEN35_TREE_SETTLEMENT
    bool ResidentTreeAuditAllowed(const Command& command) {
        return scope.ResidentTreeAuditAllowed(io,command);
    }
#endif
    bool BeginResidentAudit(const Command& command) {
        if (active || !ResidentAuditAllowed(command)) return false;
        active=&command;
        return scope.BeginResidentFleetAudit(io,command) && ResidentAuditAllowed(command);
    }
    bool FenceResidentAudit(const Command& command) {
        if (active!=&command || !Select(0)) return false;
        const auto magic=io.Q38PanelRead(PioneerQwen35::DataflowWorkerMailboxAddress);
        return magic==PioneerQwen35::DataflowWorkerMailboxMagic && io.DataflowPanelMappingStable()
            && scope.ResidentAuditReadAllowed(io,command,0) && ResidentAuditAllowed(command);
    }
    bool AuditResidentCoreRuntime(unsigned core,const Command& command) {
        return active==&command && core<PioneerQwen35::DataflowCoreCount
            && ResidentAuditAllowed(command) && scope.AuditResidentCoreRuntime(io,command,core)
            && ResidentAuditAllowed(command);
    }
    bool ReadStableResidentCore(unsigned core,Sample& output) {
        if (!active || core>=PioneerQwen35::DataflowCoreCount || !Select(core)) return false;
        const unsigned attempts=active->pollLimit<8?active->pollLimit:8;
        for (unsigned poll=0;poll<attempts;poll++) {
            Sample first{},second{};
            if (!ReadSample(core,first)) return false;
            io.Pause();
            if (!ReadSample(core,second)) return false;
            if (!memcmp(&first.worker,&second.worker,sizeof(first.worker))
                && !memcmp(&first.roles,&second.roles,sizeof(first.roles))
                && !memcmp(&first.queues,&second.queues,sizeof(first.queues))
                && !memcmp(&first.sender,&second.sender,sizeof(first.sender))) {
                memcpy(&output,&second,sizeof(output));return true;
            }
            io.Pause();
        }
        return false;
    }
    bool EndResidentAudit(const Command& command) {
        if (active!=&command || !ResidentAuditAllowed(command)
            || !scope.FinishResidentFleetAudit(io,command) || !FenceResidentAudit(command)) return false;
        // FenceResidentAudit leaves the exact root selected for host readback.
        return scope.ResidentAuditRootAllowed(io,command) && ResidentAuditAllowed(command);
    }
    void PauseResidentAudit() { io.Pause(); }
    void RetainResidentAudit() { scope.RetainResidentAudit(); }
};
#endif
