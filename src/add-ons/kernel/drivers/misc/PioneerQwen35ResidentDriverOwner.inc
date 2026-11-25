/* SPDX-License-Identifier: MIT */
// Resident graph-owner binding for the non-loadable snapshot build. The
// physical runtime proof below deliberately denies; this is not authority to
// publish commands, access hardware or retire a retained run.
#if !defined(PIONEER_QWEN35_RESIDENT_SERVICE_WITNESS) || !defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) \
    || !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || defined(PIONEER_QWEN35_RECOVERY_QUALIFY)
#error Resident driver owner requires the non-loadable snapshot witness
#endif
#include "PioneerQwen35ResidentHostPci.h"
#include "PioneerQwen35ResidentFleetAuditPci.h"
#include "../../../../../tools/pioneer/PioneerQwen35DataflowResidentOwnedRunner.h"

class PioneerQ35ResidentDriverOwner {
    using Command=PioneerQwen35::DataflowResidentCommand;
    using Session=PioneerQwen35::DataflowResidentGraphSession;
    struct Handle;
    struct Scope {
        PioneerQ35ResidentDriverOwner& owner;
        PioneerQwen35::DataflowResidentFleetAudit& audit;
        Handle* handle=nullptr;
        bool retained=false;
        Scope(PioneerQ35ResidentDriverOwner& o,PioneerQwen35::DataflowResidentFleetAudit& a):owner(o),audit(a) {}
        bool ResidentSessionAllowed(PioneerSessionIO& io,const Command& c,unsigned operation) const;
        bool ResidentRootAllowed(PioneerSessionIO& io,const Command& c,unsigned operation) const;
        bool AuditResidentFleet(PioneerSessionIO& io,const Command& c);
        bool ResidentAuditAllowed(PioneerSessionIO& io,const Command& c) const;
        bool BeginResidentFleetAudit(PioneerSessionIO& io,const Command& c) const;
        bool ResidentAuditReadAllowed(PioneerSessionIO& io,const Command& c,unsigned core) const;
        bool AuditResidentCoreRuntime(PioneerSessionIO& io,const Command& c,unsigned core) const;
        bool FinishResidentFleetAudit(PioneerSessionIO& io,const Command& c) const;
        bool ResidentAuditRootAllowed(PioneerSessionIO& io,const Command& c) const;
        void RetainResidentAudit();
        void RetainResident();
    };
    using Host=PioneerQwen35ResidentHostPci<PioneerSessionIO,Scope>;
    using Run=PioneerQwen35::DataflowResidentSessionRun<Host>;
    struct Handle {
        PioneerQwen35::DataflowResidentFleetAudit audit;
        Scope scope;
        Host host;
        Run run;
        explicit Handle(PioneerQ35ResidentDriverOwner& owner)
            :scope(owner,audit),host(owner.context.io,scope),run(host) { scope.handle=this; }
    };
    PioneerStoreContext& context;
    PioneerQ35GraphOwner* graph;
    PioneerQ35StreamRecordingWitness& stream;
    Session session;
    PioneerQwen35::DataflowResidentOwnedRunner<Handle> runner;
    Handle* active=nullptr;
    bool bound=false,scopeRetained=false,reservationRetained=false;

    bool BaseSafe() const {
        auto* role=context.io.qwen35BorrowedRoleScope;
        return !scopeRetained && graph && context.qwen35GraphOwner==graph && context.qwen35DataflowService
            && context.qwen35DataflowMemory && context.qwen35DataflowAuthority && context.qwen35DataflowProgramScratch
            && context.qwen35DataflowRoutesAudited && context.io.qwen35DataflowContext==&context
            && context.q35PciReservationHeld && !context.q35PciReleaseAttempted && !sColdRecoveryRequired
            && !sReloadQuiesced && sReloadGeneration && sRuntimeOwner && sRuntimeOwner->magic==0x50313030
            && sRuntimeOwner->context==&context && sBankStorageOnly && context.bankStorageOnly
            && context.bytes==PioneerResidentBanks::Total && context.initialized && role
            && role->ProveExclusiveRewrite(context.io);
    }
    bool SessionSafe(const Session& requested,const PioneerQwen35::DataflowStreamPlan& plan,unsigned slot) const {
        return bound && &requested==&session && BaseSafe() && active && context.qwen35DataflowService
            && context.qwen35DataflowService->OwnsResidentRun(&session,active->run)
            && stream.OwnsResidentSession(&session,plan,slot) && !context.Unsafe(true);
    }
    bool HandleSafe(const Handle* handle,const Command& command) const {
        const auto* plan=stream.SealedPlan();
        return handle && active==handle && plan && SessionSafe(session,*plan,stream.SealedSlot())
            && PioneerQwen35::DataflowResidentCommandValid(command)
            && !memcmp(&command,&handle->run.command,sizeof(command));
    }
    // Proving this requires owned resident images, firmware/route exclusion,
    // both NoCs, all sender receipts and recoverable stop on this hardware.
    // Samples and the non-loadable build do not establish those facts.
    bool PhysicalRuntimeProof(PioneerSessionIO&,const Command&) const { return false; }

    struct Policy {
        PioneerQ35ResidentDriverOwner& owner;
        bool ResidentBindAllowed(const Session& requested,const PioneerQwen35::DataflowStreamPlan& plan,unsigned slot) {
            return &requested==&owner.session && !owner.bound && !owner.active && owner.BaseSafe()
                && owner.context.qwen35DataflowService->Ready() && !owner.context.Unsafe(true)
                && owner.stream.SealedMatches(plan,slot);
        }
        bool ReserveResidentSession(const Session& requested,const PioneerQwen35::DataflowStreamPlan& plan,unsigned slot) {
            if (!ResidentBindAllowed(requested,plan,slot)
                || !owner.stream.ReserveResidentSession(&requested,plan,slot)) return false;
            if (owner.context.qwen35DataflowService->BindResidentSession(&requested)) return true;
            if (!owner.stream.CancelResidentReservation(&requested)) {
                owner.reservationRetained=true;sColdRecoveryRequired=true;
            }
            return false;
        }
        bool ResidentSessionSafe(const Session& requested,const PioneerQwen35::DataflowStreamPlan& plan,unsigned slot) {
            return owner.SessionSafe(requested,plan,slot);
        }
        Handle* NewResidentRun() {
            if (owner.active) return nullptr;
            owner.active=PioneerQ35NewAligned<Handle>(owner);return owner.active;
        }
        PioneerQwen35::DataflowProjectionAuthority* NewResidentCandidate() {
            static_assert(alignof(PioneerQwen35::DataflowProjectionAuthority)<=alignof(void*),
                "authority requires ordinary kernel allocation");
            return new(std::nothrow) PioneerQwen35::DataflowProjectionAuthority;
        }
        void DeleteResidentRun(Handle* handle) {
            if (!handle || owner.active!=handle) { sColdRecoveryRequired=true;return; }
            owner.active=nullptr;PioneerQ35DeleteAligned(handle);
        }
        void DeleteResidentCandidate(PioneerQwen35::DataflowProjectionAuthority* authority) { delete authority; }
        bool ResidentCommitAllowed(const Session& requested,const Run& run) {
            return owner.active && &run==&owner.active->run && &requested==&owner.session
                && owner.HandleSafe(owner.active,run.command) && owner.PhysicalRuntimeProof(owner.context.io,run.command);
        }
        void RetainResidentSession(const Session& requested,const void* run) {
            if (!owner.active || run!=&owner.active->run || &requested!=&owner.session
                || !owner.context.qwen35DataflowService->RetainResidentRun(&requested,owner.active->run))
                sColdRecoveryRequired=true;
        }
        void RetainResidentOwnedRun(const Session&,Handle& handle,bool serviceRetained) {
            owner.scopeRetained=true;
            if (owner.active!=&handle || !serviceRetained) sColdRecoveryRequired=true;
        }
        void Pause() { owner.context.io.Pause(); }
    };
public:
    PioneerQ35ResidentDriverOwner(PioneerStoreContext& c,PioneerQ35GraphOwner* g,PioneerQ35StreamRecordingWitness& s)
        :context(c),graph(g),stream(s) {}
    bool Bind() {
        const auto* plan=stream.SealedPlan();
        if (bound || !plan || !context.qwen35DataflowService) return false;
        Policy policy{*this};
        bound=session.Bind(*plan,context.qwen35DataflowRoutes,stream.SealedSlot(),context.qwen35DataflowAuthority,policy);
        return bound;
    }
    bool Ready(unsigned slot) const {
        const auto* plan=stream.SealedPlan();
        return bound && !Retained() && plan && slot==stream.SealedSlot() && context.qwen35DataflowService
            && context.qwen35DataflowService->ResidentSessionIdle(&session) && stream.OwnsResidentSession(&session,*plan,slot);
    }
    bool Retained() const { return runner.Retained() || scopeRetained || reservationRetained; }
    bool CanDestroy() const {
        return !Retained() && !active && (!bound
            || (context.qwen35DataflowService && context.qwen35DataflowService->Closed()));
    }
    PioneerQwen35::DataflowResidentSessionResult RunToken(unsigned position,unsigned token,bool decode,unsigned polls,
        PioneerQwen35::DataflowArgmaxResult& output,unsigned& stage) {
        output={PioneerQwen35::Vocabulary,0,0,0};stage=PioneerQwen35::DataflowDecoderSequenceInvalidRequest;
        if (!Ready(stream.SealedSlot()) || !context.qwen35DataflowProgramScratch)
            return PioneerQwen35::DataflowResidentSessionInvalid;
        Policy policy{*this};
        return runner.Run(session,*context.qwen35DataflowService,context.qwen35DataflowAuthority,
            position,token,decode,polls,*context.qwen35DataflowProgramScratch,policy,output,stage);
    }
};

bool PioneerQ35ResidentDriverOwner::Scope::ResidentSessionAllowed(PioneerSessionIO& io,const Command& c,unsigned) const {
    return !retained && &io==&owner.context.io && owner.HandleSafe(handle,c) && owner.PhysicalRuntimeProof(io,c);
}
bool PioneerQ35ResidentDriverOwner::Scope::ResidentRootAllowed(PioneerSessionIO& io,const Command& c,unsigned operation) const {
    return ResidentSessionAllowed(io,c,operation) && io.runtimeCore==0 && io.DataflowPanelMappingStable();
}
bool PioneerQ35ResidentDriverOwner::Scope::AuditResidentFleet(PioneerSessionIO& io,const Command& c) {
    PioneerQwen35ResidentFleetAuditPci<PioneerSessionIO,Scope> pci(io,*this);
    return audit.Audit(c,pci)==PioneerQwen35::DataflowResidentFleetAuditComplete && audit.AuditedFor(c);
}
bool PioneerQ35ResidentDriverOwner::Scope::ResidentAuditAllowed(PioneerSessionIO& io,const Command& c) const {
    return ResidentSessionAllowed(io,c,Host::Audit);
}
bool PioneerQ35ResidentDriverOwner::Scope::BeginResidentFleetAudit(PioneerSessionIO& io,const Command& c) const {
    return ResidentAuditAllowed(io,c);
}
bool PioneerQ35ResidentDriverOwner::Scope::ResidentAuditReadAllowed(PioneerSessionIO& io,const Command& c,unsigned core) const {
    return ResidentAuditAllowed(io,c) && core<PioneerQwen35::DataflowCoreCount && io.runtimeCore==core
        && io.DataflowPanelMappingStable();
}
bool PioneerQ35ResidentDriverOwner::Scope::AuditResidentCoreRuntime(PioneerSessionIO& io,const Command& c,unsigned core) const {
    auto* role=io.qwen35BorrowedRoleScope;
    return ResidentAuditAllowed(io,c) && role && core<PioneerQwen35::DataflowCoreCount
        && role->ProveRoleRuntimeSafe(io,core) && ResidentAuditAllowed(io,c);
}
bool PioneerQ35ResidentDriverOwner::Scope::FinishResidentFleetAudit(PioneerSessionIO& io,const Command& c) const {
    return ResidentAuditAllowed(io,c);
}
bool PioneerQ35ResidentDriverOwner::Scope::ResidentAuditRootAllowed(PioneerSessionIO& io,const Command& c) const {
    return ResidentAuditReadAllowed(io,c,0);
}
void PioneerQ35ResidentDriverOwner::Scope::RetainResidentAudit() { retained=true;owner.scopeRetained=true; }
void PioneerQ35ResidentDriverOwner::Scope::RetainResident() { retained=true;owner.scopeRetained=true; }

static PioneerQ35ResidentDriverOwner* PioneerQ35ResidentDriverCreate(PioneerStoreContext& context,
    PioneerQ35GraphOwner* graph,PioneerQ35StreamRecordingWitness& stream) {
    return PioneerQ35NewAligned<PioneerQ35ResidentDriverOwner>(context,graph,stream);
}
static bool PioneerQ35ResidentDriverBind(PioneerQ35ResidentDriverOwner* owner) { return owner && owner->Bind(); }
static bool PioneerQ35ResidentDriverReady(const PioneerQ35ResidentDriverOwner* owner,unsigned slot) {
    return owner && owner->Ready(slot);
}
static bool PioneerQ35ResidentDriverRetained(const PioneerQ35ResidentDriverOwner* owner) {
    return owner && owner->Retained();
}
static bool PioneerQ35ResidentDriverDestroy(PioneerQ35ResidentDriverOwner*& owner) {
    if (!owner) return true;
    if (!owner->CanDestroy()) return false;
    PioneerQ35DeleteAligned(owner);owner=nullptr;return true;
}
static PioneerQwen35::DataflowGraphSessionResult PioneerQ35ResidentDriverRun(PioneerQ35ResidentDriverOwner* owner,
    unsigned position,unsigned token,bool decode,unsigned polls,PioneerQwen35::DataflowArgmaxResult& output,unsigned& stage) {
    using namespace PioneerQwen35;
    if (!owner) { output={Vocabulary,0,0,0};stage=DataflowDecoderSequenceInvalidRequest;return DataflowGraphSessionInvalid; }
    switch (owner->RunToken(position,token,decode,polls,output,stage)) {
        case DataflowResidentSessionInvalid: return DataflowGraphSessionInvalid;
        case DataflowResidentSessionPreflight: return DataflowGraphSessionPreflight;
        case DataflowResidentSessionRetained: return DataflowGraphSessionRetained;
        case DataflowResidentSessionComplete: return DataflowGraphSessionComplete;
    }
    return DataflowGraphSessionInvalid;
}
