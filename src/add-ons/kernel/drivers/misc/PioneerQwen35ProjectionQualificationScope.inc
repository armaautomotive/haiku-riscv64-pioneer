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
            && (!c.qwen35DataflowService || c.qwen35DataflowService->Closed()) && !c.Unsafe()
            && c.lease.ready && !c.lease.pending && !c.lease.recovery_required
            && c.host.state==TT_HOST_PAGE_READY && !c.host.released && HeldAndQuiet();
    }
    bool RuntimeReady(const PioneerQ35ProjectionScratch& scratch,
        const PioneerQwen35::DataflowProjectionProgram& program) const {
        auto* scope=Borrowed();
        if (!scope || !Identity() || !Owned(&scratch) || !c.qwen35QualificationInternalActive
            || !c.qwen35DataflowService || !c.qwen35DataflowService->Ready() || c.Unsafe(true)
            || !program.Valid() || !(scratch.IsStaged() || scratch.IsRunning())
            || !scratch.MatchesBankClaimOwner(c.banks,c.baseline.baseline.baseline.baseline.dram)) return false;
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
    bool ScratchStopped(const void* p) const {
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
        auto* scope=Borrowed();
        if (!scope || !Identity() || !Owned(p) || !c.qwen35QualificationInternalActive
            || !c.qwen35DataflowService || !c.qwen35DataflowService->Closed()) return false;
        return CheckFleet(scope,[&](unsigned core) {
            return scope->ProveRoleAutoRestoreCompletionSafe(c.io,core);
        },false);
#else
        (void)p;return false;
#endif
    }
    bool QualificationOwnerRetirementAllowed(const PioneerQ35ProjectionQualificationOwner& holder,
        const PioneerQ35ProjectionScratch& scratch,const PioneerQwen35::DataflowProjectionQualificationService& q) const {
        return &holder==&c.qwen35QualificationOwner && q.IsAudited() && scratch.IsRestored()
            && c.qwen35DataflowService && c.qwen35DataflowService->Closed() && TransferReady(&scratch);
    }
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
