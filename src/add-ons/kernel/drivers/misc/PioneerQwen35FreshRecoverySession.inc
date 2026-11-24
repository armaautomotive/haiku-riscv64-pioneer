/* SPDX-License-Identifier: MIT */
// Dedicated post-reset scratch-transfer session. This is compiled only into
// the non-loadable snapshot witness and is not called by the recovery ioctl.
// In particular, a caller must still supply a real scope proving the retained
// PCI reservation, same physical card, firmware/NoC exclusion and ownership
// of the old scratch snapshot. This object never reserves or unreserves PCI.
#if !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || defined(PIONEER_QWEN35_RECOVERY_QUALIFY) \
    || !defined(PIONEER_QWEN35_DEVICE_DISPATCH) || !defined(PIONEER_QWEN35_RECOVERY_RUN_ENABLE) \
    || !defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS)
#error Fresh Q35 reset transfer session is restricted to the non-loadable snapshot witness
#endif
#if defined(PIONEER_QWEN35_FRESH_RECOVERY_TEST) && defined(_KERNEL_MODE)
#error Simulated fresh-session disposal is not permitted in a kernel build
#endif

class PioneerQ35FreshRecoverySession {
public:
    enum Stage { Unstarted=0,Attached=1,Mapped=2,Baseline=3,Banks=4,Host=5,Lease=6,Ready=7,Retained=8 };
private:
    PioneerStoreContext* fOld=nullptr;
    PioneerStoreContext* fFresh=nullptr;
    PioneerProbeCookie* fOwner=nullptr;
    uint64 fTicket=0;
    Stage fStage=Unstarted;
    unsigned fFailureStage=0;

    PioneerQ35FreshRecoverySession()=default;
    ~PioneerQ35FreshRecoverySession() { delete fFresh; }
    template<class Scope>
    bool Retain(Scope& scope,unsigned failed) {
        fFailureStage=failed;fStage=Retained;
        if (fOld) { fOld->qwen35QualificationOwner.Retain();fOld->banks.Quarantine(); }
        if (fFresh) {
            fFresh->banks.Quarantine();fFresh->lease.recovery_required=true;
            fFresh->io.QuarantineHostPage();
        }
        sColdRecoveryRequired=true;
        scope.RetainFreshRecovery(*this);
        return false;
    }
    bool TicketValid(pioneer_driver_recovery_guard_v2_info* guard) const {
        if (!guard || !fOld || !fFresh || !fOwner || !fTicket
            || fOwner!=sRuntimeOwner || fOwner->context!=fOld
            || !sColdRecoveryRequired || sReloadQuiesced || sOpenCookies!=1
            || !sBankStorageOnly || !fOld->bankStorageOnly
            || fOld->recoveryFreshSession!=this || !fOld->recoveryAttempted
            || !fOld->recoveryFinishPending || fOld->recoveryFinishTicket!=fTicket
            || fOld->recoveryMapping || !fOld->io.postResetDisposeComplete
            || !fOld->recoveryFinishProof.ReadyToRetireOneOwner()
            || fOld->recoveryFinishProof.reset_generation!=fTicket
            || fOld->recoveryFinishProof.firmware_generation!=fTicket
            || !fOld->qwen35QualificationOwner.Active()
            || !fOld->qwen35QualificationOwner.Scratch()
            || !fOld->banks.Q35StreamClaimIdentity(fOld->qwen35QualificationOwner.Scratch(),
                PioneerQwen35::DataflowCoreCount)) return false;
        return guard->validate(fTicket)==B_OK;
    }
public:
    PioneerQ35FreshRecoverySession(const PioneerQ35FreshRecoverySession&)=delete;
    PioneerQ35FreshRecoverySession& operator=(const PioneerQ35FreshRecoverySession&)=delete;
    static PioneerQ35FreshRecoverySession* Allocate() { return new(std::nothrow) PioneerQ35FreshRecoverySession; }
    // No session touching PCI can be destroyed by this method. A later
    // explicit audited close must be implemented before a ready or retained
    // session can be retired.
    bool DiscardUnstarted() {
        if (fStage!=Unstarted || fOld || fFresh || fOwner || fTicket) return false;
        delete this;return true;
    }
    Stage CurrentStage() const { return fStage; }
    unsigned FailureStage() const { return fFailureStage; }
    PioneerStoreContext* Fresh() const { return fFresh; }
    uint64 Ticket() const { return fTicket; }
#ifdef PIONEER_QWEN35_FRESH_RECOVERY_TEST
    // Test doubles own no hardware mappings or DMA; production cannot call
    // this and must wait for the separately audited close implementation.
    static void DisposeSimulated(PioneerQ35FreshRecoverySession* session) {
        if (session && session->fOld && session->fOld->recoveryFreshSession==session)
            session->fOld->recoveryFreshSession=nullptr;
        delete session;
    }
#endif

    template<class Scope>
    bool Prepare(PioneerProbeCookie* owner,PioneerStoreContext& old,
        pioneer_driver_recovery_guard_v2_info* guard,Scope& scope) {
        if (fStage!=Unstarted || fOld || fFresh || fOwner || fTicket || !owner || !guard
            || owner!=sRuntimeOwner || owner->context!=&old || old.recoveryFreshSession
            || !sColdRecoveryRequired || sReloadQuiesced || sOpenCookies!=1
            || !sBankStorageOnly || !old.bankStorageOnly || !old.recoveryAttempted
            || !old.recoveryFinishPending || !old.recoveryFinishTicket || old.recoveryMapping
            || !old.io.postResetDisposeComplete || !old.recoveryFinishProof.ReadyToRetireOneOwner()
            || old.recoveryFinishProof.reset_generation!=old.recoveryFinishTicket
            || old.recoveryFinishProof.firmware_generation!=old.recoveryFinishTicket
            || !old.qwen35QualificationOwner.Active() || !old.qwen35QualificationOwner.Scratch()
            || !old.banks.Q35StreamClaimIdentity(old.qwen35QualificationOwner.Scratch(),
                PioneerQwen35::DataflowCoreCount) || !old.initialized
            || old.bytes!=PioneerResidentBanks::Total
            || guard->validate(old.recoveryFinishTicket)!=B_OK
            || !scope.FreshRecoveryAdmission(*this,owner,old,old.recoveryFinishTicket)
            || guard->validate(old.recoveryFinishTicket)!=B_OK) return false;
        auto* fresh=new(std::nothrow) PioneerStoreContext(PioneerResidentBanks::Total);
        if (!fresh) return false;
        if (!fresh->initialized || !fresh->hashes) { delete fresh;return false; }
        fresh->profile=old.profile;
        fresh->bankStorageOnly=true;
        memcpy(fresh->initialized,old.initialized,PioneerResidentBanks::Pages);
        fOld=&old;fFresh=fresh;fOwner=owner;fTicket=old.recoveryFinishTicket;
        old.recoveryFreshSession=this;fStage=Attached;
        auto allowed=[&](unsigned stage) {
            return TicketValid(guard) && scope.FreshRecoveryBoundary(*this,old,*fresh,stage)
                && TicketValid(guard);
        };
        if (!allowed(Attached)) return Retain(scope,Attached);
        if (!BlackholePioneerMmioBegin(fresh->io,fresh->Pci(),fresh->transaction))
            return Retain(scope,Mapped);
        fStage=Mapped;
        if (!allowed(Mapped)) return Retain(scope,Mapped);
        if (!fresh->io.PioneerHostDmaAuditIO::AfterNodeId(fresh->Pci().node_id[0])
            || !PioneerResidentSessionIdle(fresh->io,fresh->baseline.baseline.baseline.noc,fresh->idle))
            return Retain(scope,Baseline);
        fStage=Baseline;
        if (!allowed(Baseline)) return Retain(scope,Baseline);
        auto& oldDram=old.baseline.baseline.baseline.baseline.dram;
        auto& newDram=fresh->baseline.baseline.baseline.baseline.dram;
        if (!fresh->banks.Bind(newDram) || !old.banks.Q35RecoveryRoutesMatch(oldDram,fresh->banks,newDram))
            return Retain(scope,Banks);
        fStage=Banks;
        if (!allowed(Banks)) return Retain(scope,Banks);
        if (!PioneerPrepareHostPage(fresh->io,fresh->host)) return Retain(scope,Host);
        fStage=Host;
        if (!allowed(Host)) return Retain(scope,Host);
        if (!PioneerResidentLeaseBegin(fresh->io,fresh->baseline.baseline.baseline.noc,fresh->lease))
            return Retain(scope,Lease);
        fStage=Lease;
        if (!allowed(Lease)) return Retain(scope,Lease);
        fStage=Ready;return true;
    }
};
