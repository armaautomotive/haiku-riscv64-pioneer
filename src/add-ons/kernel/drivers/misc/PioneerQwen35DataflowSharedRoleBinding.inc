/* SPDX-License-Identifier: MIT */
// Actual Haiku session/owner/mapping composition for the non-loadable shared
// role witness. The borrowed scope currently denies all physical proofs,
// including role release: this is not an executable service.
#if !defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS) || !defined(PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY) || !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || !defined(PIONEER_QWEN35_DEVICE_DISPATCH)
#error Shared role binding is a non-loadable recovery witness only
#endif
#include "PioneerQwen35DataflowL2CpuResetProbe.h"
#include "PioneerQwen35DataflowHeldFleetSample.h"
class PioneerQwen35SharedRoleBinding {
    using Views=PioneerQwen35::DataflowRoleWritableViews<PioneerQwen35RoleHaikuMap>;
    using Adapter=PioneerQwen35RoleFleetPciIO<PioneerSessionIO,PioneerQwen35RoleBorrowedScopeWitness,Views>;
    PioneerStoreContext& fContext;
    PioneerQwen35RoleHaikuMap fPlatform;
    Views fViews;
    PioneerQwen35::DataflowRoleFleetRecoveryOwner fOwner;
    uint32_t* fCodeSnapshot=NULL;
    uint32_t* fRamSnapshot=NULL;
    bool fCaptureAttempted=false;
    bool fReleaseAuthorized=false;
    bool fReleaseAttempted[PioneerQwen35::DataflowBarrierWorkers]{};
    bool fReleaseConfirmed[PioneerQwen35::DataflowBarrierWorkers]{};
    bool fRunningImagesObserved[PioneerQwen35::DataflowBarrierWorkers]{};
    bool fReleaseHeld[PioneerQwen35::DataflowBarrierWorkers]{};
    bool fStopAttempted[PioneerQwen35::DataflowBarrierWorkers]{};
    uint32_t fStopToken[PioneerQwen35::DataflowBarrierWorkers]{};
    bool fComputeStopAttempted[PioneerQwen35::DataflowBarrierWorkers]{};
    bool fHoldAttempted[PioneerQwen35::DataflowBarrierWorkers]{};
    PioneerQwen35RoleBorrowedScopeWitness* Scope() {
        auto* scope=fContext.io.qwen35BorrowedRoleScope;
        return scope && scope->ProveExclusiveRewrite(fContext.io)?scope:NULL;
    }
    bool OpenForRecovery(PioneerQwen35RoleBorrowedScopeWitness& scope) {
        if (fViews.Opened()) return true;
        PioneerQwen35RoleRouteGuard<PioneerSessionIO,PioneerQwen35RoleBorrowedScopeWitness> guard(fContext.io,scope);
        if (fViews.HasMappings()) return fViews.ReopenForRecovery(guard);
        return fViews.Open(fContext.io.profile.bar0_host_start,fContext.io.profile.bar0_host_end,guard);
    }
    static PioneerQwen35::DataflowRoleProgramBounds RoleBounds() {
        using namespace PioneerQwen35;
        return {{sizeof(DataflowSharedReaderShimWords)/sizeof(uint32_t),
                 sizeof(DataflowSharedWriterShimWords)/sizeof(uint32_t)},
                {sizeof(DataflowSharedReaderCodeWords)/sizeof(uint32_t),
                 sizeof(DataflowSharedWriterCodeWords)/sizeof(uint32_t)},
                {DataflowSharedReaderParkAddress,DataflowSharedWriterParkAddress}};
    }
    static PioneerQwen35::DataflowRoleProgramImage RoleImage() {
        using namespace PioneerQwen35;
        return {RoleBounds(),{DataflowSharedReaderShimWords,DataflowSharedWriterShimWords},
            {DataflowSharedReaderCodeWords,DataflowSharedWriterCodeWords}};
    }
public:
    explicit PioneerQwen35SharedRoleBinding(PioneerStoreContext& context)
        :fContext(context),fViews(fPlatform) {
        fCodeSnapshot=(uint32_t*)malloc(sizeof(uint32_t)*PioneerQwen35::DataflowRoleFleetRecoveryOwner::CodeWords());
        fRamSnapshot=(uint32_t*)malloc(sizeof(uint32_t)*PioneerQwen35::DataflowRoleFleetRecoveryOwner::RamWords());
    }
    PioneerQwen35SharedRoleBinding(const PioneerQwen35SharedRoleBinding&)=delete;
    PioneerQwen35SharedRoleBinding& operator=(const PioneerQwen35SharedRoleBinding&)=delete;
    ~PioneerQwen35SharedRoleBinding() { free(fCodeSnapshot); free(fRamSnapshot); }
    bool Valid() const { return fCodeSnapshot && fRamSnapshot; }
    bool CanDestroy() const { return !fViews.HasMappings() && !fOwner.RestorationRequired(); }
    __attribute__((noinline)) bool CaptureAndStageHeld() {
        using namespace PioneerQwen35;
        if (fCaptureAttempted || !Valid()) return false;
        fCaptureAttempted=true;
        auto* scope=Scope();
        if (!scope || !scope->ProveRoleStatusInspectionAllowed(fContext.io)) return false;
        PioneerQwen35RoleRouteGuard<PioneerSessionIO,PioneerQwen35RoleBorrowedScopeWitness> route(fContext.io,*scope);
        uint32_t l2cpuReset=0; bool restoreFailed=false;
        const bool sampled=PioneerQwen35SampleL2CpuReset(fContext.io,*scope,route,l2cpuReset,restoreFailed);
        if (restoreFailed) sColdRecoveryRequired=true;
        if (!sampled || !PioneerQwen35L2CpuRiscsHeld(l2cpuReset)) return false;
        unsigned failedCore=DataflowBarrierWorkers;
        const bool heldSample=PioneerQwen35SampleHeldFleet(fContext.io,*scope,route,failedCore,restoreFailed);
        if (restoreFailed) sColdRecoveryRequired=true;
        if (!heldSample) return false;
        // These status samples do not establish the independent global
        // incoming-NoC/ownership proof, which remains false in this build.
        // No role mappings or role memory writes occur without that proof.
        if (!scope->ProveRoleFleetHeldAndQuiet(fContext.io,0) || !OpenForRecovery(*scope)) return false;
        Adapter io(fContext.io,*scope,fViews);
        if (!fOwner.CaptureAll(fCodeSnapshot,DataflowRoleFleetRecoveryOwner::CodeWords(),
                fRamSnapshot,DataflowRoleFleetRecoveryOwner::RamWords(),io)) return false;
        return fOwner.StageImages(DataflowSharedReaderCodeWords,
            sizeof(DataflowSharedReaderCodeWords)/sizeof(uint32_t),DataflowSharedWriterCodeWords,
            sizeof(DataflowSharedWriterCodeWords)/sizeof(uint32_t),DataflowSharedReaderShimWords,
            sizeof(DataflowSharedReaderShimWords)/sizeof(uint32_t),DataflowSharedWriterShimWords,
            sizeof(DataflowSharedWriterShimWords)/sizeof(uint32_t),io);
    }
    __attribute__((noinline)) bool ArmAndBeginRelease() {
        auto* scope=Scope();
        if (!scope || !fViews.Opened()) return false;
        Adapter io(fContext.io,*scope,fViews);
        if (!fOwner.PreparePossibleRelease(io) || !fOwner.BeginReleaseAttempt(io)) return false;
        fReleaseAuthorized=true; return true;
    }
    __attribute__((noinline)) bool FleetHeldAndQuiescent() {
        auto* scope=Scope();
        if (!scope || !fViews.Opened()) return false;
        Adapter io(fContext.io,*scope,fViews);
        for (unsigned core=0;core<PioneerQwen35::DataflowBarrierWorkers;core++)
            if (!io.AllRolesHeld(core)) return false;
        return true;
    }
    __attribute__((noinline)) bool StartCore(unsigned core) {
        auto* scope=Scope();
        if (!scope || !fReleaseAuthorized || core>=PioneerQwen35::DataflowBarrierWorkers
            || fReleaseAttempted[core] || !fViews.Opened()) return false;
        Adapter io(fContext.io,*scope,fViews);
        fReleaseConfirmed[core]=io.ReleaseCore(core,fReleaseAttempted[core]);
        return fReleaseConfirmed[core];
    }
    __attribute__((noinline)) bool CoreReady(unsigned core,const PioneerQwen35::DataflowWorkerSnapshot& snapshot) {
        auto* scope=Scope();
        if (!scope || core>=PioneerQwen35::DataflowBarrierWorkers || !fReleaseConfirmed[core]
            || !fReleaseAuthorized || !fViews.Opened()) return false;
        Adapter io(fContext.io,*scope,fViews);
        if (!fRunningImagesObserved[core]) {
            if (!io.InspectRunningRoleImages(core,RoleImage())) return false;
        }
        if (!io.RunningReady(core,snapshot,
            sizeof(PioneerQwen35::DataflowServiceTextWords)/sizeof(uint32_t),
            sizeof(PioneerQwen35::DataflowServiceDispatchWords)/sizeof(uint32_t))
            || !io.RunningRolePcs(core,RoleBounds(),false)) return false;
        fRunningImagesObserved[core]=true;
        return true;
    }
    __attribute__((noinline)) bool RuntimeStable(unsigned core) {
        auto* scope=Scope();
        if (!scope || core>=PioneerQwen35::DataflowBarrierWorkers || !fReleaseConfirmed[core]
            || !fReleaseAuthorized || !fViews.Opened()) return false;
        Adapter io(fContext.io,*scope,fViews);
        const unsigned textWords=sizeof(PioneerQwen35::DataflowServiceTextWords)/sizeof(uint32_t);
        const unsigned dispatchWords=sizeof(PioneerQwen35::DataflowServiceDispatchWords)/sizeof(uint32_t);
        const bool fault=scope->RetainedFaultAuditActive(fContext.io);
        return (fault?io.FaultRuntimeStable(core,textWords,dispatchWords)
            :io.RuntimeStable(core,textWords,dispatchWords)) && io.RunningRolePcs(core,RoleBounds(),fault);
    }
    __attribute__((noinline)) bool FaultCoreSettled(unsigned core,const PioneerQwen35::DataflowWorkerSnapshot& snapshot) {
        auto* scope=Scope();
        if (!scope || core>=PioneerQwen35::DataflowBarrierWorkers || !fReleaseConfirmed[core]
            || !fReleaseAuthorized || !fViews.Opened() || !scope->RetainedFaultAuditActive(fContext.io)) return false;
        Adapter io(fContext.io,*scope,fViews);
        return io.FaultSettled(core,snapshot,sizeof(PioneerQwen35::DataflowServiceTextWords)/sizeof(uint32_t),
            sizeof(PioneerQwen35::DataflowServiceDispatchWords)/sizeof(uint32_t))
            && io.RunningRolePcs(core,RoleBounds(),true);
    }
    __attribute__((noinline)) bool CoreIdleForWrite(unsigned core,unsigned address,unsigned bytes) {
        using namespace PioneerQwen35;
        auto* scope=Scope();
        if (!scope || core>=DataflowBarrierWorkers || !fReleaseConfirmed[core]
            || !fReleaseAuthorized || !fViews.Opened() || !DataflowPciRunningWriteRange(address,bytes)) return false;
        Adapter io(fContext.io,*scope,fViews);
        const unsigned acknowledgement=DataflowWorkerMailboxAddress+offsetof(DataflowWorkerMailbox,acknowledgedSequence);
        if (scope->RetainedFaultAuditActive(fContext.io))
            return address==acknowledgement && bytes==sizeof(uint32_t)
                && io.FaultCompletedForAcknowledgement(core,sizeof(DataflowServiceTextWords)/sizeof(uint32_t),
                    sizeof(DataflowServiceDispatchWords)/sizeof(uint32_t))
                && io.RunningRolePcs(core,RoleBounds(),true);
        if (address==acknowledgement && bytes==sizeof(uint32_t))
            return io.CompletedForAcknowledgement(core,sizeof(DataflowServiceTextWords)/sizeof(uint32_t),
                sizeof(DataflowServiceDispatchWords)/sizeof(uint32_t))
                && io.RunningRolePcs(core,RoleBounds(),false);
        DataflowWorkerSnapshot snapshot{};
        return fContext.io.ReadWorkerSnapshot(core,snapshot) && CoreReady(core,snapshot);
    }
    bool StopAndHoldAll(unsigned& stage,unsigned& core) {
        // The old stop path understands T0 alone. For released tiles publish
        // or observe one role stop request first; the real scope still denies
        // this until it can prove compute/queues settled. Never release saved
        // state until the subsequent parked-PC/NoC/hold audit exists.
        bool any=false;
        for (bool attempted:fReleaseAttempted) any|=attempted;
        if (any) {
            auto* scope=Scope();
            if (!scope || !fViews.Opened()) { stage=8;core=PioneerQwen35::DataflowBarrierWorkers;return false; }
            Adapter io(fContext.io,*scope,fViews);
            for (unsigned i=0;i<PioneerQwen35::DataflowBarrierWorkers;i++) {
                if (!fReleaseAttempted[i] || fReleaseConfirmed[i] || fStopAttempted[i]
                    || fComputeStopAttempted[i] || fHoldAttempted[i]) continue;
                if (fReleaseHeld[i]) {
                    if (!io.UnconfirmedReleaseHeld(i)) { stage=15;core=i;return false; }
                } else if (io.UnconfirmedReleaseHeld(i)) fReleaseHeld[i]=true;
            }
            for (unsigned i=0;i<PioneerQwen35::DataflowBarrierWorkers;i++) {
                if (!fReleaseAttempted[i] || fReleaseHeld[i]) continue;
                if (fHoldAttempted[i]) { if (!io.HoldObserved(i)) { stage=11;core=i;return false; } continue; }
                bool okay=fStopAttempted[i]?io.StopRequestObserved(i,fStopToken[i])
                    :io.RequestStop(i,false,fStopAttempted[i],fStopToken[i]);
                if (!okay && !fStopAttempted[i]) okay=io.RequestStop(i,true,fStopAttempted[i],fStopToken[i]);
                if (!okay) { stage=8;core=i;return false; }
            }
            for (unsigned i=0;i<PioneerQwen35::DataflowBarrierWorkers;i++) {
                if (!fReleaseAttempted[i] || fReleaseHeld[i]) continue;
                if (fHoldAttempted[i]) { if (!io.HoldObserved(i)) { stage=11;core=i;return false; } continue; }
                bool parked=false;
                for (unsigned poll=0;poll<65536;poll++) {
                    if (io.RolesStoppedAndParked(i,fStopToken[i],
                        PioneerQwen35::DataflowSharedReaderParkAddress,
                        PioneerQwen35::DataflowSharedWriterParkAddress)) { parked=true;break; }
                    fContext.io.Pause();
                }
                if (!parked) { stage=9;core=i;return false; }
            }
            for (unsigned i=0;i<PioneerQwen35::DataflowBarrierWorkers;i++) {
                if (!fReleaseAttempted[i] || fReleaseHeld[i]) continue;
                if (fHoldAttempted[i]) { if (!io.HoldObserved(i)) { stage=11;core=i;return false; } continue; }
                const bool okay=fComputeStopAttempted[i]?io.ComputeStopObserved(i)
                    :io.RequestComputeStop(i,fComputeStopAttempted[i]);
                if (!okay) { stage=10;core=i;return false; }
            }
            for (unsigned i=0;i<PioneerQwen35::DataflowBarrierWorkers;i++) {
                if (!fReleaseAttempted[i] || fReleaseHeld[i]) continue;
                if (fHoldAttempted[i]) { if (!io.HoldObserved(i)) { stage=11;core=i;return false; } continue; }
                bool parked=false;
                for (unsigned poll=0;poll<65536;poll++) {
                    if (io.ComputeStoppedAndParked(i,PioneerQwen35::DataflowServiceStoppedParkAddress))
                        { parked=true;break; }
                    fContext.io.Pause();
                }
                if (!parked) { stage=12;core=i;return false; }
            }
            // No reset is asserted until every started tile has both role
            // receipts and a stopped compute receipt/park proof. The scope
            // must independently prove the entire fleet and NoCs are quiet.
            for (unsigned i=0;i<PioneerQwen35::DataflowBarrierWorkers;i++) {
                if (!fReleaseAttempted[i] || fReleaseHeld[i]) continue;
                if (fHoldAttempted[i] ? !io.HoldObserved(i) : !io.HoldCore(i,fHoldAttempted[i]))
                    { stage=13;core=i;return false; }
            }
            for (unsigned i=0;i<PioneerQwen35::DataflowBarrierWorkers;i++) {
                if (fReleaseHeld[i] && !io.UnconfirmedReleaseHeld(i)) { stage=15;core=i;return false; }
                if (!io.AllRolesHeld(i)) { stage=14;core=i;return false; }
            }
            stage=0;core=PioneerQwen35::DataflowBarrierWorkers;return true;
        }
        return fContext.io.StopAndAuditAll(stage,core);
    }
    __attribute__((noinline)) bool RestoreRolesHeld() {
        if (fOwner.Untouched()) return true;
        auto* scope=Scope();
        if (!scope || !OpenForRecovery(*scope)) return false;
        Adapter io(fContext.io,*scope,fViews);
        if (fOwner.CoveredStateRestored()) return fOwner.VerifyAfterResidentRestore(io);
        return fOwner.RestoreAll(io);
    }
    __attribute__((noinline)) bool VerifyAfterResidentRestore() {
        if (fOwner.Untouched()) return true;
        auto* scope=Scope();
        if (!scope || !OpenForRecovery(*scope)) return false;
        Adapter io(fContext.io,*scope,fViews);
        return fOwner.CoveredStateRestored() && fOwner.VerifyAfterResidentRestore(io);
    }
    bool FinalizeBeforeResume() { return (fOwner.Untouched() || fOwner.CoveredStateRestored())
        ?fViews.Close():false; }
};
static_assert(alignof(PioneerQwen35SharedRoleBinding)<=alignof(void*),"shared role binding requires ordinary kernel allocation");

static PioneerQwen35SharedRoleBinding*
PioneerQ35CreateSharedRoleBinding(PioneerStoreContext& context)
{
    auto* binding=new(std::nothrow) PioneerQwen35SharedRoleBinding(context);
    if (binding && !binding->Valid()) { delete binding; binding=NULL; }
    return binding;
}
static bool
PioneerQ35DestroySharedRoleBinding(PioneerQwen35SharedRoleBinding* binding)
{
    if (!binding) return true;
    if (!binding->CanDestroy()) return false;
    delete binding; return true;
}
bool PioneerSessionIO::SharedCaptureAndStageHeld() {
    return qwen35DataflowContext && qwen35DataflowContext->qwen35SharedRoleBinding
        && qwen35DataflowContext->qwen35SharedRoleBinding->CaptureAndStageHeld();
}
bool PioneerSessionIO::SharedArmAndBeginRelease() {
    return qwen35DataflowContext && qwen35DataflowContext->qwen35SharedRoleBinding
        && qwen35DataflowContext->qwen35SharedRoleBinding->ArmAndBeginRelease();
}
bool PioneerSessionIO::SharedFleetHeldAndQuiescent() {
    return qwen35DataflowContext && qwen35DataflowContext->qwen35SharedRoleBinding
        && qwen35DataflowContext->qwen35SharedRoleBinding->FleetHeldAndQuiescent();
}
bool PioneerSessionIO::SharedStartCore(unsigned core) {
    return qwen35DataflowContext && qwen35DataflowContext->qwen35SharedRoleBinding
        && qwen35DataflowContext->qwen35SharedRoleBinding->StartCore(core);
}
bool PioneerSessionIO::SharedCoreReady(unsigned core,const PioneerQwen35::DataflowWorkerSnapshot& snapshot) {
    return qwen35DataflowContext && qwen35DataflowContext->qwen35SharedRoleBinding
        && qwen35DataflowContext->qwen35SharedRoleBinding->CoreReady(core,snapshot);
}
bool PioneerSessionIO::SharedRuntimeStable(unsigned core) {
    return qwen35DataflowContext && qwen35DataflowContext->qwen35SharedRoleBinding
        && qwen35DataflowContext->qwen35SharedRoleBinding->RuntimeStable(core);
}
bool PioneerSessionIO::SharedFaultCoreSettled(unsigned core,const PioneerQwen35::DataflowWorkerSnapshot& snapshot) {
    return qwen35DataflowContext && qwen35DataflowContext->qwen35SharedRoleBinding
        && qwen35DataflowContext->qwen35SharedRoleBinding->FaultCoreSettled(core,snapshot);
}
bool PioneerSessionIO::SharedCoreIdleForWrite(unsigned core,unsigned address,unsigned bytes) {
    return qwen35DataflowContext && qwen35DataflowContext->qwen35SharedRoleBinding
        && qwen35DataflowContext->qwen35SharedRoleBinding->CoreIdleForWrite(core,address,bytes);
}
bool PioneerSessionIO::SharedStopAndHoldAll(unsigned& stage,unsigned& core) {
    return qwen35DataflowContext && qwen35DataflowContext->qwen35SharedRoleBinding
        && qwen35DataflowContext->qwen35SharedRoleBinding->StopAndHoldAll(stage,core);
}
bool PioneerSessionIO::SharedRestoreRolesHeld() {
    return qwen35DataflowContext && qwen35DataflowContext->qwen35SharedRoleBinding
        && qwen35DataflowContext->qwen35SharedRoleBinding->RestoreRolesHeld();
}
bool PioneerSessionIO::SharedVerifyRolesAfterResidentRestore() {
    return qwen35DataflowContext && qwen35DataflowContext->qwen35SharedRoleBinding
        && qwen35DataflowContext->qwen35SharedRoleBinding->VerifyAfterResidentRestore();
}
bool PioneerSessionIO::SharedFinalizeBeforeResume() {
    return qwen35DataflowContext && qwen35DataflowContext->qwen35SharedRoleBinding
        && qwen35DataflowContext->qwen35SharedRoleBinding->FinalizeBeforeResume();
}
