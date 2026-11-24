/* SPDX-License-Identifier: MIT */
// Actual Haiku session/owner/mapping composition for the non-loadable shared
// role witness. Dedicated staging can use bounded observations; role release,
// physical ownership and recovery remain unqualified.
#if !defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS) || !defined(PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY) || !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || !defined(PIONEER_QWEN35_DEVICE_DISPATCH)
#error Shared role binding is a non-loadable recovery witness only
#endif
#include "PioneerQwen35DataflowFleetInspection.h"
#include "../../../../../tools/pioneer/PioneerQwen35DataflowRoleStartOrder.h"
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
#include "PioneerQwen35LocalRestoreImages.h"
#include "PioneerQwen35RoleServiceProbeImages.h"
#endif
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
#include "../../../../../tools/pioneer/PioneerQwen35ExclusiveRoleFleet.h"
#include "../../../../../tools/pioneer/PioneerQwen35ExclusiveRoleStop.h"
#endif
class PioneerQwen35SharedRoleBinding {
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_TEST
    friend struct PioneerQ35SharedRoleBindingTestAccess;
#endif
    using Views=PioneerQwen35::DataflowRoleWritableViews<PioneerQwen35RoleHaikuMap>;
    using Adapter=PioneerQwen35RoleFleetPciIO<PioneerSessionIO,PioneerQwen35RoleBorrowedScopeWitness,Views>;
    PioneerStoreContext& fContext;
    PioneerQwen35RoleHaikuMap fPlatform;
    Views fViews;
    PioneerQwen35::DataflowRoleFleetRecoveryOwner fOwner;
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
    PioneerQwen35::DataflowExclusiveRoleFleet fTakeover;
    PioneerQwen35::DataflowExclusiveRoleStop fTakeoverStop;
#endif
    PioneerQwen35::DataflowRoleStartOrder fStartOrder;
    uint32_t* fCodeSnapshot=NULL;
    uint32_t* fRamSnapshot=NULL;
    bool fCaptureAttempted=false;
    bool fReleaseAuthorized=false;
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
    bool fAutoRestoreOnStop=false;
    bool fAutoRestoreParked[PioneerQwen35::DataflowBarrierWorkers]{};
    bool fAutoCancelParked[PioneerQwen35::DataflowBarrierWorkers]{};
    bool fAutoRestoreRecorded[PioneerQwen35::DataflowBarrierWorkers]{};
    bool fCoreRestoreReleaseWrites[PioneerQwen35::DataflowBarrierWorkers]{};
    bool fCoreRestoreHoldWrites[PioneerQwen35::DataflowBarrierWorkers]{};
#endif
    bool fReleaseAttempted[PioneerQwen35::DataflowBarrierWorkers]{};
    bool fReleaseConfirmed[PioneerQwen35::DataflowBarrierWorkers]{};
    bool fRunningImagesObserved[PioneerQwen35::DataflowBarrierWorkers]{};
    bool fStackReleaseAttempted[PioneerQwen35::DataflowBarrierWorkers]{};
    bool fStackReleaseSubmitted[PioneerQwen35::DataflowBarrierWorkers]{};
    bool fStackReleaseWriting[PioneerQwen35::DataflowBarrierWorkers]{};
    bool fStackReleaseConfirmed[PioneerQwen35::DataflowBarrierWorkers]{};
#ifndef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
    bool fStackReleaseDrainedAbsent[PioneerQwen35::DataflowBarrierWorkers]{};
    bool fStackCancelAttempted[PioneerQwen35::DataflowBarrierWorkers]{};
    bool fReleaseHeld[PioneerQwen35::DataflowBarrierWorkers]{};
    bool fStopAttempted[PioneerQwen35::DataflowBarrierWorkers]{};
    uint32_t fStopToken[PioneerQwen35::DataflowBarrierWorkers]{};
    bool fComputeStopAttempted[PioneerQwen35::DataflowBarrierWorkers]{};
    bool fHoldAttempted[PioneerQwen35::DataflowBarrierWorkers]{};
#endif
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
    PioneerQwen35::DataflowRoleProgramBounds RoleBounds() const {
        using namespace PioneerQwen35;
#if defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS)
        if (fAutoRestoreOnStop) return {{sizeof(DataflowServiceProbeReaderShimWords)/sizeof(uint32_t),
                 sizeof(DataflowServiceProbeWriterShimWords)/sizeof(uint32_t)},
                {sizeof(DataflowServiceProbeReaderCodeWords)/sizeof(uint32_t),
                 sizeof(DataflowServiceProbeWriterCodeWords)/sizeof(uint32_t)},
                {DataflowServiceProbeReaderShimParkAddress,DataflowServiceProbeWriterShimParkAddress},
                {DataflowServiceProbeReaderRuntimeCodeWords,DataflowServiceProbeWriterRuntimeCodeWords},
                {DataflowServiceProbeReaderLoopAddress,DataflowServiceProbeWriterLoopAddress},
                {DataflowServiceProbeReaderLoopEnd,DataflowServiceProbeWriterLoopEnd}};
        return {{sizeof(DataflowSharedReaderShimWords)/sizeof(uint32_t),
                 sizeof(DataflowSharedWriterShimWords)/sizeof(uint32_t)},
                {sizeof(DataflowSharedReaderCodeWords)/sizeof(uint32_t),
                 sizeof(DataflowSharedWriterCodeWords)/sizeof(uint32_t)},
                {DataflowSharedReaderParkAddress,DataflowSharedWriterParkAddress},{},
                {DataflowSharedReaderLoopAddress,DataflowSharedWriterLoopAddress},
                {DataflowSharedReaderLoopEnd,DataflowSharedWriterLoopEnd}};
#elif defined(PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS)
        return {{sizeof(DataflowExclusiveReaderShimWords)/sizeof(uint32_t),
                 sizeof(DataflowExclusiveWriterShimWords)/sizeof(uint32_t)},
                {sizeof(DataflowSharedReaderCodeWords)/sizeof(uint32_t),
                 sizeof(DataflowSharedWriterCodeWords)/sizeof(uint32_t)},
                {DataflowExclusiveReaderParkAddress,DataflowExclusiveWriterParkAddress},{},
                {DataflowExclusiveReaderLoopAddress,DataflowExclusiveWriterLoopAddress},
                {DataflowExclusiveReaderLoopEnd,DataflowExclusiveWriterLoopEnd}};
#else
        return {{sizeof(DataflowSharedReaderShimWords)/sizeof(uint32_t),
                 sizeof(DataflowSharedWriterShimWords)/sizeof(uint32_t)},
                {sizeof(DataflowSharedReaderCodeWords)/sizeof(uint32_t),
                 sizeof(DataflowSharedWriterCodeWords)/sizeof(uint32_t)},
                {DataflowSharedReaderParkAddress,DataflowSharedWriterParkAddress},{},
                {DataflowSharedReaderLoopAddress,DataflowSharedWriterLoopAddress},
                {DataflowSharedReaderLoopEnd,DataflowSharedWriterLoopEnd}};
#endif
    }
    PioneerQwen35::DataflowRoleProgramImage RoleImage() const {
        using namespace PioneerQwen35;
#if defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS)
        if (fAutoRestoreOnStop)
            return {RoleBounds(),{DataflowServiceProbeReaderShimWords,DataflowServiceProbeWriterShimWords},
                {DataflowServiceProbeReaderCodeWords,DataflowServiceProbeWriterCodeWords}};
        return {RoleBounds(),{DataflowSharedReaderShimWords,DataflowSharedWriterShimWords},
            {DataflowSharedReaderCodeWords,DataflowSharedWriterCodeWords}};
#elif defined(PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS)
        return {RoleBounds(),{DataflowExclusiveReaderShimWords,DataflowExclusiveWriterShimWords},
            {DataflowSharedReaderCodeWords,DataflowSharedWriterCodeWords}};
#else
        return {RoleBounds(),{DataflowSharedReaderShimWords,DataflowSharedWriterShimWords},
            {DataflowSharedReaderCodeWords,DataflowSharedWriterCodeWords}};
#endif
    }
public:
    explicit PioneerQwen35SharedRoleBinding(PioneerStoreContext& context)
        :fContext(context),fViews(fPlatform) {
#ifndef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
        // This service releases T0 alongside B/NC. Its compute image uses
        // an L1 stack; preserve T0 private RAM across the release as well.
        (void)fOwner.SelectComputeLocalRamPreservation();
        fCodeSnapshot=(uint32_t*)malloc(sizeof(uint32_t)*PioneerQwen35::DataflowRoleFleetRecoveryOwner::CodeWords());
        fRamSnapshot=(uint32_t*)malloc(sizeof(uint32_t)*PioneerQwen35::DataflowRoleFleetRecoveryOwner::RamWords());
#endif
    }
    PioneerQwen35SharedRoleBinding(const PioneerQwen35SharedRoleBinding&)=delete;
    PioneerQwen35SharedRoleBinding& operator=(const PioneerQwen35SharedRoleBinding&)=delete;
    ~PioneerQwen35SharedRoleBinding() { free(fCodeSnapshot); free(fRamSnapshot); }
    bool Valid() const {
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
        return RoleImage().Valid();
#else
        return fCodeSnapshot && fRamSnapshot && fOwner.ComputeLocalRamPreservationSelected();
#endif
    }
    bool StacklessCopiesCaptured(unsigned core) const {
        return core<PioneerQwen35::DataflowBarrierWorkers && fReleaseConfirmed[core]
            && fOwner.StacklessCopiesCaptured(core);
    }
    bool CanDestroy() const { return !fViews.HasMappings() && !fOwner.RestorationRequired()
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
        && !fTakeover.ResetRequired()
#endif
        ; }
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
    // Identifies this binding's completed software ledger. It is not a
    // firmware, reset-continuity or live NoC ownership proof.
    bool ProjectionFleetRestoredLedger() const {
        if (!fAutoRestoreOnStop || !fCodeSnapshot || !fRamSnapshot || fViews.HasMappings()
            || !fOwner.AutoRestoredFleetLedgerComplete()) return false;
        for (unsigned core=0;core<PioneerQwen35::DataflowBarrierWorkers;core++)
            if (!fReleaseConfirmed[core] || !fStackReleaseConfirmed[core]
                || !fAutoRestoreParked[core] || fAutoCancelParked[core] || !fAutoRestoreRecorded[core])
                return false;
        return true;
    }
    // Explicit compile-only opt-in. The concrete scope and PCI adapter still
    // deny the lifetime proof and release authority, so these operations do
    // not authorize a physical restore or a fallback to host private RAM.
    bool SelectCoreLocalRestore() {
        return !fCaptureAttempted && fOwner.SelectCoreLocalRestoreMode();
    }
    // The qualified stop-to-restore images may be selected only before any
    // capture. Release remains denied until partial-start recovery and the
    // physical scope are qualified; selection stages no device bytes.
    bool SelectAutoRestoreOnStop() {
        if (fCaptureAttempted || fAutoRestoreOnStop || !fOwner.SelectAutoRestoreMode()) return false;
        fAutoRestoreOnStop=true;return true;
    }
    __attribute__((noinline)) bool StageCoreLocalRestore(unsigned core) {
        auto* scope=Scope();
        if (!scope || !fViews.Opened() || !fOwner.CoreLocalRestoreMode()) return false;
        Adapter io(fContext.io,*scope,fViews);
        using namespace PioneerQwen35;
        return fOwner.StageCoreLocalRestore(core,DataflowLocalRestoreReaderShimWords,
            sizeof(DataflowLocalRestoreReaderShimWords)/sizeof(uint32_t),
            DataflowLocalRestoreWriterShimWords,
            sizeof(DataflowLocalRestoreWriterShimWords)/sizeof(uint32_t),io);
    }
    __attribute__((noinline)) bool BeginCoreLocalRestoreRelease(unsigned core) {
        auto* scope=Scope();
        if (!scope || !fViews.Opened() || !fOwner.CoreLocalRestoreMode()
            || core>=PioneerQwen35::DataflowBarrierWorkers || fCoreRestoreReleaseWrites[core]) return false;
        Adapter io(fContext.io,*scope,fViews);
        return fOwner.BeginCoreLocalRestoreRelease(core,io)
            && io.ReleaseCoreLocalRestore(core,fCoreRestoreReleaseWrites[core]);
    }
    __attribute__((noinline)) bool PrepareCoreLocalRestoreHandoff(unsigned core) {
        auto* scope=Scope();
        if (!scope || !fViews.Opened() || !fOwner.CoreLocalRestoreMode()) return false;
        Adapter io(fContext.io,*scope,fViews);
        return fOwner.PrepareCoreLocalRestoreHandoff(core,io);
    }
    __attribute__((noinline)) bool ConfirmCoreLocalRestoreHandoff(unsigned core) {
        auto* scope=Scope();
        if (!scope || !fViews.Opened() || !fOwner.CoreLocalRestoreMode()
            || core>=PioneerQwen35::DataflowBarrierWorkers || !fCoreRestoreReleaseWrites[core]) return false;
        Adapter io(fContext.io,*scope,fViews);
        return fOwner.ConfirmCoreLocalRestoreHandoff(core,io);
    }
    __attribute__((noinline)) bool MarkCoreLocalRestoreComplete(unsigned core) {
        auto* scope=Scope();
        if (!scope || !fViews.Opened() || !fOwner.CoreLocalRestoreMode()
            || core>=PioneerQwen35::DataflowBarrierWorkers || !fCoreRestoreHoldWrites[core]) return false;
        Adapter io(fContext.io,*scope,fViews);
        return io.RestoreCoreHoldObserved(core) && fOwner.MarkCoreRestoredByCore(core,io);
    }
    __attribute__((noinline)) bool HoldCoreLocalRestore(unsigned core) {
        auto* scope=Scope();
        if (!scope || !fViews.Opened() || !fOwner.CoreLocalRestoreMode()
            || core>=PioneerQwen35::DataflowBarrierWorkers || !fCoreRestoreReleaseWrites[core]) return false;
        Adapter io(fContext.io,*scope,fViews);
        if (!fOwner.CoreLocalRestoreHandoffCompleted(core)
            && !fOwner.ConfirmCoreLocalRestoreHandoff(core,io)) return false;
        if (fCoreRestoreHoldWrites[core]) return io.RestoreCoreHoldObserved(core);
        return io.HoldCoreLocalRestore(core,fCoreRestoreHoldWrites[core]);
    }
    __attribute__((noinline)) bool RestoreCoreLocalHandoff(unsigned core) {
        auto* scope=Scope();
        if (!scope || !fViews.Opened() || !fOwner.CoreLocalRestoreMode()) return false;
        Adapter io(fContext.io,*scope,fViews);
        return fOwner.RestoreCoreLocalHandoff(core,io);
    }
#endif
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
    bool TakeoverResetRequired() const { return fTakeover.ResetRequired(); }
#endif
#ifdef PIONEER_QWEN35_RECOVERY_RUN_ENABLE
    // A verified ASIC reset invalidates the old tile images and RAM. Retire
    // the host BAR views without restoring those stale snapshots. The reset
    // and firmware evidence must match this in-kernel recovery ticket, and
    // the ordinary session areas must already have been disposed.
    bool RetireAfterVerifiedReset(const PioneerDriverRecoveryProof& proof,uint64_t ticket) {
        if (!PioneerQ35GraphRecoveryProofMatches(proof,ticket,
                fContext.recoveryMapping==NULL,fContext.io.postResetDisposeComplete)) return false;
        fStartOrder.Close();
        if (!fViews.Close() || fViews.HasMappings()) return false;
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
        fTakeover.RetireAfterVerifiedReset();
#endif
        return true;
    }
#endif
    __attribute__((noinline)) bool CaptureAndStageHeld() {
        using namespace PioneerQwen35;
        if (fCaptureAttempted || !Valid()) return false;
        fCaptureAttempted=true;
        auto* scope=Scope();
        if (!scope || !scope->ProveRoleStatusInspectionAllowed(fContext.io)) return false;
        PioneerQwen35RoleRouteGuard<PioneerSessionIO,PioneerQwen35RoleBorrowedScopeWitness> route(fContext.io,*scope);
        PioneerQwen35FleetInspectionResult inspection;
        const bool sampled=PioneerQwen35InspectHeldFleet(fContext.io,*scope,route,inspection);
        if (inspection.restoreFailed) sColdRecoveryRequired=true;
        if (!sampled) return false;
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
        // Admit only this locked staging attempt after the observations.
        // This does not establish absence of firmware writes or enable release.
        if (!scope->BeginDedicatedObservedStage(fContext.io) || !OpenForRecovery(*scope)) return false;
#else
        if (!scope->ProveRoleFleetHeldAndQuiet(fContext.io,0) || !OpenForRecovery(*scope)) return false;
#endif
        Adapter io(fContext.io,*scope,fViews);
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
        return fTakeover.Stage(RoleImage(),io);
#else
        if (!fOwner.CaptureAllStackless(fCodeSnapshot,DataflowRoleFleetRecoveryOwner::CodeWords(),
                fRamSnapshot,DataflowRoleFleetRecoveryOwner::RamWords(),io)) return false;
        const auto image=RoleImage();
        return image.Valid() && fOwner.StageImages(image.code[DataflowRoleReader],
            image.bounds.codeWords[DataflowRoleReader],image.code[DataflowRoleWriter],
            image.bounds.codeWords[DataflowRoleWriter],image.shim[DataflowRoleReader],
            image.bounds.shimWords[DataflowRoleReader],image.shim[DataflowRoleWriter],
            image.bounds.shimWords[DataflowRoleWriter],io);
#endif
    }
// PIONEER_Q35_SHARED_AUTO_ARM_BEGIN
    __attribute__((noinline)) bool ArmAndBeginRelease() {
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
        auto* scope=Scope();
        if (!scope || !fViews.Opened() || !scope->ProveRoleDedicatedObservedStage(fContext.io)) return false;
        Adapter io(fContext.io,*scope,fViews);
        if (!fTakeover.Arm(io) || !scope->BeginDedicatedObservedRelease(fContext.io)) return false;
        fReleaseAuthorized=true; return true;
#else
        auto* scope=Scope();
        if (!scope || !fViews.Opened()) return false;
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
        // Auto-restore may arm only when the real scope can cover a partial
        // three-role start and its stop/hold recovery. The owner rechecks the
        // full staged fleet and preservation mask before arming; StartCore
        // independently checks the release proof again for each tile. The
        // current Haiku scope still denies that physical proof.
        if (fAutoRestoreOnStop && (!fOwner.AutoRestoreMode()
            || !fOwner.ComputeLocalRamPreservationSelected()
            || fContext.io.runtimeCore>=PioneerQwen35::DataflowBarrierWorkers
            || !scope->ProveRoleReleaseSafe(fContext.io,fContext.io.runtimeCore))) return false;
#endif
        Adapter io(fContext.io,*scope,fViews);
        if (!fOwner.PreparePossibleRelease(io) || !fOwner.BeginReleaseAttempt(io)
            || !fStartOrder.Arm()) return false;
        fReleaseAuthorized=true; return true;
#endif
    }
// PIONEER_Q35_SHARED_AUTO_ARM_END
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
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
        if (!fTakeover.BeginRelease(core)) return false;
        scope->EndDedicatedObservedStage();
#else
        if (!fStartOrder.BeginRelease(core)) return false;
#endif
        Adapter io(fContext.io,*scope,fViews);
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
        fReleaseConfirmed[core]=io.ReleaseDedicatedCore(core,fReleaseAttempted[core]);
        return fReleaseConfirmed[core] && fReleaseAttempted[core] && fTakeover.ConfirmRelease(core);
#else
        fReleaseConfirmed[core]=io.ReleaseCore(core,fReleaseAttempted[core]);
        return fReleaseConfirmed[core] && fReleaseAttempted[core] && fStartOrder.ConfirmRelease(core);
#endif
    }
    __attribute__((noinline)) bool PrepareRoleStacks(unsigned core) {
        using namespace PioneerQwen35;
        if (!fStartOrder.BeginHandoff(core)) return false;
        auto* scope=Scope();
        if (!scope || core>=DataflowBarrierWorkers || !fReleaseAuthorized
            || !fReleaseConfirmed[core] || fStackReleaseSubmitted[core] || !fViews.Opened()
            || !scope->ProveRoleStartupHandoffSafe(fContext.io,core)) return false;
        Adapter io(fContext.io,*scope,fViews);
        bool ready=false;
        for (unsigned poll=0;poll<65536;poll++) {
            if (io.StartupHandoffWriteAllowed(core)) { ready=true; break; }
            fContext.io.Pause();
        }
        if (!ready || !io.InspectRunningRoleImages(core,RoleImage())
            || !fOwner.CaptureStacklessCore(core,io)
            || !io.InspectRunningRoleRamCopies(core,fOwner.OriginalRamWords(core,DataflowRoleReader),
                fOwner.OriginalRamWords(core,DataflowRoleWriter))
            || !io.StartupHandoffWriteAllowed(core) || !fOwner.MarkStacklessMayRun(core,io)) return false;
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
        if (fAutoRestoreOnStop && !fOwner.ArmAutoRestoreCore(core,io)) return false;
#endif
        // Consume the one-shot submission before calling the transport. The
        // transport marks the actual store immediately before issuing it.
        fStackReleaseSubmitted[core]=true; fStackReleaseWriting[core]=true;
        const bool okay=fContext.io.WriteStackReleaseTracked(core,fStackReleaseAttempted[core]);
        fStackReleaseWriting[core]=false;
        fStackReleaseConfirmed[core]=okay;
        return okay && fStackReleaseAttempted[core] && fOwner.StacklessCopiesCaptured(core)
            && fStartOrder.ConfirmHandoff(core);
    }
    __attribute__((noinline)) bool CoreReady(unsigned core,const PioneerQwen35::DataflowWorkerSnapshot& snapshot) {
        auto* scope=Scope();
        if (!scope || core>=PioneerQwen35::DataflowBarrierWorkers || !fReleaseConfirmed[core]
#ifndef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
            || !fStackReleaseConfirmed[core]
#endif
            || !fReleaseAuthorized || !fViews.Opened()) return false;
        Adapter io(fContext.io,*scope,fViews);
        if (!fRunningImagesObserved[core]) {
            if (!io.InspectRunningRoleImages(core,RoleImage())
#ifndef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
                || !io.InspectRunningRoleRamCopies(core,
                    fOwner.OriginalRamWords(core,PioneerQwen35::DataflowRoleReader),
                    fOwner.OriginalRamWords(core,PioneerQwen35::DataflowRoleWriter))
#endif
                ) return false;
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
#ifndef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
            || !fStackReleaseConfirmed[core]
#endif
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
#ifndef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
            || !fStackReleaseConfirmed[core]
#endif
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
#ifndef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
        const unsigned stackRelease=DataflowRoleSnapshotHandoffAddress
            +offsetof(DataflowRoleSnapshotHandoff,release);
        if (address==stackRelease && bytes==sizeof(uint32_t))
            return fStackReleaseSubmitted[core] && !fStackReleaseAttempted[core] && fStackReleaseWriting[core]
                && io.StartupHandoffWriteAllowed(core);
        if (!fStackReleaseConfirmed[core]) return false;
#endif
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
        fStartOrder.Close();
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
        // A close attempt must never allow a later launch of the remaining
        // partially released fleet, even when stop itself needs recovery.
        if (auto* scope=Scope()) scope->EndDedicatedObservedRelease();
        fReleaseAuthorized=false;
        bool any=false;
        for (bool attempted:fReleaseAttempted) any|=attempted;
        if (!any) return fContext.io.StopAndAuditAll(stage,core);
        auto* scope=Scope();
        if (!scope || !fViews.Opened()) { stage=8;core=PioneerQwen35::DataflowBarrierWorkers;return false; }
        Adapter io(fContext.io,*scope,fViews);
        return fTakeoverStop.StopAndHold(fReleaseAttempted,fReleaseConfirmed,
            PioneerQwen35::DataflowExclusiveReaderParkAddress,
            PioneerQwen35::DataflowExclusiveWriterParkAddress,
            PioneerQwen35::DataflowServiceStoppedParkAddress,65536,io,stage,core);
#else
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
                if (!fReleaseAttempted[i] || fReleaseConfirmed[i] || fStackReleaseAttempted[i]
                    || fStackCancelAttempted[i] || fStopAttempted[i]
                    || fComputeStopAttempted[i] || fHoldAttempted[i]) continue;
                if (fReleaseHeld[i]) {
                    if (!io.UnconfirmedReleaseHeld(i)) { stage=15;core=i;return false; }
                } else if (io.UnconfirmedReleaseHeld(i)) fReleaseHeld[i]=true;
            }
            for (unsigned i=0;i<PioneerQwen35::DataflowBarrierWorkers;i++) {
                if (!fReleaseAttempted[i] || fReleaseHeld[i]) continue;
                if (fHoldAttempted[i]) { if (!io.HoldObserved(i)) { stage=11;core=i;return false; } continue; }
                if (fStackReleaseAttempted[i] && !fStackReleaseConfirmed[i]
                    && !fStackReleaseDrainedAbsent[i]) {
                    typename Adapter::StackReleaseObservation state=Adapter::StackReleaseNotObserved;
                    if (!io.ObserveDrainedStackRelease(i,state)) { stage=16;core=i;return false; }
                    if (state==Adapter::StackReleaseObserved) fStackReleaseConfirmed[i]=true;
                    else fStackReleaseDrainedAbsent[i]=true;
                }
                if (!fStackReleaseConfirmed[i]) {
                    // An unfenced posted release can race cancellation.
                    if (fStackReleaseAttempted[i] && !fStackReleaseDrainedAbsent[i])
                        { stage=16;core=i;return false; }
                    if (!fStackCancelAttempted[i]
                        && !io.WaitAndRequestStackCancel(i,fStackCancelAttempted[i],fStackReleaseAttempted[i],65536))
                        { stage=16;core=i;return false; }
                    continue;
                }
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
                    bool stopped=false;
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
                    if (fAutoRestoreOnStop && fStackReleaseConfirmed[i]) {
                        stopped=io.RolesAutoRestoredAndParked(i,fStopToken[i],
                            PioneerQwen35::DataflowServiceProbeReaderParkAddress,
                            PioneerQwen35::DataflowServiceProbeWriterParkAddress);
                        if (stopped) fAutoRestoreParked[i]=true;
                    } else
#endif
                    {
                        const auto bounds=RoleBounds();
                        stopped=fStackReleaseConfirmed[i]
                            ?io.RolesStoppedAndParked(i,fStopToken[i],bounds.park[0],bounds.park[1])
                            :fStackCancelAttempted[i] && io.StackCancelledAndParked(i,bounds.park[0],bounds.park[1]);
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
                        if (fAutoRestoreOnStop && !fStackReleaseConfirmed[i] && stopped)
                            fAutoCancelParked[i]=true;
#endif
                    }
                    if (stopped) { parked=true;break; }
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
            // No reset is asserted until every started tile has either both
            // normal stop receipts or both stackless cancellation receipts,
            // plus a stopped compute receipt/park proof. The scope must
            // independently prove the entire fleet and NoCs are quiet.
            for (unsigned i=0;i<PioneerQwen35::DataflowBarrierWorkers;i++) {
                if (!fReleaseAttempted[i] || fReleaseHeld[i]) continue;
                if (fHoldAttempted[i] ? !io.HoldObserved(i) : !io.HoldCore(i,fHoldAttempted[i]))
                    { stage=13;core=i;return false; }
            }
            for (unsigned i=0;i<PioneerQwen35::DataflowBarrierWorkers;i++) {
                if (fReleaseHeld[i] && !io.UnconfirmedReleaseHeld(i)) { stage=15;core=i;return false; }
                if (!io.AllRolesHeld(i)) { stage=14;core=i;return false; }
            }
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
            if (fAutoRestoreOnStop) {
                for (unsigned i=0;i<PioneerQwen35::DataflowBarrierWorkers;i++) {
                    if (!fOwner.StacklessCopiesCaptured(i) || fAutoRestoreRecorded[i]) continue;
                    const bool okay=fStackReleaseConfirmed[i]
                        ?fAutoRestoreParked[i] && fOwner.MarkAutoRestoredCore(i,io)
                        :fAutoCancelParked[i] && fStackCancelAttempted[i]
                            && fOwner.MarkAutoCancelledCore(i,fStackReleaseAttempted[i],
                                fStackReleaseDrainedAbsent[i],io);
                    if (!okay) { stage=17;core=i;return false; }
                    fAutoRestoreRecorded[i]=true;
                }
            }
#endif
            stage=0;core=PioneerQwen35::DataflowBarrierWorkers;return true;
        }
        return fContext.io.StopAndAuditAll(stage,core);
#endif
    }
    __attribute__((noinline)) bool RestoreRolesHeld() {
        fStartOrder.Close();
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
        return fTakeover.Untouched(); // touched takeover images require verified ASIC reset
#else
        if (fOwner.Untouched()) return true;
        auto* scope=Scope();
        if (!scope || !OpenForRecovery(*scope)) return false;
        Adapter io(fContext.io,*scope,fViews);
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
        // Before a copy-aware release ledger starts, no role can have used
        // stack RAM. Restore the staged fleet through the existing held path.
        if (fOwner.CoreLocalRestoreMode() && !fOwner.CopyAwareReleaseStarted()) {
            if (fOwner.CoveredStateRestored()) return fOwner.VerifyAfterResidentRestore(io);
            return fOwner.RestoreAll(io);
        }
        if (fAutoRestoreOnStop) {
            if (fOwner.CoveredStateRestored()) return fOwner.VerifyAfterAutoResidentRestore(io);
            return fOwner.RestoreAfterAutoRestoreFleetProof(io);
        }
        if (fOwner.CoreLocalRestoreMode()) {
            if (fOwner.CoveredStateRestored()) return fOwner.VerifyAfterCoreLocalResidentRestore(io);
            return fOwner.RestoreAfterCoreLocalFleetProof(io);
        }
#endif
        if (fOwner.CoveredStateRestored()) return fOwner.VerifyAfterResidentRestore(io);
        return fOwner.RestoreAll(io);
#endif
    }
    __attribute__((noinline)) bool VerifyAfterResidentRestore() {
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
        return fTakeover.Untouched();
#else
        if (fOwner.Untouched()) return true;
        auto* scope=Scope();
        if (!scope || !OpenForRecovery(*scope)) return false;
        Adapter io(fContext.io,*scope,fViews);
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
        if (fOwner.CoreLocalRestoreMode() && !fOwner.CopyAwareReleaseStarted())
            return fOwner.CoveredStateRestored() && fOwner.VerifyAfterResidentRestore(io);
        if (fAutoRestoreOnStop)
            return fOwner.CoveredStateRestored() && fOwner.VerifyAfterAutoResidentRestore(io);
        if (fOwner.CoreLocalRestoreMode())
            return fOwner.CoveredStateRestored() && fOwner.VerifyAfterCoreLocalResidentRestore(io);
#endif
        return fOwner.CoveredStateRestored() && fOwner.VerifyAfterResidentRestore(io);
#endif
    }
    bool FinalizeBeforeResume() {
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
        return fTakeover.Untouched() && fViews.Close();
#else
        return (fOwner.Untouched() || fOwner.CoveredStateRestored()) ?fViews.Close():false;
#endif
    }
};

#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
static bool
PioneerQ35SharedRoleProjectionRestored(const PioneerQwen35SharedRoleBinding* binding)
{
    return binding && binding->ProjectionFleetRestoredLedger();
}
#endif

#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
// Force the non-loadable snapshot build to compile each actual binding path.
// Nothing calls this function in the driver; it creates no hardware control.
static __attribute__((used,noinline)) bool
PioneerQ35CoreLocalBindingCompileWitness(PioneerQwen35SharedRoleBinding* binding,
    unsigned operation,unsigned core)
{
    if (!binding) return false;
    switch (operation) {
        case 0: return binding->SelectCoreLocalRestore();
        case 1: return binding->StageCoreLocalRestore(core);
        case 2: return binding->BeginCoreLocalRestoreRelease(core);
        case 3: return binding->MarkCoreLocalRestoreComplete(core);
        case 4: return binding->PrepareCoreLocalRestoreHandoff(core);
        case 5: return binding->ConfirmCoreLocalRestoreHandoff(core);
        case 6: return binding->RestoreCoreLocalHandoff(core);
        case 7: return binding->HoldCoreLocalRestore(core);
        case 8: return binding->SelectAutoRestoreOnStop();
        default: return false;
    }
}
#endif

static bool PioneerQ35SharedRoleCopiesCaptured(const PioneerQwen35SharedRoleBinding* binding,unsigned core) {
    return binding && binding->StacklessCopiesCaptured(core);
}
static_assert(alignof(PioneerQwen35SharedRoleBinding)<=alignof(void*),"shared role binding requires ordinary kernel allocation");

static PioneerQwen35SharedRoleBinding*
PioneerQ35CreateSharedRoleBinding(PioneerStoreContext& context)
{
    auto* binding=new(std::nothrow) PioneerQwen35SharedRoleBinding(context);
    if (binding && !binding->Valid()) { delete binding; binding=NULL; }
    return binding;
}
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
static bool
PioneerQ35SelectSharedRoleAutoRestore(PioneerQwen35SharedRoleBinding* binding)
{
    return binding && binding->SelectAutoRestoreOnStop();
}
#endif
static bool
PioneerQ35DestroySharedRoleBinding(PioneerQwen35SharedRoleBinding* binding)
{
    if (!binding) return true;
    if (!binding->CanDestroy()) return false;
    delete binding; return true;
}
#ifdef PIONEER_QWEN35_RECOVERY_RUN_ENABLE
static bool
PioneerQ35ReleaseSharedRoleAfterReset(PioneerQwen35SharedRoleBinding*& binding,
    const PioneerDriverRecoveryProof& proof,uint64_t ticket)
{
    if (!binding) return true;
    if (!binding->RetireAfterVerifiedReset(proof,ticket)) return false;
    delete binding;binding=NULL;return true;
}
#endif
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
bool PioneerSessionIO::SharedPrepareRoleStacks(unsigned core) {
    return qwen35DataflowContext && qwen35DataflowContext->qwen35SharedRoleBinding
        && qwen35DataflowContext->qwen35SharedRoleBinding->PrepareRoleStacks(core);
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
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
bool PioneerSessionIO::SharedTakeoverResetRequired() {
    return qwen35DataflowContext && qwen35DataflowContext->qwen35SharedRoleBinding
        && qwen35DataflowContext->qwen35SharedRoleBinding->TakeoverResetRequired();
}
#endif
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
