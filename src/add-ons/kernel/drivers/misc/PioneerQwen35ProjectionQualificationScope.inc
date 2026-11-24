/* SPDX-License-Identifier: MIT */
// Haiku session adapter for the fixed projection. This is compiled only in
// the non-loadable shared-role witness. It relies on the existing borrowed
// scope for caller, lock, generation and hardware proofs; those fleet proofs
// still fail closed, so this class cannot authorize a physical launch.
#if !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || !defined(PIONEER_QWEN35_DEVICE_DISPATCH) \
    || !defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS) || !defined(PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY)
#error Projection qualification scope requires the non-loadable shared-role witness
#endif

class PioneerQ35ProjectionQualificationScope {
    using Fixture=PioneerQwen35::DataflowProjectionQualification;
    PioneerStoreContext& c;
    PioneerQwen35RoleBorrowedScopeWitness* Borrowed() const {
        auto* scope=c.io.qwen35BorrowedRoleScope;
        return scope && scope->ProveExclusiveRewrite(c.io)?scope:NULL;
    }
    bool Identity() const {
        return Borrowed() && sOpenCookies==1 && !sColdRecoveryRequired && !sReloadQuiesced
            && c.io.qwen35DataflowContext==&c && c.bankStorageOnly && sBankStorageOnly
            && c.bytes==PioneerResidentBanks::Total && c.initialized
            && !c.qwen35DataflowAuthority && !c.qwen35DataflowProgramScratch && !c.qwen35GraphOwner
            && c.io.SharedRoleFirmwareMatches()
            && c.banks.Matches(c.baseline.baseline.baseline.baseline.dram);
    }
    bool Owned(const void* candidate) const {
        auto* scratch=c.qwen35QualificationOwner.Scratch();
        return scratch && candidate==scratch && !c.qwen35QualificationOwner.Retained();
    }
    template<class Proof>
    bool CheckFleet(PioneerQwen35RoleBorrowedScopeWitness* scope,Proof proof,bool idle) const {
        // Every fleet check must leave both TLBs selecting worker zero. A
        // failed selection may have programmed only one TLB; a denied proof
        // after a successful selection must not strand another route.
        auto* scratch=c.qwen35QualificationOwner.Scratch();
        if (c.io.runtimeCore!=0) { Retain(scratch);return false; }
        bool okay=true,selectionFailed=false;
        for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
            if (!c.io.SelectRuntimeCore(core)) { selectionFailed=true;okay=false;break; }
            if (!proof(core)) { okay=false;break; }
        }
        // A failed selection can leave a posted or partially applied TLB
        // write. Keep the owner rather than issuing a competing restoration.
        if (selectionFailed) { Retain(scratch);return false; }
        // Do not program another route if the exclusive session or its bank
        // identity disappeared while sampling the fleet.
        const bool restored=Identity() && Borrowed()==scope && c.io.SelectRuntimeCore(0)
            && c.io.DataflowPanelMappingStable() && Identity() && Borrowed()==scope
            && (!idle || c.io.CoreIdle(0)) && Identity();
        if (!restored) Retain(scratch);
        return okay && restored;
    }
    bool HeldAndQuiet() const {
        auto* scope=Borrowed();
        if (!scope || !c.q38Dispatch || !c.q38DispatchRoutes
            || c.q38Dispatch->prepared!=PioneerQwen35::DataflowCoreCount
            || c.q38Dispatch->unsafe || c.q38Dispatch->closed || c.q38Dispatch->rootRunning) return false;
        return CheckFleet(scope,[&](unsigned core) { return scope->ProveRoleFleetHeldAndQuiet(c.io,core); },true);
    }
    bool TransferReady(const void* candidate) const {
        return Identity() && Owned(candidate) && c.qwen35QualificationInternalActive
            && (!c.qwen35DataflowService || c.qwen35DataflowService->Closed()
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
                || c.qwen35DataflowService->ScratchHeld()
#endif
                ) && !c.Unsafe()
            && c.lease.ready && !c.lease.pending && !c.lease.recovery_required
            && c.host.state==TT_HOST_PAGE_READY && !c.host.released && HeldAndQuiet();
    }
    bool RuntimeIdentity(const PioneerQ35ProjectionScratch& scratch,
        const PioneerQwen35::DataflowProjectionProgram& program) const {
        auto* scope=Borrowed();
        if (!scope || !Identity() || !Owned(&scratch) || !c.qwen35QualificationInternalActive
            || !c.qwen35DataflowService || !c.qwen35DataflowService->Ready() || c.Unsafe(true)
            || !program.Valid() || !(scratch.IsStaged() || scratch.IsRunning())
            || !scratch.MatchesBankClaimOwner(c.banks,c.baseline.baseline.baseline.baseline.dram)) return false;
        return true;
    }
    bool RuntimeReady(const PioneerQ35ProjectionScratch& scratch,
        const PioneerQwen35::DataflowProjectionProgram& program) const {
        auto* scope=Borrowed();
        if (!scope || !RuntimeIdentity(scratch,program)) return false;
        return CheckFleet(scope,[&](unsigned core) { return scope->ProveRoleRuntimeSafe(c.io,core); },false);
    }
    void Retain(const void* candidate) const {
        if (!candidate || candidate!=c.qwen35QualificationOwner.Scratch()) return;
        c.qwen35QualificationOwner.Retain();c.banks.Quarantine();c.lease.recovery_required=true;
        c.io.QuarantineHostPage();sColdRecoveryRequired=true;
    }
public:
    explicit PioneerQ35ProjectionQualificationScope(PioneerStoreContext& context):c(context) {}
    PioneerQ35ProjectionQualificationScope(const PioneerQ35ProjectionQualificationScope&)=delete;
    PioneerQ35ProjectionQualificationScope& operator=(const PioneerQ35ProjectionQualificationScope&)=delete;
    bool QualificationOwnerAllocationAllowed(const PioneerStoreContext& requested) const {
        return &requested==&c && Identity() && !c.qwen35QualificationOwner.Active()
            && !c.qwen35DataflowService && !c.Unsafe();
    }
    bool QualificationScratchOperationAllowed(const PioneerQ35ProjectionQualificationOwner& holder,
        const PioneerQ35ProjectionScratch& scratch,unsigned operation,const PioneerStoreContext& requested) const {
        return &requested==&c && &holder==&c.qwen35QualificationOwner && holder.Owns(scratch)
            && operation<=PioneerQ35ProjectionQualificationOwner::Restore && TransferReady(&scratch);
    }
    bool ScratchReserveAllowed(const void* p,unsigned bank,unsigned offset,unsigned bytes) const {
        return bank==0 && offset==Fixture::ScratchOffset && bytes==Fixture::ScratchBytes
            && !c.banks.Q35StreamClaimed() && TransferReady(p);
    }
    bool ScratchTransferAllowed(const void* p,unsigned bank,unsigned offset,bool) const {
        return bank==0 && offset>=Fixture::ScratchOffset && offset<Fixture::ScratchOffset+Fixture::ScratchBytes
            && !(offset&(Fixture::PageBytes-1)) && TransferReady(p)
            && c.banks.Q35StreamOwnerMatches(c.baseline.baseline.baseline.baseline.dram,p,
                PioneerQwen35::DataflowCoreCount);
    }
    bool ScratchStageAllowed(const void* p) const { return TransferReady(p); }
    bool ScratchOutputAllowed(const void* p) const { return TransferReady(p); }
    bool ScratchRestoreAllowed(const void* p) const { return TransferReady(p); }
    // Only the still-attached owner may inspect its quarantined transfer.
    // This is observation authority, not a fleet ownership or recovery proof.
    bool ScratchRetainedObservationAllowed(const void* p,PioneerSessionIO& io) const {
        auto* scope=c.io.qwen35BorrowedRoleScope;
        return &io==&c.io && scope && scope->ProveRetainedOwnerIdentity(io)
            && sOpenCookies==1 && c.qwen35QualificationInternalActive
            && c.qwen35QualificationOwner.Retained() && p==c.qwen35QualificationOwner.Scratch()
            && c.banks.Quarantined() && c.lease.pending && c.lease.recovery_required
            && (!c.qwen35DataflowService || c.qwen35DataflowService->Closed())
            && c.q38Dispatch && c.q38DispatchRoutes && c.q38Dispatch->prepared==PioneerQwen35::DataflowCoreCount
            && !c.q38Dispatch->unsafe && !c.q38Dispatch->closed && !c.q38Dispatch->rootRunning && io.runtimeCore==0
            && io.DataflowPanelMappingStable()
            && PioneerQwen35WithSelectedClock(io,0,[&]() { return io.ResetRead()==0x47800; });
    }
    void RetainScratch(const void* p) { Retain(p); }
    bool QualificationPrepareRoutesAllowed(const PioneerQ35ProjectionScratch& scratch,
        const PioneerStoreContext& requested) const {
        return &requested==&c && TransferReady(&scratch)
            && scratch.MatchesStagedLease(0,Fixture::ScratchAddress,Fixture::ScratchBytes)
            && scratch.MatchesBankClaim(c.banks,c.baseline.baseline.baseline.baseline.dram);
    }
    bool QualificationInstallationAllowed(const PioneerQ35ProjectionScratch& scratch,
        const PioneerStoreContext& requested) const {
        auto* scope=Borrowed();
        if (!scope || !QualificationPrepareRoutesAllowed(scratch,requested) || !c.qwen35DataflowRoutesAudited)
            return false;
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
        // Inspect all 120 held workers, L2CPU reset and host DMA while the
        // exact scratch owner and service-free session are still retained.
        // This observation does not replace the independent fleet proof.
        bool restoreFailed=false;
        if (!scope->InspectQualificationHeldFleet(c.io,restoreFailed)) {
            if (restoreFailed) Retain(&scratch);
            return false;
        }
#endif
        return CheckFleet(scope,[&](unsigned core) {
            return scope->ProveRoleStacklessFleetPreparationSafe(c.io,core);
        },false);
    }
    bool QualificationLeaseMatches(const PioneerQ35ProjectionScratch& scratch,unsigned bank,unsigned address,
        unsigned bytes,const unsigned (&routes)[2][PioneerQwen35::Banks]) const {
        if (!Identity() || !Owned(&scratch) || !c.qwen35QualificationInternalActive
            || !c.qwen35DataflowRoutesAudited || !scratch.MatchesStagedLease(bank,address,bytes)
            || !scratch.MatchesBankClaim(c.banks,c.baseline.baseline.baseline.baseline.dram)
            || !PioneerQwen35::DataflowBankRoutesValid(routes)) return false;
        for (unsigned n=0;n<2;n++) for (unsigned b=0;b<PioneerQwen35::Banks;b++)
            if (routes[n][b]!=c.qwen35DataflowRoutes[n][b]) return false;
        return true;
    }
    bool QualificationExecutionSafe(const PioneerQ35ProjectionScratch& scratch,
        const PioneerQwen35::DataflowProjectionProgram& program) const { return RuntimeReady(scratch,program); }
    // A worker can execute as soon as its command is published; demanding
    // an idle fleet here would reject valid concurrent NoC traffic. Check
    // the exact in-flight owner and permit active traffic. These bounded
    // checks do not identify every traffic source or exclude firmware writes.
    // Pre-publication and terminal fleet audits remain separate.
    bool QualificationProjectionInFlightSafe(const PioneerQ35ProjectionScratch& scratch,
        const PioneerQwen35::DataflowProjectionProgram& program) const {
#if defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) && defined(PIONEER_QWEN35_RECOVERY_RUN_ENABLE)
        auto* scope=Borrowed();
        auto* qualification=c.qwen35QualificationOwner.Qualification();
        return scope && qualification && qualification->IsRunning() && qualification->Program()==&program
            && c.qwen35QualificationOwner.Owns(scratch) && scratch.IsRunning() && RuntimeIdentity(scratch,program)
            && c.q35PciReservationHeld && !c.q35PciReleaseAttempted
            && c.io.runtimeCore==0 && c.io.DataflowPanelMappingStable()
            && scope->ProveRoleProjectionInFlightSafe(c.io,&scratch,&program);
#else
        (void)scratch;(void)program;return false;
#endif
    }
    // Called only after a successful per-core operation under this same
    // locked owner. The next fleet check and scratch transfer require root.
    // This does not establish the still-missing runtime or firmware proof.
    bool QualificationRestoreRuntimeRoute(const PioneerQ35ProjectionScratch& scratch,
        const PioneerQwen35::DataflowProjectionProgram& program,PioneerSessionIO& io) const {
        if (&io!=&c.io || !Identity() || !Owned(&scratch) || !c.qwen35QualificationInternalActive
            || !c.qwen35DataflowService || !c.qwen35DataflowService->Ready() || c.Unsafe(true)
            || !program.Valid() || !(scratch.IsStaged() || scratch.IsRunning())
            || !scratch.MatchesBankClaimOwner(c.banks,c.baseline.baseline.baseline.baseline.dram)
            || io.runtimeCore>=PioneerQwen35::DataflowCoreCount || !io.DataflowPanelMappingStable()
            || !io.SelectRuntimeCore(0) || !io.DataflowPanelMappingStable() || !Identity()) {
            Retain(&scratch);return false;
        }
        return true;
    }
    bool ScratchRunAllowed(const void* p) const {
        auto* qualification=c.qwen35QualificationOwner.Qualification();
        auto* scratch=c.qwen35QualificationOwner.Scratch();
        return scratch && p==scratch && qualification && qualification->Program()
            && RuntimeReady(*scratch,*qualification->Program());
    }
    bool QualificationRunAllowed(const PioneerQ35ProjectionQualificationOwner& holder,
        const PioneerQ35ProjectionScratch& scratch,const PioneerQwen35::DataflowProjectionQualificationService& q,
        const PioneerStoreContext& requested) const {
        return &requested==&c && &holder==&c.qwen35QualificationOwner && holder.Owns(scratch)
            && q.Program() && RuntimeReady(scratch,*q.Program());
    }
    bool AuditQualification(PioneerSessionIO& io,const PioneerQwen35::DataflowProjectionProgram& program,
        unsigned sequence,const PioneerQwen35::DataflowWorkerSnapshot (&receipts)[PioneerQwen35::DataflowCoreCount]) {
        auto* scratch=c.qwen35QualificationOwner.Scratch();
        return &io==&c.io && scratch && RuntimeReady(*scratch,program)
            && PioneerQwen35DataflowAuditProjectionReceipts(io,program,sequence,receipts);
    }
    void RetainQualification(const void*,const void* scratch) { Retain(scratch); }
    bool QualificationCloseAllowed(const PioneerQ35ProjectionQualificationOwner& holder,
        const PioneerQ35ProjectionScratch& scratch,const PioneerQwen35::DataflowProjectionQualificationService& q,
        const PioneerStoreContext& requested) const {
        return &requested==&c && &holder==&c.qwen35QualificationOwner && q.Program()
            && RuntimeReady(scratch,*q.Program());
    }
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
    bool QualificationHoldAllowed(const void* p,const PioneerQwen35::DataflowServiceOwner& service) const {
        auto* scratch=c.qwen35QualificationOwner.Scratch();
        auto* qualification=c.qwen35QualificationOwner.Qualification();
        return scratch && p==scratch && &service==c.qwen35DataflowService
            && qualification && qualification->IsAudited() && qualification->Program()
            && scratch->IsRunning() && RuntimeReady(*scratch,*qualification->Program());
    }
    // The service has already completed its held restoration sequence. This
    // only returns the selected route; the separate held proof remains denied.
    bool QualificationRestoreHeldRoute(const void* p,const PioneerQwen35::DataflowServiceOwner& service,
        PioneerSessionIO& io) const {
        auto* scratch=c.qwen35QualificationOwner.Scratch();
        auto* qualification=c.qwen35QualificationOwner.Qualification();
        if (&io!=&c.io || !scratch || p!=scratch || !Identity() || !Owned(p)
            || !c.qwen35QualificationInternalActive || &service!=c.qwen35DataflowService
            || !service.ScratchHeld() || c.Unsafe() || !qualification || !qualification->IsAudited()
            || !scratch->IsRunning() || !scratch->MatchesBankClaimOwner(c.banks,c.baseline.baseline.baseline.baseline.dram)
            || io.runtimeCore>=PioneerQwen35::DataflowCoreCount || !io.DataflowPanelMappingStable()
            || !io.SelectRuntimeCore(0) || !io.DataflowPanelMappingStable() || !Identity()) {
            Retain(p);return false;
        }
        return true;
    }
    // A successful one-shot resume has completed before this call. No failed
    // resume or route write is retried here; the owner remains retained.
    bool QualificationRestoreClosedRoute(const PioneerQ35ProjectionScratch& scratch,
        const PioneerQwen35::DataflowServiceOwner& service,PioneerSessionIO& io) const {
        auto* qualification=c.qwen35QualificationOwner.Qualification();
        if (&io!=&c.io || !Identity() || !Owned(&scratch) || !c.qwen35QualificationInternalActive
            || &service!=c.qwen35DataflowService || !service.Closed() || c.Unsafe()
            || !c.qwen35QualificationOwner.RetirementPrepared() || !scratch.IsRestored()
            || !qualification || !qualification->IsAudited()
            || !scratch.MatchesBankClaimOwner(c.banks,c.baseline.baseline.baseline.baseline.dram)
            || io.runtimeCore>=PioneerQwen35::DataflowCoreCount || !io.DataflowPanelMappingStable()
            || !io.SelectRuntimeCore(0) || !io.DataflowPanelMappingStable() || !Identity()) {
            Retain(&scratch);return false;
        }
        return true;
    }
    // The binding's complete ledger is historical evidence; the independent
    // live fleet/NoC proof still fails closed in the real borrowed scope.
    bool QualificationHeldAllowed(const void* p,const PioneerQwen35::DataflowServiceOwner& service) const {
        auto* scratch=c.qwen35QualificationOwner.Scratch();
        auto* qualification=c.qwen35QualificationOwner.Qualification();
        return scratch && p==scratch && Identity() && Owned(p) && c.qwen35QualificationInternalActive
            && &service==c.qwen35DataflowService && service.ScratchHeld() && !c.Unsafe()
            && qualification && qualification->IsAudited() && scratch->IsRunning()
            && scratch->MatchesBankClaimOwner(c.banks,c.baseline.baseline.baseline.baseline.dram)
            && PioneerQ35SharedRoleProjectionRestored(c.qwen35SharedRoleBinding) && HeldAndQuiet();
    }
    // Preparation is recorded while held; resume separately rechecks the
    // same bank owner, complete role ledger and live transfer/fleet proof.
    // The real borrowed fleet proof still denies this transition.
// PIONEER_Q35_SCOPE_RESUME_BEGIN
    bool QualificationResumeAllowed(const void* p,const PioneerQwen35::DataflowServiceOwner& service) const {
        auto* scratch=c.qwen35QualificationOwner.Scratch();
        auto* qualification=c.qwen35QualificationOwner.Qualification();
        return scratch && p==scratch && &service==c.qwen35DataflowService && service.ScratchHeld()
            && c.qwen35QualificationOwner.RetirementPrepared() && scratch->IsRestored()
            && qualification && qualification->IsAudited()
            && scratch->MatchesBankClaimOwner(c.banks,c.baseline.baseline.baseline.baseline.dram)
            && PioneerQ35SharedRoleProjectionRestored(c.qwen35SharedRoleBinding) && TransferReady(p);
    }
// PIONEER_Q35_SCOPE_RESUME_END
#endif
    bool ScratchStopped(const void* p) const {
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
        return c.qwen35DataflowService && QualificationHeldAllowed(p,*c.qwen35DataflowService);
#else
        (void)p;return false;
#endif
    }
    bool QualificationOwnerRetirementAllowed(const PioneerQ35ProjectionQualificationOwner& holder,
        const PioneerQ35ProjectionScratch& scratch,const PioneerQwen35::DataflowProjectionQualificationService& q) const {
        return &holder==&c.qwen35QualificationOwner && q.IsAudited() && scratch.IsRestored()
            && c.qwen35DataflowService && c.qwen35DataflowService->Closed() && TransferReady(&scratch);
    }
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
    // A retired bank claim is prepared only while this exact restored scratch
    // and role ledger are still held, with a fresh transfer/fleet check.
// PIONEER_Q35_SCOPE_PREPARE_BEGIN
    bool QualificationOwnerPrepareRetirementAllowed(const PioneerQ35ProjectionQualificationOwner& holder,
        const PioneerQ35ProjectionScratch& scratch,const PioneerQwen35::DataflowProjectionQualificationService& q) const {
        return &holder==&c.qwen35QualificationOwner && holder.Owns(scratch) && scratch.IsRestored()
            && q.IsAudited() && c.qwen35DataflowService && c.qwen35DataflowService->ScratchHeld()
            && scratch.MatchesBankClaimOwner(c.banks,c.baseline.baseline.baseline.baseline.dram)
            && PioneerQ35SharedRoleProjectionRestored(c.qwen35SharedRoleBinding) && TransferReady(&scratch);
    }
// PIONEER_Q35_SCOPE_PREPARE_END
#endif
    bool QualificationOwnerAbandonAllowed(const PioneerQ35ProjectionQualificationOwner& holder) const {
        return &holder==&c.qwen35QualificationOwner && Identity() && c.qwen35QualificationInternalActive
            && !c.qwen35DataflowService && !c.banks.Q35StreamClaimed() && !c.Unsafe();
    }
    bool QualificationOwnerAbandonCapturedAllowed(const PioneerQ35ProjectionQualificationOwner& holder,
        const PioneerQ35ProjectionScratch& scratch,
        const PioneerQwen35::DataflowProjectionQualificationService& qualification) const {
        return &holder==&c.qwen35QualificationOwner && holder.Owns(scratch) && qualification.IsUnstarted()
            && scratch.IsCaptured() && !c.qwen35DataflowService && TransferReady(&scratch);
    }
    bool QualificationOwnerAbandonRestoredAllowed(const PioneerQ35ProjectionQualificationOwner& holder,
        const PioneerQ35ProjectionScratch& scratch,
        const PioneerQwen35::DataflowProjectionQualificationService& qualification) const {
        return &holder==&c.qwen35QualificationOwner && holder.Owns(scratch) && qualification.IsUnstarted()
            && scratch.IsRestored() && (!c.qwen35DataflowService || c.qwen35DataflowService->Closed())
            && TransferReady(&scratch);
    }
};
