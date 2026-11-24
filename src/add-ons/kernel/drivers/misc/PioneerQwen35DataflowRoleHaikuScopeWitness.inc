/* SPDX-License-Identifier: MIT */
// Compile-only bridge for the actual Haiku session, lock and mapper types.
// The existing control path can borrow its lock in this non-loadable witness;
// no writable mapping operation is registered and no accelerator is touched.
#if !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || !defined(PIONEER_QWEN35_DEVICE_DISPATCH)
#error Role scope witness must only be compiled into the non-loadable graph/recovery driver
#endif
#include "PioneerQwen35DataflowRoleHaikuMap.h"
#include "PioneerQwen35DataflowRoleRouteGuard.h"
#include "PioneerQwen35DataflowNocSafetySample.h"
#if defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) && defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS)
#include "PioneerQwen35DataflowFleetInspection.h"
class PioneerQ35ProjectionQualificationScope;
#endif
#include "../../../../../tools/pioneer/PioneerQwen35DataflowRoleReadOnlyViews.h"
#ifdef PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY
#include "../../../../../tools/pioneer/PioneerQwen35DataflowRoleWritableViews.h"
#include "PioneerQwen35DataflowRoleFleetPciIO.h"

static status_t Qwen35DataflowControl(PioneerProbeCookie*,uint32_t,void*,size_t);
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
static status_t Qwen35DataflowGenerate(PioneerProbeCookie*,void*,size_t);
static bool PioneerQ35SharedRoleCopiesCaptured(const PioneerQwen35SharedRoleBinding*,unsigned);
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
static bool PioneerQ35SharedRoleResetTransition(const PioneerQwen35SharedRoleBinding*,unsigned,uint32_t,uint32_t);
static bool PioneerQ35SharedRoleSnapshotHoldTransition(const PioneerQwen35SharedRoleBinding*,unsigned,bool);
#endif
#endif
// Borrow, never reacquire, the lock already held by Qwen35DataflowControl.
// The private constructor limits creation to that control path. The caller
// must detach before releasing sLock; no memory or control writes are yet
// authorized by this witness.
class PioneerQwen35RoleBorrowedScopeWitness {
    friend status_t Qwen35DataflowControl(PioneerProbeCookie*,uint32_t,void*,size_t);
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
    friend status_t Qwen35DataflowGenerate(PioneerProbeCookie*,void*,size_t);
#endif
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
    friend class PioneerQwen35SharedRoleBinding;
#endif
#if defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) && defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS)
    friend class PioneerQ35ProjectionQualificationScope;
    bool fQualificationInspection=false;
#endif
    PioneerProbeCookie* fCookie;
    PioneerStoreContext* fContext;
    thread_id fThread=-1;
    team_id fTeam=-1;
    uint64 fGeneration=0;
    bool fAttached=false,fAttempted=false;
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
    bool fDedicatedStageObserved=false,fDedicatedReleaseObserved=false;
    // The binding calls this only after its ARC, fleet and DMA observations.
    // It records a point-in-time admission, not firmware write exclusion.
    bool BeginDedicatedObservedStage(PioneerSessionIO& io) {
        if (fDedicatedStageObserved || !ProveRoleStatusInspectionAllowed(io)) return false;
        fDedicatedStageObserved=true;return true;
    }
    void EndDedicatedObservedStage() { fDedicatedStageObserved=false; }
    // Called only after the complete staged fleet has been verified and the
    // software release ledger armed. This is an observation-based compile
    // witness, not proof of concurrent firmware exclusion or recovery.
    bool BeginDedicatedObservedRelease(PioneerSessionIO& io) {
        if (fDedicatedReleaseObserved || !ProveRoleDedicatedObservedStage(io)) return false;
        fDedicatedReleaseObserved=true;return true;
    }
    void EndDedicatedObservedRelease() { fDedicatedReleaseObserved=false; }
#endif
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
    const void* fFaultOwner=NULL;
    const void* fFaultRun=NULL;
    bool fFaultAttempted=false,fFaultClose=false;
#endif
    PioneerQwen35RoleBorrowedScopeWitness(PioneerProbeCookie* cookie,PioneerStoreContext* context)
        :fCookie(cookie),fContext(context) {}
#if defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) && defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS)
    // The qualification scope calls this only after binding its exact staged
    // scratch and bank lease. This is a read-only, bounded observation; it
    // does not grant role memory, reset, firmware-exclusion or launch rights.
    bool InspectQualificationHeldFleet(PioneerSessionIO& io,bool& restoreFailed) {
        restoreFailed=false;
        if (fQualificationInspection || !ProveExclusiveRewrite(io)
            || fContext->qwen35DataflowService || fContext->qwen35SharedRoleBinding
            || !io.SharedRoleFirmwareMatches()) return false;
        fQualificationInspection=true;
        PioneerQwen35RoleRouteGuard<PioneerSessionIO,PioneerQwen35RoleBorrowedScopeWitness> route(io,*this);
        PioneerQwen35FleetInspectionResult result;
        const bool sampled=PioneerQwen35InspectHeldFleet(io,*this,route,result);
        fQualificationInspection=false;
        restoreFailed=result.restoreFailed;
        if (sampled && (!ProveExclusiveRewrite(io) || !io.SharedRoleFirmwareMatches())) {
            restoreFailed=true;return false;
        }
        return sampled;
    }
#endif
public:
    PioneerQwen35RoleBorrowedScopeWitness(const PioneerQwen35RoleBorrowedScopeWitness&)=delete;
    PioneerQwen35RoleBorrowedScopeWitness& operator=(const PioneerQwen35RoleBorrowedScopeWitness&)=delete;
    ~PioneerQwen35RoleBorrowedScopeWitness() {
        if (fAttached) { (void)Detach(); sColdRecoveryRequired=true; }
    }
    bool Attach() {
        if (fAttempted) return false;
        fAttempted=true;
        if (fAttached || !fCookie || !fContext || fContext->io.qwen35BorrowedRoleScope
            || fContext->io.qwen35DataflowContext!=fContext
            || fCookie->magic!=0x50313030 || fCookie->context!=fContext
            || sRuntimeOwner!=fCookie || sQwen35ExclusiveCookie!=fCookie || sColdRecoveryRequired || sReloadQuiesced
            || !sReloadGeneration || !sBankStorageOnly || !fContext->bankStorageOnly
            || fContext->bytes!=PioneerResidentBanks::Total) return false;
        const thread_id current=find_thread(NULL); thread_info info={};
        if (current<0 || fCookie->team<0 || get_thread_info(current,&info)!=B_OK
            || info.team!=fCookie->team) return false;
        fThread=current; fTeam=info.team; fGeneration=sReloadGeneration;
        fContext->io.qwen35BorrowedRoleScope=this; fAttached=true; return true;
    }
private:
    bool OwnerIdentity(PioneerSessionIO& io,bool allowRetainedCold) const {
        if (!fAttached || !fCookie || !fContext || &io!=&fContext->io
            || io.qwen35BorrowedRoleScope!=this || io.qwen35DataflowContext!=fContext
            || fThread<0 || find_thread(NULL)!=fThread || fTeam<0 || fCookie->team!=fTeam
            || fCookie->magic!=0x50313030 || fCookie->context!=fContext
            || sRuntimeOwner!=fCookie || sQwen35ExclusiveCookie!=fCookie
            || (sColdRecoveryRequired && !allowRetainedCold) || sReloadQuiesced
            || !fGeneration || sReloadGeneration!=fGeneration || !sBankStorageOnly
            || !fContext->bankStorageOnly || fContext->bytes!=PioneerResidentBanks::Total) return false;
        thread_info info={};
        return get_thread_info(fThread,&info)==B_OK && info.team==fTeam;
    }
public:
    bool ProveOwnerIdentity(PioneerSessionIO& io) const { return OwnerIdentity(io,false); }
    // Read-only inspection under the same still-attached session. This does
    // not reopen a cold session or grant ownership of any device resource.
    bool ProveRetainedOwnerIdentity(PioneerSessionIO& io) const {
        return sColdRecoveryRequired && OwnerIdentity(io,true);
    }
    bool ProveExclusiveRewrite(PioneerSessionIO& io) const {
        if (!ProveOwnerIdentity(io)) return false;
        if (!PioneerQ35GraphOwnerRetained(fContext->qwen35GraphOwner)) {
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
            return !fFaultOwner;
#else
            return true;
#endif
        }
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
        return fFaultOwner && fFaultOwner==fContext->qwen35GraphOwner && fFaultRun
            && PioneerQ35GraphOwnerOwnsRetainedRun(fContext->qwen35GraphOwner,fFaultRun);
#else
        return false;
#endif
    }
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
    bool BeginRetainedFault(PioneerSessionIO& io,const void* owner,const void* run) {
        if (fFaultAttempted) return false;
        fFaultAttempted=true;
        if (!ProveOwnerIdentity(io) || !owner || owner!=fContext->qwen35GraphOwner || !run
            || !PioneerQ35GraphOwnerRetained(fContext->qwen35GraphOwner)
            || !PioneerQ35GraphOwnerOwnsRetainedRun(fContext->qwen35GraphOwner,run)) return false;
        fFaultOwner=owner;fFaultRun=run;return true;
    }
    bool PromoteRetainedFaultClose(PioneerSessionIO& io) {
        if (fFaultClose || !ProveExclusiveRewrite(io) || !fFaultOwner || !fFaultRun
            || !PioneerQ35GraphOwnerFaultClosePermitted(fContext->qwen35GraphOwner,fFaultRun)) return false;
        fFaultClose=true;return true;
    }
    bool FaultCloseAuthorized(PioneerSessionIO& io) const {
        return fFaultClose && ProveExclusiveRewrite(io) && fFaultOwner && fFaultRun
            && PioneerQ35GraphOwnerFaultClosePermitted(fContext->qwen35GraphOwner,fFaultRun);
    }
    bool RetainedFaultAuditActive(PioneerSessionIO& io) const {
        return fFaultOwner && fFaultRun && !fFaultClose && ProveExclusiveRewrite(io);
    }
    bool FaultAcknowledgementAuthorized(PioneerSessionIO& io,unsigned core) const {
        return RetainedFaultAuditActive(io) && core<PioneerQwen35::DataflowBarrierWorkers
            && PioneerQ35GraphOwnerFaultAckPermitted(fContext->qwen35GraphOwner,fFaultRun,core);
    }
    bool EndRetainedFault(PioneerSessionIO& io,const void* owner,const void* run) {
        if (!ProveOwnerIdentity(io) || !fFaultOwner || fFaultOwner!=owner || fFaultRun!=run
            || owner!=fContext->qwen35GraphOwner) return false;
        fFaultOwner=NULL;fFaultRun=NULL;fFaultClose=false;return true;
    }
#endif
    bool ProveRoleMemoryReadSafe(PioneerSessionIO& io,unsigned core,unsigned which) const {
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
        return which<=PioneerQwen35::DataflowRolePhysicalExtraCode
            && core<PioneerQwen35::DataflowBarrierWorkers && io.runtimeCore==core
            && ProveRoleDedicatedObservedStage(io) && io.DataflowPanelMappingStable()
            && PioneerQwen35WithSelectedClock(io,core,[&]() {
                return io.ResetRead()==0x47800 && io.ResetRead()==0x47800;
            });
#else
        // The snapshot path may inspect only held-role images and boot words.
        // In particular this does not authorize the host private-RAM reads
        // that failed in the earlier physical qualification. The independent
        // fleet proof still denies access until real ownership is provided.
        return which<=PioneerQwen35::DataflowRolePhysicalExtraCode
            && core<PioneerQwen35::DataflowBarrierWorkers && io.runtimeCore==core
            && io.DataflowPanelMappingStable() && ProveExclusiveRewrite(io)
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
            && io.SharedRoleFirmwareMatches()
#endif
            && ProveRoleFleetHeldAndQuiet(io,core)
            && PioneerQwen35WithSelectedClock(io,core,[&]() {
                return io.ResetRead()==0x47800 && io.ResetRead()==0x47800;
            }) && ProveRoleFleetHeldAndQuiet(io,core);
#endif
    }
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
    bool ProveRoleStatusInspectionAllowed(PioneerSessionIO& io) const {
        return ProveExclusiveRewrite(io) && io.SharedRoleFirmwareMatches()
            && ((fContext->qwen35DataflowService
                && fContext->qwen35DataflowService->SharedRoleCapturePending())
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
                || (fQualificationInspection && !fContext->qwen35DataflowService
                    && !fContext->qwen35SharedRoleBinding)
#endif
                );
    }
    // Read-only image comparison during installation or a separately
    // authorized retained-fault audit. Neither admits writes or proves that
    // another agent cannot change an image after it has been sampled.
    bool ProveRoleImageInspectionAllowed(PioneerSessionIO& io,unsigned core) const {
        return core<PioneerQwen35::DataflowBarrierWorkers && io.runtimeCore==core
            && io.DataflowPanelMappingStable()
            && (ProveRoleStatusInspectionAllowed(io)
                || (RetainedFaultAuditActive(io) && io.SharedRoleFirmwareMatches()
                    && ProveRoleFaultAuditSafe(io,core)));
    }
#endif
    bool ProveRoleMemoryWriteSafe(PioneerSessionIO& io,unsigned core,unsigned which,unsigned offset,unsigned bytes) const {
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
        return which<=PioneerQwen35::DataflowRolePhysicalExtraCode
            && PioneerQwen35::DataflowRolePhysicalWriteRange(which,offset,bytes)
            && ProveRoleMemoryReadSafe(io,core,which);
#else
        return which<=PioneerQwen35::DataflowRolePhysicalExtraCode
            && PioneerQwen35::DataflowRolePhysicalWriteRange(which,offset,bytes)
            && ProveRoleMemoryReadSafe(io,core,which);
#endif
    }
// PIONEER_Q35_SCOPE_RESET_CONTROL_BEGIN
    bool ProveRoleControlWriteSafe(PioneerSessionIO& io,unsigned core,uint32_t expected,uint32_t desired) const {
#if defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) && defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS)
        return ProveRoleMemoryReadSafe(io,core,PioneerQwen35::DataflowRolePhysicalExtraCode)
            && PioneerQ35SharedRoleResetTransition(fContext->qwen35SharedRoleBinding,core,expected,desired)
            && ProveRoleMemoryReadSafe(io,core,PioneerQwen35::DataflowRolePhysicalExtraCode);
#else
        (void)io;(void)core;(void)expected;(void)desired;return false;
#endif
    }
// PIONEER_Q35_SCOPE_RESET_CONTROL_END
    bool ProveRoleFleetHeldAndQuiet(PioneerSessionIO&,unsigned) const { return false; }
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
    bool ProveRoleDedicatedObservedStage(PioneerSessionIO& io) const {
        return fDedicatedStageObserved && ProveRoleStatusInspectionAllowed(io);
    }
    // Read-only, point-in-time authorization for the final stop audit. The
    // staging admission ends at first release and cannot authorize shutdown.
    bool ProveRoleDedicatedHeldStatusAllowed(PioneerSessionIO& io,unsigned core) const {
        return core<PioneerQwen35::DataflowBarrierWorkers && ProveExclusiveRewrite(io)
            && io.SharedRoleFirmwareMatches() && io.DataflowPanelMappingStable();
    }
#endif
    // This admits fixed status reads only. The adapter must separately prove
    // the fleet is held, verify the selected route, and inspect both hardware
    // reset-PC override registers; this method does not assert their values.
    bool ProveRoleBootRoutingInspectionAllowed(PioneerSessionIO& io,unsigned core) const {
        return core<PioneerQwen35::DataflowBarrierWorkers && ProveExclusiveRewrite(io)
            && io.runtimeCore==core && io.DataflowPanelMappingStable()
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
            && io.SharedRoleFirmwareMatches()
#endif
            ;
    }
    // A firmware or other agent reset after capture invalidates the timing
    // observation. Held reset samples do not prove that no such event has
    // occurred; this continuity proof is deliberately still unavailable.
    bool ProveRoleResetContinuitySinceCapture(PioneerSessionIO&,unsigned) const { return false; }
    bool ProveRoleLocalRamStable(PioneerSessionIO& io,unsigned core) const {
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
        return core<PioneerQwen35::DataflowBarrierWorkers && io.runtimeCore==core
            && io.DataflowPanelMappingStable() && ProveExclusiveRewrite(io)
            && ProveRoleFleetHeldAndQuiet(io,core)
            && PioneerQ35SharedRoleCopiesCaptured(fContext->qwen35SharedRoleBinding,core)
            && ProveRoleResetContinuitySinceCapture(io,core);
#else
        (void)io; (void)core; return false;
#endif
    }
    // Requires stable exclusive ownership of the handoff, RAM copies and
    // live images, plus recovery if the release write may still be posted.
    bool ProveRoleStartupHandoffSafe(PioneerSessionIO&,unsigned) const { return false; }
    // Requires the verified pre-stack shim images, preservation of both
    // private RAMs across release, and exclusion of every conflicting writer
    // or release for the full snapshot. Receipts and reads alone do not.
    bool ProveRoleStacklessSnapshotSafe(PioneerSessionIO&,unsigned) const { return false; }
    // Deferred snapshots require independent guarantees about the prior RAM
    // state, reset preservation, all conflicting agents, and recovery of
    // unsnapshotted tiles (including misrouted restoration writes).
    bool ProveRoleStacklessFleetPreparationSafe(PioneerSessionIO&,unsigned) const { return false; }
    bool ProveRoleUncapturedRamProtected(PioneerSessionIO&,unsigned) const { return false; }
    // A future copy-aware restore must bind these pointers to the retained
    // fleet ledger and exclude every source/destination writer for the whole
    // transaction. No physical proof is implemented yet.
    bool ProveRoleHeldCopySources(PioneerSessionIO&,unsigned,const uint32_t*& reader,
        const uint32_t*& writer,bool& captured) const {
        reader=writer=nullptr;captured=false;return false;
    }
    bool ProveRoleCoreRestore(PioneerSessionIO&,unsigned,const uint32_t*,const uint32_t*) const { return false; }
    // These require lifetime-spanning ownership and drain guarantees, not
    // merely sampled reset/route/receipt values. No live proof exists yet.
    bool ProveRoleRestoreHandoffHeldSafe(PioneerSessionIO&,unsigned) const { return false; }
    bool ProveRoleRestoreCompletionSafe(PioneerSessionIO&,unsigned) const { return false; }
    bool ProveRoleRestoreCompletedAndHeld(PioneerSessionIO&,unsigned) const { return false; }
    bool ProveRoleRestoreHandoffRetrySafe(PioneerSessionIO&,unsigned) const { return false; }
    bool ProveRoleRestoreReleaseReady(PioneerSessionIO&,unsigned,const uint32_t*,const uint32_t*) const { return false; }
    bool ProveRoleRestorePhysicalReleaseSafe(PioneerSessionIO&,unsigned) const { return false; }
    bool ProveRoleRestorePhysicalHoldSafe(PioneerSessionIO&,unsigned) const { return false; }
    bool ProveRoleRestoreHeldAfterAttempt(PioneerSessionIO&,unsigned) const { return false; }
    bool ProveRoleRetirementRamSources(PioneerSessionIO&,unsigned,const uint32_t*& reader,
        const uint32_t*& writer,bool& captured) const {
        reader=writer=nullptr;captured=false;return false;
    }
    bool ProveRoleFleetImageRetirementSafe(PioneerSessionIO&) const { return false; }
    bool ProveRoleFleetImageRetirementRetrySafe(PioneerSessionIO&) const { return false; }
    bool ProveRoleCoreImageRetirementSafe(PioneerSessionIO&,unsigned,const uint32_t*,const uint32_t*) const { return false; }
    bool ProveRoleCoreImageRetirementRetrySafe(PioneerSessionIO&,unsigned,const uint32_t*,const uint32_t*) const { return false; }
    // Must fence this tile's particular attempted stack-release write and
    // exclude all later writes to the handoff for the recovery transaction.
    bool ProveRoleStackReleaseWriteDrained(PioneerSessionIO&,unsigned) const { return false; }
    // These require exclusive control of the handoff and owned code, no
    // possible stack-release write, and independent stopped/NoC proofs.
    bool ProveRoleStackCancelSafe(PioneerSessionIO&,unsigned) const { return false; }
    bool ProveRoleStackCancelledSafe(PioneerSessionIO&,unsigned) const { return false; }
    // Must additionally establish partial-fleet cleanup and a role-aware
    // stop/hold path before permitting any physical reset release.
    bool ProveRoleReleaseSafe(PioneerSessionIO&,unsigned) const { return false; }
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
    // Only permits the non-loadable witness to exercise the release path.
    // Samples and session ownership do not qualify physical release, stop,
    // firmware exclusion, or no-reboot recovery.
    bool CanAttemptDedicatedObservedRelease(PioneerSessionIO& io,unsigned core) const {
        return fDedicatedReleaseObserved && core<PioneerQwen35::DataflowBarrierWorkers
            && ProveRoleStatusInspectionAllowed(io);
    }
#endif
    // A reset read alone cannot rule out a delayed posted release write. This
    // must prove that the particular attempted write has drained, and that no
    // other agent can release the tile for the remainder of recovery.
    bool ProveRoleReleaseWriteDrained(PioneerSessionIO&,unsigned) const { return false; }
    // Only the non-loadable dedicated clean-close path uses this observed
    // admission. Faulted graphs, firmware exclusion and physical recovery
    // require separate evidence; the stop owner checks receipts and ordering.
    bool CanObserveDedicatedCleanStop(PioneerSessionIO& io,unsigned core) const {
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
        auto* service=fContext?fContext->qwen35DataflowService:NULL;
        return core<PioneerQwen35::DataflowBarrierWorkers && service
            && service->SharedDedicatedCleanStopPending() && ProveExclusiveRewrite(io)
            && !PioneerQ35GraphOwnerRetained(fContext->qwen35GraphOwner)
            && !RetainedFaultAuditActive(io) && io.SharedRoleFirmwareMatches();
#else
        (void)io;(void)core;return false;
#endif
    }
    // The snapshot service records that Close began from Ready before it
    // changes phase to Faulted. That marker is not a hardware proof: the
    // separate runtime proof remains mandatory and currently denies access.
    bool CanObserveSnapshotCleanStop(PioneerSessionIO& io,unsigned core) const {
#if defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) && defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS)
        auto* service=fContext?fContext->qwen35DataflowService:NULL;
        return core<PioneerQwen35::DataflowBarrierWorkers && service
            && service->SharedSnapshotCleanStopPending() && ProveExclusiveRewrite(io)
            && !PioneerQ35GraphOwnerRetained(fContext->qwen35GraphOwner)
            && !RetainedFaultAuditActive(io) && io.SharedRoleFirmwareMatches()
            && ProveRoleRuntimeSafe(io,core);
#else
        (void)io;(void)core;return false;
#endif
    }
    bool ProveRoleStopSafe(PioneerSessionIO& io,unsigned core,bool fault) const {
        return !fault && (CanObserveDedicatedCleanStop(io,core) || CanObserveSnapshotCleanStop(io,core));
    }
    bool ProveRolePostStopSafe(PioneerSessionIO& io,unsigned core) const {
        return CanObserveDedicatedCleanStop(io,core) || CanObserveSnapshotCleanStop(io,core);
    }
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
    // A parked receipt is an observation, not lifetime exclusion of a later
    // reset, writer or misroute. The real scope still denies this proof.
    bool ProveRoleAutoRestoreCompletionSafe(PioneerSessionIO&,unsigned) const { return false; }
    bool ProveRoleAutoRestoreArmSafe(PioneerSessionIO&,unsigned,const uint32_t*,const uint32_t*) const { return false; }
    bool ProveRoleAutoRestoreHeldSafe(PioneerSessionIO&,unsigned,const uint32_t*,const uint32_t*) const { return false; }
    bool ProveRoleAutoCancelHeldSafe(PioneerSessionIO&,unsigned,const uint32_t*,const uint32_t*,bool) const { return false; }
    bool ProveRoleAutoRestoreRetirementSafe(PioneerSessionIO&,unsigned,const uint32_t*,const uint32_t*,bool) const { return false; }
#endif
    bool ProveComputeStopSafe(PioneerSessionIO& io,unsigned core) const {
        return CanObserveDedicatedCleanStop(io,core) || CanObserveSnapshotCleanStop(io,core);
    }
    bool ProveComputePostStopSafe(PioneerSessionIO& io,unsigned core) const {
        return CanObserveDedicatedCleanStop(io,core) || CanObserveSnapshotCleanStop(io,core);
    }
    bool ProveRoleHoldSafe(PioneerSessionIO& io,unsigned core) const {
        if (CanObserveDedicatedCleanStop(io,core)) return true;
#if defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) && defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS)
        return CanObserveSnapshotCleanStop(io,core)
            && PioneerQ35SharedRoleSnapshotHoldTransition(fContext->qwen35SharedRoleBinding,core,false);
#else
        return false;
#endif
    }
    bool ProveRoleHeldAfterAttempt(PioneerSessionIO& io,unsigned core) const {
        if (CanObserveDedicatedCleanStop(io,core)) return true;
#if defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) && defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS)
        return CanObserveSnapshotCleanStop(io,core)
            && PioneerQ35SharedRoleSnapshotHoldTransition(fContext->qwen35SharedRoleBinding,core,true);
#else
        return false;
#endif
    }
    // Must independently establish the owned role/compute images, routes,
    // NoC quiescence and session/graph state, including partially stopped
    // cores when used during clean close; mailbox/PC samples alone are
    // insufficient. This proof is not yet implemented.
    bool ProveRoleRuntimeSafe(PioneerSessionIO&,unsigned) const { return false; }
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
    // Admission for bounded runtime observations in this non-loadable
    // witness. Callers use it in their runtime decisions; it does not prove
    // firmware exclusion, NoC quiescence, or physical recovery.
    bool CanInspectDedicatedRuntime(PioneerSessionIO& io,unsigned core) const {
        auto* service=fContext?fContext->qwen35DataflowService:NULL;
        return core<PioneerQwen35::DataflowBarrierWorkers && service
            && ProveExclusiveRewrite(io) && !RetainedFaultAuditActive(io)
            && io.SharedRoleFirmwareMatches()
            && (service->SharedRoleCapturePending() || service->Ready() || service->GraphRunning());
    }
    // This stronger, lifetime-spanning guarantee must cover the interval
    // from the prior completion audit through the next command publication,
    // including other agents and firmware, even across separate ioctls.
    // The session lock and current route/PC samples do not establish it.
    bool ProveRoleAuditedDescriptorContinuity(PioneerSessionIO&,unsigned) const { return false; }
#endif
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
    // Fault audit requires image/route/both-NoC proof. Acknowledgement also
    // requires the exact retained graph's all-core two-pass audit; neither is
    // established by the retained identity alone.
    bool ProveRoleFaultAuditSafe(PioneerSessionIO&,unsigned) const { return false; }
    bool ProveRoleFaultAckSafe(PioneerSessionIO& io,unsigned core) const {
        return FaultAcknowledgementAuthorized(io,core) && ProveRoleFaultAuditSafe(io,core);
    }
#endif
    bool Detach() {
        if (!fAttached || !fContext) return false;
        const bool valid=find_thread(NULL)==fThread && fContext->io.qwen35BorrowedRoleScope==this
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
            && !fFaultOwner
#endif
            ;
        if (fContext->io.qwen35BorrowedRoleScope==this) fContext->io.qwen35BorrowedRoleScope=NULL;
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
        fDedicatedStageObserved=false;fDedicatedReleaseObserved=false;
#endif
        fAttached=false;
        if (!valid) sColdRecoveryRequired=true;
        return valid;
    }
};
#endif

class PioneerQwen35RoleHaikuScopeWitness {
    PioneerProbeCookie* fCookie=NULL;
    PioneerStoreContext* fContext=NULL;
    thread_id fThread=-1;
    team_id fTeam=-1;
    uint64 fGeneration=0;
    bool fHeld=false,fAttempted=false;
public:
    PioneerQwen35RoleHaikuScopeWitness()=default;
    PioneerQwen35RoleHaikuScopeWitness(const PioneerQwen35RoleHaikuScopeWitness&)=delete;
    PioneerQwen35RoleHaikuScopeWitness& operator=(const PioneerQwen35RoleHaikuScopeWitness&)=delete;
    bool Open(PioneerProbeCookie* cookie) {
        if (fAttempted || !cookie) return false;
        fAttempted=true;
        if (AcquireProbeLock(cookie,true)!=B_OK) return false;
        fHeld=true; fCookie=cookie; fContext=cookie->context;
        fThread=find_thread(NULL); fTeam=cookie->team; fGeneration=sReloadGeneration;
        thread_info info={};
        if (!fContext || fThread<0 || fTeam<0
            || get_thread_info(fThread,&info)!=B_OK || info.team!=fTeam
            || !ProveExclusiveRewrite(fContext->io)) {
            (void)ReleaseOnce(); return false;
        }
        return true;
    }
    PioneerSessionIO* IO() const { return fHeld && fContext?&fContext->io:NULL; }
    bool ProveExclusiveRewrite(PioneerSessionIO& io) const {
        return fHeld && fCookie && fContext && fIOEqual(io)
            && fThread>=0 && find_thread(NULL)==fThread && fTeam>=0 && fCookie->team==fTeam
            && fCookie->magic==0x50313030 && sRuntimeOwner==fCookie
            && fCookie->context==fContext && io.qwen35DataflowContext==fContext
            && !sColdRecoveryRequired && !sReloadQuiesced && fGeneration
            && sReloadGeneration==fGeneration && sBankStorageOnly && fContext->bankStorageOnly
            && fContext->bytes==PioneerResidentBanks::Total
            && fContext->qwen35DataflowService && fContext->qwen35DataflowService->Ready()
            && !PioneerQ35GraphOwnerRetained(fContext->qwen35GraphOwner);
    }
    // No held/quiet/reset-window proof is wired yet. Thus even this compile
    // witness denies all boot, L1 and private-RAM reads through its guard.
    bool ProveRoleMemoryReadSafe(PioneerSessionIO&,unsigned,unsigned) const { return false; }
#ifdef PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY
    // No held-fleet, retained-snapshot or RAM-window proof is connected yet.
    // Mapping and guard types compile, but every actual write is denied.
    bool ProveRoleMemoryWriteSafe(PioneerSessionIO&,unsigned,unsigned,unsigned,unsigned) const { return false; }
    bool ProveRoleControlWriteSafe(PioneerSessionIO&,unsigned,uint32_t,uint32_t) const { return false; }
#endif
    bool Close() {
        if (!fHeld || fThread<0 || find_thread(NULL)!=fThread) return false;
        return ReleaseOnce();
    }
private:
    bool ReleaseOnce() {
        if (!fHeld) return false;
        // Release is attempted once; an ambiguous failure latches recovery.
        fHeld=false;
        const status_t status=release_sem(sLock);
        if (status!=B_OK) sColdRecoveryRequired=true;
        return status==B_OK;
    }
    bool fIOEqual(const PioneerSessionIO& io) const { return fContext && &fContext->io==&io; }
};

// Persistent ownership is intentional: on an unmap failure the caller keeps
// the object and the exclusive scope rather than dropping live area IDs.
class PioneerQwen35RoleHaikuViewsWitness {
    PioneerQwen35RoleHaikuScopeWitness fScope;
    PioneerQwen35RoleHaikuMap fPlatform;
    PioneerQwen35::DataflowRoleReadOnlyViews<PioneerQwen35RoleHaikuMap> fViews;
public:
    PioneerQwen35RoleHaikuViewsWitness():fViews(fPlatform) {}
    bool Open(PioneerProbeCookie* cookie) {
        if (!fScope.Open(cookie)) return false;
        PioneerSessionIO* io=fScope.IO();
        if (!io) return false;
        PioneerQwen35RoleRouteGuard<PioneerSessionIO,PioneerQwen35RoleHaikuScopeWitness> guard(*io,fScope);
        return fViews.Open(io->profile.bar0_host_start,io->profile.bar0_host_end,guard);
    }
    bool Read(unsigned core,unsigned window,unsigned offset,uint32_t& output) {
        PioneerSessionIO* io=fScope.IO();
        if (!io) return false;
        PioneerQwen35RoleRouteGuard<PioneerSessionIO,PioneerQwen35RoleHaikuScopeWitness> guard(*io,fScope);
        return fViews.Read(core,window,offset,output,guard);
    }
    // Selected tile only. This samples known status registers twice; it does
    // not prove fleet-wide quiescence, completion of the local-RAM reset
    // window, or absence of incoming NoC traffic, and grants no RAM access.
    bool AuditHeldAndQuiet(unsigned core) {
        using namespace PioneerQwen35;
        auto sampleFifosAndClock=[&]() {
            return PioneerQwen35SampleNocFifosAndClock([&](unsigned offset) {
                uint32_t value=UINT32_MAX;
                const unsigned window=offset>=0x10000?DataflowRolePhysicalNoc1:DataflowRolePhysicalNoc0;
                if (!Read(core,window,offset&0xffffU,value)) return uint32_t(UINT32_MAX);
                return value;
            });
        };
        for (unsigned pass=0;pass<2;pass++) {
            uint32_t reset=0;
            if (!sampleFifosAndClock()
                || !Read(core,DataflowRolePhysicalControl,0x1b0,reset) || reset!=0x47800)
                return false;
            for (unsigned noc=0;noc<2;noc++) {
                const unsigned window=noc?DataflowRolePhysicalNoc1:DataflowRolePhysicalNoc0;
                uint32_t raw=0,route=0;
                if (!Read(core,window,DataflowNocLogicalIdOffset,raw)
                    || !DataflowNocDecodeLocalRoute(raw,route)
                    || !DataflowNocRouteMatchesWorker(route,core)) return false;
                for (unsigned initiator=0;initiator<4;initiator++) {
                    uint32_t status=0;
                    if (!Read(core,window,0x40+initiator*0x800,status) || status)
                        return false;
                }
                for (unsigned id=0;id<32;id++) {
                    uint32_t status=0;
                    if (!Read(core,window,0x240+id*4,status) || (status&0xffU))
                        return false;
                }
            }
            if (!sampleFifosAndClock()
                || !Read(core,DataflowRolePhysicalControl,0x1b0,reset) || reset!=0x47800)
                return false;
        }
        return true;
    }
    bool Close() {
        if (!fViews.Close()) return false;
        return fScope.Close();
    }
};
// Force type-checking against the actual driver session without registering
// an operation or making the compile-only driver loadable.
__attribute__((used)) static bool
PioneerQwen35RoleHaikuViewsCompileWitness(PioneerQwen35RoleHaikuViewsWitness& owner,
    PioneerProbeCookie* cookie,unsigned action,unsigned core,unsigned window,unsigned offset,uint32_t& value)
{
    if (action==0) return owner.Open(cookie);
    if (action==1) return owner.Read(core,window,offset,value);
    if (action==2) return owner.Close();
    if (action==3) return owner.AuditHeldAndQuiet(core);
    return false;
}
#ifdef PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY
// Separate persistent owner because a failed unmap must retain the lock and
// area IDs for retry. There is no ioctl/caller and the scope denies writes.
class PioneerQwen35RoleHaikuWritableViewsWitness {
    PioneerQwen35RoleHaikuScopeWitness fScope;
    PioneerQwen35RoleHaikuMap fPlatform;
    PioneerQwen35::DataflowRoleWritableViews<PioneerQwen35RoleHaikuMap> fViews;
public:
    PioneerQwen35RoleHaikuWritableViewsWitness():fViews(fPlatform) {}
    bool Open(PioneerProbeCookie* cookie) {
        if (!fScope.Open(cookie)) return false;
        PioneerSessionIO* io=fScope.IO();
        if (!io) return false;
        PioneerQwen35RoleRouteGuard<PioneerSessionIO,PioneerQwen35RoleHaikuScopeWitness> guard(*io,fScope);
        return fViews.Open(io->profile.bar0_host_start,io->profile.bar0_host_end,guard);
    }
    bool Write(unsigned core,unsigned which,unsigned offset,const uint32_t* data,unsigned bytes) {
        PioneerSessionIO* io=fScope.IO();
        if (!io) return false;
        PioneerQwen35RoleRouteGuard<PioneerSessionIO,PioneerQwen35RoleHaikuScopeWitness> guard(*io,fScope);
        return fViews.WriteWords(core,which,offset,data,bytes,guard);
    }
    bool WriteResetDisable(unsigned core,uint32_t expected,uint32_t desired) {
        PioneerSessionIO* io=fScope.IO();
        if (!io) return false;
        PioneerQwen35RoleRouteGuard<PioneerSessionIO,PioneerQwen35RoleHaikuScopeWitness> guard(*io,fScope);
        return fViews.WriteResetDisable(core,expected,desired,guard);
    }
    bool Close() {
        if (!fViews.Close()) return false;
        return fScope.Close();
    }
};
__attribute__((used)) static bool
PioneerQwen35RoleHaikuWritableViewsCompileWitness(PioneerQwen35RoleHaikuWritableViewsWitness& owner,
    PioneerProbeCookie* cookie,unsigned action,unsigned core,unsigned which,unsigned offset,
    const uint32_t* data,unsigned bytes,uint32_t expected,uint32_t desired)
{
    if (action==0) return owner.Open(cookie);
    if (action==1) return owner.Write(core,which,offset,data,bytes);
    if (action==2) return owner.WriteResetDisable(core,expected,desired);
    if (action==3) return owner.Close();
    return false;
}
// Type-check the recovery owner against the borrowed session, selected routes
// and fixed Haiku mappings. Dedicated observed staging is separate from the
// snapshot proof; the non-loadable witness cannot release a physical role.
__attribute__((used)) static bool
PioneerQwen35RoleFleetPciCompileWitness(PioneerSessionIO& io,
    PioneerQwen35RoleBorrowedScopeWitness& scope,
    PioneerQwen35::DataflowRoleWritableViews<PioneerQwen35RoleHaikuMap>& views,
    PioneerQwen35::DataflowRoleFleetRecoveryOwner& owner,uint32_t* codeStorage,
    uint32_t* ramStorage,const uint32_t* image,unsigned words,unsigned action)
{
    using namespace PioneerQwen35;
    PioneerQwen35RoleFleetPciIO<PioneerSessionIO,PioneerQwen35RoleBorrowedScopeWitness,
        DataflowRoleWritableViews<PioneerQwen35RoleHaikuMap>> adapter(io,scope,views);
    if (action==0) return adapter.AllRolesHeld(0);
    if (action==1) return owner.CaptureAll(codeStorage,DataflowRoleFleetRecoveryOwner::CodeWords(),
        ramStorage,DataflowRoleFleetRecoveryOwner::RamWords(),adapter);
    if (action==2) return owner.StageImages(image,words,image,words,image,words,image,words,adapter);
    if (action==3) return owner.PreparePossibleRelease(adapter) && owner.BeginReleaseAttempt(adapter);
    if (action==4) return owner.RestoreAll(adapter) && owner.VerifyAfterResidentRestore(adapter);
    return false;
}
#endif
