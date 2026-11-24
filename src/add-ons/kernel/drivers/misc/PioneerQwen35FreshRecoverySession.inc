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
#ifndef PIONEER_QWEN35_FRESH_RECOVERY_TEST
#include "PioneerQwen35ProjectionResetRecovery.h"
#endif

class PioneerQ35FreshRecoverySession {
public:
    enum Stage { Unstarted=0,Attached=1,Mapped=2,Baseline=3,Banks=4,Host=5,Lease=6,Ready=7,
        ScratchRestoring=8,ScratchVerified=9,Closing=10,LeaseClosed=11,FleetAudited=12,
        PciClosed=13,TransportClosed=14,Retiring=15,HostReleased=16,OwnerRetired=17,Retained=18 };
private:
    PioneerQ35ProjectionResetRecovery fRestore;
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
        if (fOld) {
            if (!fOld->recoveryFreshSession) fOld->recoveryFreshSession=this;
            fOld->qwen35QualificationOwner.Retain();fOld->banks.Quarantine();
        }
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
            || !fOld->qwen35QualificationOwner.ModifiedResetEligible(fOld->banks)) return false;
        return guard->validate(fTicket)==B_OK;
    }
    bool RetiredTicketValid(pioneer_driver_recovery_guard_v2_info* guard,
        const PioneerQ35ProjectionScratch* scratch,const uint32_t* snapshot) const {
        if (!guard || !fOld || !fFresh || !fOwner || !fTicket || fOwner!=sRuntimeOwner
            || fOwner->context!=fOld || !sColdRecoveryRequired || sReloadQuiesced || sOpenCookies!=1
            || !sBankStorageOnly || !fOld->bankStorageOnly || fOld->recoveryFreshSession!=this
            || !fOld->recoveryAttempted || !fOld->recoveryFinishPending || fOld->recoveryFinishTicket!=fTicket
            || fOld->recoveryMapping || !fOld->io.postResetDisposeComplete
            || !fOld->recoveryFinishProof.ReadyToRetireOneOwner()
            || fOld->recoveryFinishProof.reset_generation!=fTicket
            || fOld->recoveryFinishProof.firmware_generation!=fTicket
            || !fOld->qwen35QualificationOwner.RetiredModifiedIdentity(scratch,snapshot)
            || !scratch->ResetRestoreClaimMatches(fOld->banks,snapshot)
            || !fRestore.VerifiedFor(*scratch,fTicket,fOld->banks,fFresh->banks)) return false;
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
    template<class Scope> void RetainUncertain(Scope& scope) {
        if (fStage==Retained) {
            if (fOld && !fOld->recoveryFreshSession) fOld->recoveryFreshSession=this;
            sColdRecoveryRequired=true;return;
        }
        (void)Retain(scope,unsigned(fStage));
    }
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
            || !old.qwen35QualificationOwner.ModifiedResetEligible(old.banks) || !old.initialized
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

    // Restore through this session's exact fresh resources and retained old
    // snapshot. The underlying owner writes and rereads every page and is
    // single-use; the surrounding scope supplies the missing live proofs.
    template<class Scope>
    bool RestoreScratch(pioneer_driver_recovery_guard_v2_info* guard,Scope& scope) {
        if (fStage!=Ready || !fOld || !fFresh) return false;
        auto* scratch=fOld->qwen35QualificationOwner.Scratch();
        if (!TicketValid(guard) || !scratch
            || !scope.FreshRecoveryRestoreAdmission(*this,*fOld,*fFresh,fRestore)
            || !TicketValid(guard)) return Retain(scope,ScratchRestoring);
        fStage=ScratchRestoring;
        if (!fRestore.Restore(*scratch,fTicket,fOld->recoveryFinishProof,fFresh->io,scope,
            fOld->banks,fOld->baseline.baseline.baseline.baseline.dram,
            fFresh->banks,fFresh->baseline.baseline.baseline.baseline.dram,
            fFresh->baseline.baseline.baseline.noc,fFresh->host,fFresh->lease,
            fFresh->initialized,PioneerResidentBanks::Pages)) return Retain(scope,ScratchRestoring);
        if (!TicketValid(guard) || !fRestore.VerifiedFor(*scratch,fTicket,fOld->banks,fFresh->banks)
            || !scope.FreshRecoveryRestoreComplete(*this,*fOld,*fFresh,fRestore)
            || !TicketValid(guard) || !fRestore.VerifiedFor(*scratch,fTicket,fOld->banks,fFresh->banks))
            return Retain(scope,ScratchVerified);
        fStage=ScratchVerified;return true;
    }

    // Close only after the exact saved scratch has been restored and reread.
    // The scope must independently audit the complete fleet and host DMA
    // after the final transfer. The wired host page, fresh bank claim and old
    // snapshot stay alive even after PCI has been restored to idle; releasing
    // them requires a separate, not-yet-implemented owner-retirement step.
    // Any attempted close or uncertain result is one-shot and retained.
    template<class Scope>
    bool CloseTransport(pioneer_driver_recovery_guard_v2_info* guard,Scope& scope) {
        if (fStage!=ScratchVerified || !fOld || !fFresh) return false;
        auto verified=[&]() {
            auto* scratch=fOld->qwen35QualificationOwner.Scratch();
            return scratch && fRestore.VerifiedFor(*scratch,fTicket,fOld->banks,fFresh->banks);
        };
        if (!TicketValid(guard) || !verified()
            || !scope.FreshRecoveryCloseAdmission(*this,*fOld,*fFresh,fRestore)
            || !TicketValid(guard) || !verified()) return Retain(scope,Closing);
        fStage=Closing;
        if (!PioneerResidentLeaseEnd(fFresh->io,fFresh->baseline.baseline.baseline.noc,fFresh->lease))
            return Retain(scope,LeaseClosed);
        fStage=LeaseClosed;
        if (!TicketValid(guard) || !verified()
            || !scope.FreshRecoveryPostTransferAudit(*this,*fOld,*fFresh,fRestore)
            || !TicketValid(guard) || !verified()) return Retain(scope,FleetAudited);
        // Use a distinct fresh audit mapping, kept on the fresh context on
        // every failure. Audit both NoCs on all 120 held cores and host DMA,
        // then restore and unmap the temporary TLB apertures before PCI end.
        if (fFresh->recoveryMapping) return Retain(scope,FleetAudited);
        fFresh->recoveryMapping=new(std::nothrow) PioneerPostResetMapping(fFresh->profile);
        if (!fFresh->recoveryMapping) return Retain(scope,FleetAudited);
        auto& audit=*fFresh->recoveryMapping;
        tt_arc_scratch_result firmware{};PioneerPostResetAuditResult fleet{};
        if (!audit.Open() || !TicketValid(guard) || !audit.ProbeFirmware(100,firmware)
            || !TicketValid(guard)) return Retain(scope,FleetAudited);
        const auto& dram=fFresh->baseline.baseline.baseline.baseline.dram;
        for (unsigned word=0;word<TT_DRAM_SOURCE_WORDS;word++)
            if (audit.topology.source[word]!=dram.source[word]) return Retain(scope,FleetAudited);
        if (!audit.Audit(fleet) || !TicketValid(guard) || !audit.AuditHostDma()
            || !TicketValid(guard) || !audit.Close()) return Retain(scope,FleetAudited);
        delete fFresh->recoveryMapping;fFresh->recoveryMapping=nullptr;
        if (!TicketValid(guard) || !verified()
            || !scope.FreshRecoveryPostTransferAudit(*this,*fOld,*fFresh,fRestore)
            || !TicketValid(guard) || !verified()) return Retain(scope,FleetAudited);
        fStage=FleetAudited;
        if (!PioneerQ38DispatchMmioEnd(fFresh->io,fFresh->Pci(),fFresh->transaction))
            return Retain(scope,PciClosed);
        fStage=PciClosed;
        if (!TicketValid(guard) || !verified()
            || !scope.FreshRecoveryPciIdle(*this,*fOld,*fFresh,fRestore)
            || !TicketValid(guard) || !verified()) return Retain(scope,TransportClosed);
        fStage=TransportClosed;return true;
    }

    // Retire only after the post-transfer fleet/DMA audit and PCI idle
    // transition. The exact old snapshot stays alive through old-bank
    // destruction; the fresh bank is destroyed first. A failed attempt
    // keeps the original context and all still-owned resources retained.
    // On success this deletes the fresh session, so the caller must discard
    // its pointer and revalidate the recovery guard before generic finish.
    template<class Scope>
    bool Retire(pioneer_driver_recovery_guard_v2_info* guard,Scope& scope) {
        if (fStage!=TransportClosed || !fOld || !fFresh) return false;
        auto* scratch=fOld->qwen35QualificationOwner.Scratch();
        auto* snapshot=fOld->qwen35QualificationOwner.Snapshot();
        auto readyForRelease=[&]() {
            return !fFresh->recoveryMapping && fFresh->Pci().restored && !fFresh->transaction.config_attempted
                && !fFresh->lease.ready && !fFresh->lease.pending && !fFresh->lease.recovery_required
                && fFresh->lease.l1_restored && fFresh->lease.niu_restored && fFresh->lease.tlb_restored
                && fFresh->host.state==TT_HOST_PAGE_READY && fFresh->host.allocated && !fFresh->host.released;
        };
        if (!TicketValid(guard) || !scratch || !snapshot
            || !fRestore.VerifiedFor(*scratch,fTicket,fOld->banks,fFresh->banks)
            || !readyForRelease()
            || !scope.FreshRecoveryRetirementAdmission(*this,*fOld,*fFresh,fRestore)
            || !TicketValid(guard) || !readyForRelease()
            || !fRestore.VerifiedFor(*scratch,fTicket,fOld->banks,fFresh->banks)) return Retain(scope,Retiring);
        fStage=Retiring;
        fFresh->host.release_status=fFresh->io.ReleaseHostPage();
        fFresh->host.released=fFresh->host.release_status==B_OK;
        if (!fFresh->host.released) return Retain(scope,HostReleased);
        fStage=HostReleased;
        if (!TicketValid(guard) || !fRestore.VerifiedFor(*scratch,fTicket,fOld->banks,fFresh->banks)
            || !scope.FreshRecoveryRetirementAdmission(*this,*fOld,*fFresh,fRestore)
            || !TicketValid(guard)) return Retain(scope,HostReleased);
        if (!fOld->qwen35QualificationOwner.RetireModifiedAfterReset(fOld->recoveryFinishProof,fTicket,
            fRestore,fOld->banks,fFresh->banks,*this,scope)) return Retain(scope,OwnerRetired);
        fStage=OwnerRetired;
        if (!RetiredTicketValid(guard,scratch,snapshot)
            || !scope.FreshRecoveryAfterOwnerRetired(*this,*fOld,*fFresh,fRestore)
            || !RetiredTicketValid(guard,scratch,snapshot)) return Retain(scope,OwnerRetired);
        fOld->recoveryFreshSession=nullptr;
        delete this;return true;
    }
};
