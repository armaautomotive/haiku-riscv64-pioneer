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
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
#include "PioneerQwen35GuardedLeaseEnd.h"
#include "PioneerQwen35GuardedMmioBegin.h"
#include "PioneerQwen35GuardedMmioEnd.h"
#include "PioneerQwen35GuardedHostPage.h"
#include "PioneerQwen35GuardedRoleSelect.h"
#endif
#endif

class PioneerQ35FreshRecoverySession {
public:
    enum Stage { Unstarted=0,Attached=1,Mapped=2,Baseline=3,Banks=4,Host=5,Lease=6,Ready=7,
        ScratchRestoring=8,ScratchVerified=9,Closing=10,LeaseClosed=11,FleetAudited=12,
        PciClosed=13,TransportClosed=14,Retiring=15,HostReleased=16,OwnerRetired=17,Retained=18,
        RoleVerified=19,RoleHostDisposed=20,RoleMappingsProved=21,RoleGraphRetired=22,
        RoleFreshHostReleased=23,RoleOwnerRetired=24 };
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
    struct RoleRestorationReceipt { const void* binding=nullptr;const void* graph=nullptr;uint64 ticket=0; };
#endif
private:
    PioneerQ35ProjectionResetRecovery fRestore;
    PioneerStoreContext* fOld=nullptr;
    PioneerStoreContext* fFresh=nullptr;
    PioneerProbeCookie* fOwner=nullptr;
    uint64 fTicket=0;
    bool fFabric=false;
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
    bool fRole=false;
    bool fRoleMappingsEvidencePublished=false;
    bool fRoleGraphRetireAttempted=false;
    bool fRoleFreshHostReleaseAttempted=false;
    const void* fRoleBinding=nullptr;
    const void* fGraphOwner=nullptr;
    RoleRestorationReceipt fRoleReceipt{};
#endif
    Stage fStage=Unstarted;
    unsigned fFailureStage=0;

    PioneerQ35FreshRecoverySession()=default;
    ~PioneerQ35FreshRecoverySession() { delete fFresh; }
    template<class Scope>
    bool Retain(Scope& scope,unsigned failed) {
        fFailureStage=failed;fStage=Retained;
        if (fOld) {
            if (!fOld->recoveryFreshSession) fOld->recoveryFreshSession=this;
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
            if (!fRole) fOld->qwen35QualificationOwner.Retain();
            else fOld->io.QuarantineHostPage();
#else
            fOld->qwen35QualificationOwner.Retain();
#endif
            fOld->banks.Quarantine();
        }
        if (fFresh) {
            fFresh->banks.Quarantine();fFresh->lease.recovery_required=true;
            fFresh->io.QuarantineHostPage();
        }
        sColdRecoveryRequired=true;
        scope.RetainFreshRecovery(*this);
        return false;
    }
    static bool EvidenceValid(const PioneerStoreContext& old,uint64 ticket,bool fabric,bool roleMappingsProved=false) {
#ifndef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
        (void)roleMappingsProved;
#endif
        if (!ticket || !old.recoveryAttempted || old.recoveryMapping) return false;
        if (fabric) {
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
            return old.recoveryFabricEvidenceRecorded && old.recoveryFabricStartTicket==ticket
                && old.recoveryFabricPciIdleAttempted && old.recoveryFabricPciIdle
                && old.recoveryFabricAuditReleaseAttempted && old.recoveryFabricAuditReleased
                && !old.recoveryFabricArcOwner && old.io.fabricOldMmioDisposeComplete
                && !old.io.postResetDisposeComplete && old.q35PciReservationHeld && !old.q35PciReleaseAttempted
                && (old.recoveryFabricEvidence.ReadyForScratchRestore()
                    || (roleMappingsProved && old.recoveryFabricEvidence.ReadyToRetireOneOwner()))
                && old.recoveryFabricEvidence.fabric_generation==ticket
                && old.recoveryFabricEvidence.firmware_generation==ticket;
#else
            return false;
#endif
        }
        return old.recoveryFinishPending && old.recoveryFinishTicket==ticket && old.io.postResetDisposeComplete
            && old.recoveryFinishProof.ReadyToRetireOneOwner()
            && old.recoveryFinishProof.reset_generation==ticket && old.recoveryFinishProof.firmware_generation==ticket;
    }
    bool TicketValid(pioneer_driver_recovery_guard_v2_info* guard) const {
        if (!guard || !guard->validate || !fOld || !fFresh || !fOwner || !fTicket
            || fOwner!=sRuntimeOwner || fOwner->context!=fOld
            || !sColdRecoveryRequired || sReloadQuiesced || sOpenCookies!=1
            || !sBankStorageOnly || !fOld->bankStorageOnly
            || fOld->recoveryFreshSession!=this || !EvidenceValid(*fOld,fTicket,fFabric
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
                ,fRole && fRoleMappingsEvidencePublished
#endif
                )) return false;
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
        if (fRole) {
            if (!fFabric || !fOld->recoveryFabricRoleViewsDisposed || !fRoleBinding || !fGraphOwner
                || fOld->qwen35SharedRoleBinding!=fRoleBinding
                || fOld->qwen35GraphOwner!=fGraphOwner) return false;
        } else
#endif
        if (!fOld->qwen35QualificationOwner.ModifiedResetEligible(fOld->banks)) return false;
        return guard->validate(fTicket)==B_OK;
    }
    bool RetiredTicketValid(pioneer_driver_recovery_guard_v2_info* guard,
        const PioneerQ35ProjectionScratch* scratch,const uint32_t* snapshot) const {
        if (!guard || !guard->validate || !fOld || !fFresh || !fOwner || !fTicket || fOwner!=sRuntimeOwner
            || fOwner->context!=fOld || !sColdRecoveryRequired || sReloadQuiesced || sOpenCookies!=1
            || !sBankStorageOnly || !fOld->bankStorageOnly || fOld->recoveryFreshSession!=this
            || !EvidenceValid(*fOld,fTicket,fFabric)
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
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
    // Metadata only: callers must independently validate the recovery ticket
    // and physical scope before touching any views or device state.
    bool RoleReady() const {
        return fRole && fStage==Ready && fOld && fFresh && fFresh->io.fleetEnabled
            && fOld->recoveryFabricRoleViewsDisposed && fRoleBinding && fGraphOwner
            && fOld->recoveryFreshSession==this && fOld->qwen35SharedRoleBinding==fRoleBinding
            && fOld->qwen35GraphOwner==fGraphOwner;
    }
    // Software identity only; caller must validate the ticket and physical
    // exclusion separately. The old graph and role remain owned here.
    bool RoleOwnerRetirementReady(uint64 ticket) const {
        return fRole && fFabric && ticket && ticket==fTicket && fStage==RoleFreshHostReleased
            && fRoleMappingsEvidencePublished && fRoleGraphRetireAttempted && fRoleFreshHostReleaseAttempted
            && fOld && fFresh && fOld->recoveryFreshSession==this
            && fOld->qwen35SharedRoleBinding==fRoleBinding && fOld->qwen35GraphOwner==fGraphOwner
            && fFresh->host.released && fFresh->host.release_status==B_OK
            && fFresh->io.FreshRecoveryHostReleased();
    }
    bool RoleSessionRetirementReady(uint64 ticket) const {
        return fRole && fFabric && ticket && ticket==fTicket && fStage==RoleOwnerRetired
            && fRoleMappingsEvidencePublished && fRoleGraphRetireAttempted && fRoleFreshHostReleaseAttempted
            && fOld && fFresh && fOld->recoveryFreshSession==this
            && fOld->qwen35SharedRoleBinding==fRoleBinding && fOld->qwen35GraphOwner==fGraphOwner
            && !fFresh->recoveryMapping && fFresh->Pci().restored && !fFresh->transaction.config_attempted
            && !fFresh->lease.ready && !fFresh->lease.pending && !fFresh->lease.recovery_required
            && fFresh->lease.l1_restored && fFresh->lease.niu_restored && fFresh->lease.tlb_restored
            && fFresh->host.released && fFresh->host.release_status==B_OK
            && fFresh->io.FreshRecoveryHostReleased();
    }
#endif
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

private:
    template<class Scope>
    bool PrepareWithEvidence(PioneerProbeCookie* owner,PioneerStoreContext& old,
        pioneer_driver_recovery_guard_v2_info* guard,Scope& scope,uint64 ticket,bool fabric,bool role=false) {
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
        const void* requestedRole=role?old.qwen35SharedRoleBinding:nullptr;
        const void* requestedGraph=role?old.qwen35GraphOwner:nullptr;
#endif
        if (fStage!=Unstarted || fOld || fFresh || fOwner || fTicket || !owner || !guard
            || owner!=sRuntimeOwner || owner->context!=&old || old.recoveryFreshSession
            || !sColdRecoveryRequired || sReloadQuiesced || sOpenCookies!=1
            || !sBankStorageOnly || !old.bankStorageOnly || !EvidenceValid(old,ticket,fabric) || !old.initialized
            || old.bytes!=PioneerResidentBanks::Total || !guard->validate || guard->validate(ticket)!=B_OK) return false;
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
        if (role) {
            if (!fabric || !old.recoveryFabricRoleViewsDisposed || !requestedRole || !requestedGraph
                || !scope.FreshRoleRecoveryAdmission(*this,owner,old,ticket)
                || old.qwen35SharedRoleBinding!=requestedRole || old.qwen35GraphOwner!=requestedGraph) return false;
        } else
#else
        if (role) return false;
#endif
        if (!old.qwen35QualificationOwner.ModifiedResetEligible(old.banks)) return false;
        if (!scope.FreshRecoveryAdmission(*this,owner,old,ticket) || guard->validate(ticket)!=B_OK) return false;
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
        if (role && (old.qwen35SharedRoleBinding!=requestedRole || old.qwen35GraphOwner!=requestedGraph)) return false;
#endif
        auto* fresh=new(std::nothrow) PioneerStoreContext(PioneerResidentBanks::Total);
        if (!fresh) return false;
        if (!fresh->initialized || !fresh->hashes) { delete fresh;return false; }
        fresh->profile=old.profile;
        fresh->bankStorageOnly=true;
        memcpy(fresh->initialized,old.initialized,PioneerResidentBanks::Pages);
        fOld=&old;fFresh=fresh;fOwner=owner;fTicket=ticket;fFabric=fabric;
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
        fRole=role;
        if (role) { fRoleBinding=requestedRole;fGraphOwner=requestedGraph; }
#endif
        old.recoveryFreshSession=this;fStage=Attached;
        auto allowed=[&](unsigned stage) {
            return TicketValid(guard) && scope.FreshRecoveryBoundary(*this,old,*fresh,stage)
                && TicketValid(guard);
        };
        if (!allowed(Attached)) return Retain(scope,Attached);
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
        if (fabric) {
            bool lost=false;
            auto check=[&]() {
                if (lost || !TicketValid(guard) || !scope.FreshRecoverySetupAccessAllowed(*this,old,*fresh,Mapped)
                    || !TicketValid(guard)) { lost=true;return false; }
                return true;
            };
            if (!PioneerQ35GuardedMmioBegin(fresh->io,fresh->Pci(),fresh->transaction,check))
                return Retain(scope,Mapped);
        } else
#endif
        if (!BlackholePioneerMmioBegin(fresh->io,fresh->Pci(),fresh->transaction))
            return Retain(scope,Mapped);
        fStage=Mapped;
        if (!allowed(Mapped)) return Retain(scope,Mapped);
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
        if (fabric) {
            bool lost=false;
            auto check=[&]() {
                if (lost || !TicketValid(guard) || !scope.FreshRecoverySetupAccessAllowed(*this,old,*fresh,Baseline)
                    || !TicketValid(guard)) { lost=true;return false; }
                return true;
            };
            if (!fresh->io.GuardedRecoveryBaseline(fresh->Pci().node_id[0],check))
                return Retain(scope,Baseline);
            if (!PioneerQ35GuardedSessionIdle(fresh->io,fresh->baseline.baseline.baseline.noc,fresh->idle,check))
                return Retain(scope,Baseline);
        } else {
#endif
        if (!fresh->io.PioneerHostDmaAuditIO::AfterNodeId(fresh->Pci().node_id[0])) return Retain(scope,Baseline);
        if (!PioneerResidentSessionIdle(fresh->io,fresh->baseline.baseline.baseline.noc,fresh->idle))
            return Retain(scope,Baseline);
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
        }
#endif
        fStage=Baseline;
        if (!allowed(Baseline)) return Retain(scope,Baseline);
        auto& oldDram=old.baseline.baseline.baseline.baseline.dram;
        auto& newDram=fresh->baseline.baseline.baseline.baseline.dram;
        if (!fresh->banks.Bind(newDram) || !old.banks.Q35RecoveryRoutesMatch(oldDram,fresh->banks,newDram))
            return Retain(scope,Banks);
        fStage=Banks;
        if (!allowed(Banks)) return Retain(scope,Banks);
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
        if (fabric) {
            bool lost=false;
            auto check=[&]() {
                if (lost || !TicketValid(guard) || !scope.FreshRecoverySetupAccessAllowed(*this,old,*fresh,Host)
                    || !TicketValid(guard)) { lost=true;return false; }
                return true;
            };
            if (!PioneerQ35GuardedPrepareHostPage(fresh->io,fresh->host,check)) return Retain(scope,Host);
        } else
#endif
        if (!PioneerPrepareHostPage(fresh->io,fresh->host)) return Retain(scope,Host);
        fStage=Host;
        if (!allowed(Host)) return Retain(scope,Host);
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
        if (fabric) {
            bool lost=false;
            auto check=[&]() {
                if (lost || !TicketValid(guard) || !scope.FreshRecoverySetupAccessAllowed(*this,old,*fresh,Lease)
                    || !TicketValid(guard)) { lost=true;return false; }
                return true;
            };
            if (!PioneerQ35GuardedLeaseBegin(fresh->io,fresh->baseline.baseline.baseline.noc,fresh->lease,check))
                return Retain(scope,Lease);
        } else
#endif
        if (!PioneerResidentLeaseBegin(fresh->io,fresh->baseline.baseline.baseline.noc,fresh->lease))
            return Retain(scope,Lease);
        fStage=Lease;
        if (!allowed(Lease)) return Retain(scope,Lease);
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
        // Store contexts start with fleet selection disabled. The role
        // recovery adapter needs all 120 fixed routes; enable only this
        // retained, fabric-role session after the guarded lease is ready.
        if (role) {
            fresh->io.fleetEnabled=true;
            if (!allowed(Lease)) return Retain(scope,Lease);
        }
#endif
        fStage=Ready;return true;
    }
public:
    template<class Scope>
    bool Prepare(PioneerProbeCookie* owner,PioneerStoreContext& old,
        pioneer_driver_recovery_guard_v2_info* guard,Scope& scope) {
        return PrepareWithEvidence(owner,old,guard,scope,old.recoveryFinishTicket,false);
    }
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
    template<class Scope>
    bool PrepareFabric(PioneerProbeCookie* owner,PioneerStoreContext& old,
        pioneer_driver_recovery_guard_v2_info* guard,Scope& scope) {
        return PrepareWithEvidence(owner,old,guard,scope,old.recoveryFabricStartTicket,true);
    }
    template<class Scope>
    bool PrepareFabricRole(PioneerProbeCookie* owner,PioneerStoreContext& old,
        pioneer_driver_recovery_guard_v2_info* guard,Scope& scope) {
        return PrepareWithEvidence(owner,old,guard,scope,old.recoveryFabricStartTicket,true,true);
    }
    // The scope must independently prove restoration of all role images,
    // local RAM and parent handoffs, plus closure of fresh role mappings.
    // This records an intermediate state only; nothing is released here.
    template<class Scope>
    bool VerifyRoleRestoration(pioneer_driver_recovery_guard_v2_info* guard,Scope& scope) {
        if (!fRole || fStage!=Ready || !fOld || !fFresh) return false;
        if (!RoleReady() || !TicketValid(guard)
            || !PioneerQ35FabricRoleRestorationMatches(*fOld,fTicket)
            || !scope.FreshRoleRecoveryRestored(*this,*fOld,*fFresh)
            || !TicketValid(guard) || !PioneerQ35FabricRoleRestorationMatches(*fOld,fTicket))
            return Retain(scope,RoleVerified);
        fRoleReceipt={fRoleBinding,fGraphOwner,fTicket};
        fStage=RoleVerified;return true;
    }
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
    // Called only after a separate fixed-projection run in this fresh session
    // has audited all workers, checked every output and restored its scratch.
    // The physical run, ownership scope and call site are not implemented.
    template<class Scope>
    bool RecordFabricMatrixQualification(pioneer_driver_recovery_guard_v2_info* guard,Scope& scope) {
        if (!fRole || !fFabric || fStage!=RoleVerified || !fOld || !fFresh || !TicketValid(guard)) return false;
        if (!fOld->recoveryFabricMatrixReceipt.Seal(*fOld,*this,fFresh->qwen35QualificationOwner,
                fTicket,guard,scope) || !TicketValid(guard)) return Retain(scope,RoleVerified);
        return true;
    }
#endif
#endif

    // Restore through this session's exact fresh resources and retained old
    // snapshot. The underlying owner writes and rereads every page and is
    // single-use; the surrounding scope supplies the missing live proofs.
    template<class Scope>
    bool RestoreScratch(pioneer_driver_recovery_guard_v2_info* guard,Scope& scope) {
        if (fStage!=Ready || !fOld || !fFresh
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
            || fRole
#endif
            ) return false;
        auto* scratch=fOld->qwen35QualificationOwner.Scratch();
        if (!TicketValid(guard) || !scratch
            || !scope.FreshRecoveryRestoreAdmission(*this,*fOld,*fFresh,fRestore)
            || !TicketValid(guard)) return Retain(scope,ScratchRestoring);
        fStage=ScratchRestoring;
        auto transferTicketValid=[&]() { return TicketValid(guard); };
        auto restore=[&](const auto& proof) {
            return fRestore.Restore(*scratch,fTicket,proof,fFresh->io,scope,
                fOld->banks,fOld->baseline.baseline.baseline.baseline.dram,
                fFresh->banks,fFresh->baseline.baseline.baseline.baseline.dram,
                fFresh->baseline.baseline.baseline.noc,fFresh->host,fFresh->lease,
                fFresh->initialized,PioneerResidentBanks::Pages,transferTicketValid);
        };
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
        const bool restored=fFabric?restore(fOld->recoveryFabricEvidence):restore(fOld->recoveryFinishProof);
#else
        const bool restored=restore(fOld->recoveryFinishProof);
#endif
        if (!restored) return Retain(scope,ScratchRestoring);
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
private:
    template<class Scope,class Receipt,class Verify,class PostLease>
    bool CloseTransportWith(pioneer_driver_recovery_guard_v2_info* guard,Scope& scope,
        const Receipt& receipt,Verify& verified,PostLease& postLease) {
        if (!TicketValid(guard) || !verified()
            || !scope.FreshRecoveryCloseAdmission(*this,*fOld,*fFresh,receipt)
            || !TicketValid(guard) || !verified()) return Retain(scope,Closing);
        fStage=Closing;
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
        if (fFabric) {
            bool lost=false;
            auto allowed=[&]() {
                if (lost || !TicketValid(guard) || !verified()
                    || !scope.FreshRecoveryLeaseAccessAllowed(*this,*fOld,*fFresh,receipt)
                    || !TicketValid(guard) || !verified()) { lost=true;return false; }
                return true;
            };
            // Role recovery selected many tiles. The lease belongs to root;
            // reselect it under the same ticket before touching its NIU.
            if (fRole && (!fFresh->lease.ready || fFresh->lease.pending || fFresh->lease.recovery_required
                || !PioneerQ35GuardedSelectRoleRoot(fFresh->io,allowed))) return Retain(scope,LeaseClosed);
            if (!PioneerQ35GuardedLeaseEnd(fFresh->io,fFresh->baseline.baseline.baseline.noc,
                fFresh->lease,allowed)) return Retain(scope,LeaseClosed);
        } else
#endif
        if (!PioneerResidentLeaseEnd(fFresh->io,fFresh->baseline.baseline.baseline.noc,fFresh->lease))
            return Retain(scope,LeaseClosed);
        fStage=LeaseClosed;
        if (!TicketValid(guard) || !verified() || !postLease()
            || !scope.FreshRecoveryPostTransferAudit(*this,*fOld,*fFresh,receipt)
            || !TicketValid(guard) || !verified()) return Retain(scope,FleetAudited);
        // Use a distinct fresh audit mapping, kept on the fresh context on
        // every failure. Audit both NoCs on all 120 held cores and host DMA,
        // then restore and unmap the temporary TLB apertures before PCI end.
        if (fFresh->recoveryMapping) return Retain(scope,FleetAudited);
        fFresh->recoveryMapping=new(std::nothrow) PioneerPostResetMapping(fFresh->profile);
        if (!fFresh->recoveryMapping) return Retain(scope,FleetAudited);
        auto& audit=*fFresh->recoveryMapping;
        tt_arc_scratch_result firmware{};PioneerPostResetAuditResult fleet{};PioneerQ35PostResetL2Result l2{};
        const auto& dram=fFresh->baseline.baseline.baseline.baseline.dram;
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
        if (fFabric) {
            // This callback is metadata-only: it must not touch PCI or the
            // shared apertures. Once revoked, the audit never accesses them
            // again, even if an authority check would later succeed.
            bool lost=false;
            auto allowed=[&]() {
                if (lost || !TicketValid(guard) || !verified()
                    || !scope.FreshRecoveryAuditAccessAllowed(*this,*fOld,*fFresh,receipt)
                    || !TicketValid(guard) || !verified()) { lost=true;return false; }
                return true;
            };
            if (!audit.OpenQ35Guarded(allowed)
                || !audit.ProbeQ35FabricFirmwareGuarded(100,firmware,allowed))
                return Retain(scope,FleetAudited);
            for (unsigned word=0;word<TT_DRAM_SOURCE_WORDS;word++)
                if (!allowed() || audit.topology.source[word]!=dram.source[word])
                    return Retain(scope,FleetAudited);
            if (!audit.AuditQ35Fabric(fleet,allowed) || !audit.AuditQ35L2CpuHeldGuarded(l2,allowed)
                || !audit.AuditQ35HostDmaGuarded(allowed)
                || !audit.RevalidateQ35FabricFirmware(firmware,allowed)
                || !audit.AuditQ35RootAerGuarded(allowed) || !audit.CloseQ35Guarded(allowed))
                return Retain(scope,FleetAudited);
        } else
#endif
        {
            if (!audit.Open() || !TicketValid(guard) || !audit.ProbeFirmware(100,firmware)
                || !TicketValid(guard)) return Retain(scope,FleetAudited);
            for (unsigned word=0;word<TT_DRAM_SOURCE_WORDS;word++)
                if (audit.topology.source[word]!=dram.source[word]) return Retain(scope,FleetAudited);
            if (!audit.Audit(fleet) || !TicketValid(guard) || !audit.AuditQ35L2CpuHeld(l2)
                || !TicketValid(guard) || !audit.AuditHostDma()
                || !TicketValid(guard) || !audit.Close()) return Retain(scope,FleetAudited);
        }
        delete fFresh->recoveryMapping;fFresh->recoveryMapping=nullptr;
        if (!TicketValid(guard) || !verified() || !postLease()
            || !scope.FreshRecoveryPostTransferAudit(*this,*fOld,*fFresh,receipt)
            || !TicketValid(guard) || !verified()) return Retain(scope,FleetAudited);
        fStage=FleetAudited;
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
        if (fFabric) {
            bool lost=false;
            auto allowed=[&]() {
                if (lost || !TicketValid(guard) || !verified()
                    || !scope.FreshRecoveryPciAccessAllowed(*this,*fOld,*fFresh,receipt)
                    || !TicketValid(guard) || !verified()) { lost=true;return false; }
                return true;
            };
            auto protectedArea=[&](int32_t area) {
                return fOld->io.ReferencesRecoveryHostArea(area)
                    || scope.FreshRecoveryAreaProtected(*this,*fOld,*fFresh,receipt,area);
            };
            auto unmap=[&]() {
                return !fFresh->lease.ready && !fFresh->lease.pending && !fFresh->lease.recovery_required
                    && fFresh->lease.l1_restored && fFresh->lease.niu_restored && fFresh->lease.tlb_restored
                    && fFresh->io.FreshRecoveryMmioUnmapReady()
                    && fFresh->io.DisposeFabricOldMmio(allowed,protectedArea)==B_OK;
            };
            if (!PioneerQ35GuardedMmioEnd(fFresh->io,fFresh->Pci(),fFresh->transaction,allowed,unmap))
                return Retain(scope,PciClosed);
        } else
#endif
        if (!PioneerQ38DispatchMmioEnd(fFresh->io,fFresh->Pci(),fFresh->transaction))
            return Retain(scope,PciClosed);
        fStage=PciClosed;
        if (!TicketValid(guard) || !verified()
            || !scope.FreshRecoveryPciIdle(*this,*fOld,*fFresh,receipt)
            || !TicketValid(guard) || !verified()) return Retain(scope,TransportClosed);
        fStage=TransportClosed;return true;
    }

public:
    template<class Scope>
    bool CloseTransport(pioneer_driver_recovery_guard_v2_info* guard,Scope& scope) {
        if (fStage!=ScratchVerified || !fOld || !fFresh) return false;
        auto verified=[&]() {
            auto* scratch=fOld->qwen35QualificationOwner.Scratch();
            return scratch && fRestore.VerifiedFor(*scratch,fTicket,fOld->banks,fFresh->banks);
        };
        auto postLease=[]() { return true; };
        return CloseTransportWith(guard,scope,fRestore,verified,postLease);
    }
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
    // This closes only the fresh transport. The scope must prove the role and
    // parent graph were restored, their mappings closed, and the root lease
    // close cannot invalidate either; it must revalidate after lease close.
    // The old owner, snapshots, host pages and PCI reservation stay retained.
    template<class Scope>
    bool CloseRoleTransport(pioneer_driver_recovery_guard_v2_info* guard,Scope& scope) {
        if (!fRole || !fFabric || fStage!=RoleVerified || !fOld || !fFresh) return false;
        auto verified=[&]() {
            return fRoleReceipt.ticket==fTicket && fRoleReceipt.binding==fRoleBinding
                && fRoleReceipt.graph==fGraphOwner && fOld->qwen35SharedRoleBinding==fRoleBinding
                && fOld->qwen35GraphOwner==fGraphOwner && fFresh->io.fleetEnabled;
        };
        if (!TicketValid(guard) || !verified()
            || !scope.FreshRoleRecoveryCloseAdmission(*this,*fOld,*fFresh,fRoleReceipt)
            || !TicketValid(guard) || !verified()) return Retain(scope,Closing);
        auto postLease=[&]() {
            return scope.FreshRoleRecoveryPostLeaseAudit(*this,*fOld,*fFresh,fRoleReceipt);
        };
        return CloseTransportWith(guard,scope,fRoleReceipt,verified,postLease);
    }
    // Release only the old quarantined DMA areas after the fresh transport
    // is closed. Scope must prove DMA cannot restart, both retained owners no
    // longer reference these buffers, and every other mapping is disjoint.
    // Admission, access, area and completion callbacks must be metadata-only;
    // the device mappings are already closed at this point.
    // The owners, snapshots, fresh host page and PCI reservation stay pinned.
    template<class Scope>
    bool DisposeRoleOldHostBuffers(pioneer_driver_recovery_guard_v2_info* guard,Scope& scope) {
        if (!fRole || !fFabric || fStage!=TransportClosed || !fOld || !fFresh) return false;
        auto receiptValid=[&]() {
            return fRoleReceipt.ticket==fTicket && fRoleReceipt.binding==fRoleBinding
                && fRoleReceipt.graph==fGraphOwner && fOld->qwen35SharedRoleBinding==fRoleBinding
                && fOld->qwen35GraphOwner==fGraphOwner && fOld->io.fabricOldMmioDisposeComplete
                && fFresh->io.fabricOldMmioDisposeComplete && !fFresh->recoveryMapping;
        };
        if (!TicketValid(guard) || !receiptValid()
            || !scope.FreshRoleOldHostDisposeAdmission(*this,*fOld,*fFresh,fRoleReceipt)
            || !TicketValid(guard) || !receiptValid()) return Retain(scope,RoleHostDisposed);
        bool lost=false;
        auto allowed=[&]() {
            if (lost || !TicketValid(guard) || !receiptValid()
                || !scope.FreshRoleOldHostAccessAllowed(*this,*fOld,*fFresh,fRoleReceipt)
                || !TicketValid(guard) || !receiptValid()) { lost=true;return false; }
            return true;
        };
        auto protectedArea=[&](int32_t area) {
            return fFresh->io.ReferencesRecoveryHostArea(area)
                || scope.FreshRoleOldHostAreaProtected(*this,*fOld,*fFresh,fRoleReceipt,area);
        };
        if (fOld->io.DisposeFabricHostBuffers(allowed,protectedArea)!=B_OK
            || !allowed() || !fOld->io.fabricHostDisposeComplete
            || !scope.FreshRoleOldHostDisposeComplete(*this,*fOld,*fFresh,fRoleReceipt)
            || !allowed() || !fOld->io.fabricHostDisposeComplete) return Retain(scope,RoleHostDisposed);
        fStage=RoleHostDisposed;return true;
    }
    // The old-MMIO flag alone excludes host pages and role mappings. Publish
    // the broader evidence only after the separately guarded host disposal,
    // fresh transport close, and scope proof that all old mappings are gone.
    // Admission and completion callbacks must be metadata-only. No owner or
    // PCI reservation is released by this intermediate receipt.
    template<class Scope>
    bool PublishRoleMappingsDisposed(pioneer_driver_recovery_guard_v2_info* guard,Scope& scope) {
        if (!fRole || !fFabric || fStage!=RoleHostDisposed || fRoleMappingsEvidencePublished
            || !fOld || !fFresh) return false;
        auto ready=[&]() {
            return fRoleReceipt.ticket==fTicket && fRoleReceipt.binding==fRoleBinding
                && fRoleReceipt.graph==fGraphOwner && fOld->qwen35SharedRoleBinding==fRoleBinding
                && fOld->qwen35GraphOwner==fGraphOwner && fOld->io.fabricOldMmioDisposeComplete
                && fOld->io.fabricHostDisposeComplete && fFresh->io.fabricOldMmioDisposeComplete
                && !fFresh->recoveryMapping && fFresh->Pci().restored && !fFresh->transaction.config_attempted;
        };
        if (!TicketValid(guard) || !ready() || !fOld->recoveryFabricEvidence.ReadyForScratchRestore()
            || !scope.FreshRoleMappingsDisposedAdmission(*this,*fOld,*fFresh,fRoleReceipt)
            || !TicketValid(guard) || !ready()) return Retain(scope,RoleMappingsProved);
        auto& evidence=fOld->recoveryFabricEvidence;
        const uint32_t prior=evidence.flags;
        evidence.flags=prior | PIONEER_FABRIC_PROOF_OLD_MAPPINGS_DISPOSED;
        fRoleMappingsEvidencePublished=true;
        if (!TicketValid(guard) || !ready() || !evidence.ReadyToRetireOneOwner()
            || !scope.FreshRoleMappingsDisposedComplete(*this,*fOld,*fFresh,fRoleReceipt)
            || !TicketValid(guard) || !ready()) {
            evidence.flags=prior;fRoleMappingsEvidencePublished=false;
            return Retain(scope,RoleMappingsProved);
        }
        fStage=RoleMappingsProved;return true;
    }

    // Retire the resident graph's retained run after both role and parent
    // restoration and disposal of all old mappings. This does not destroy
    // the graph or role owner: both are still referenced by this session.
    // A failed or interrupted release is never replayed automatically.
    template<class Scope>
    bool RetireRoleGraph(pioneer_driver_recovery_guard_v2_info* guard,Scope& scope) {
        if (!fRole || !fFabric || fStage!=RoleMappingsProved || fRoleGraphRetireAttempted
            || !fOld || !fFresh) return false;
        auto ready=[&]() {
            return fRoleReceipt.ticket==fTicket && fRoleReceipt.binding==fRoleBinding
                && fRoleReceipt.graph==fGraphOwner && fOld->qwen35SharedRoleBinding==fRoleBinding
                && fOld->qwen35GraphOwner==fGraphOwner && fOld->io.fabricOldMmioDisposeComplete
                && fOld->io.fabricHostDisposeComplete && fFresh->io.fabricOldMmioDisposeComplete
                && !fFresh->recoveryMapping && fFresh->Pci().restored && !fFresh->transaction.config_attempted
                && fOld->recoveryFabricEvidence.ReadyToRetireOneOwner();
        };
        if (!TicketValid(guard) || !ready()
            || !scope.FreshRoleGraphRetirementAdmission(*this,*fOld,*fFresh,fRoleReceipt)
            || !TicketValid(guard) || !ready()) return Retain(scope,RoleGraphRetired);
        fRoleGraphRetireAttempted=true;
        if (!PioneerQ35GraphOwnerReleaseAfterFabric(fOld->qwen35GraphOwner,
                fOld->recoveryFabricEvidence,fTicket,guard)
            || !TicketValid(guard) || !ready()
            || !scope.FreshRoleGraphRetirementComplete(*this,*fOld,*fFresh,fRoleReceipt)
            || !TicketValid(guard) || !ready()) return Retain(scope,RoleGraphRetired);
        fStage=RoleGraphRetired;return true;
    }

    // The fresh DMA page is released only after graph-run retirement. The
    // admission callback must independently prove that device DMA cannot
    // restart and that no retained owner or mapping refers to this page.
    // A failed delete or lost post-delete authority is never replayed.
    template<class Scope>
    bool ReleaseRoleFreshHostPage(pioneer_driver_recovery_guard_v2_info* guard,Scope& scope) {
        if (!fRole || !fFabric || fStage!=RoleGraphRetired || fRoleFreshHostReleaseAttempted
            || !fOld || !fFresh) return false;
        auto ready=[&]() {
            return fRoleReceipt.ticket==fTicket && fRoleReceipt.binding==fRoleBinding
                && fRoleReceipt.graph==fGraphOwner && fOld->qwen35SharedRoleBinding==fRoleBinding
                && fOld->qwen35GraphOwner==fGraphOwner && fOld->io.fabricHostDisposeComplete
                && fOld->recoveryFabricEvidence.ReadyToRetireOneOwner()
                && !fFresh->recoveryMapping && fFresh->Pci().restored && !fFresh->transaction.config_attempted
                && !fFresh->lease.ready && !fFresh->lease.pending && !fFresh->lease.recovery_required
                && fFresh->lease.l1_restored && fFresh->lease.niu_restored && fFresh->lease.tlb_restored
                && fFresh->host.state==TT_HOST_PAGE_READY && fFresh->host.allocated && !fFresh->host.released
                && fFresh->io.FreshRecoveryHostReleaseReady();
        };
        if (!TicketValid(guard) || !ready()
            || !scope.FreshRoleHostReleaseAdmission(*this,*fOld,*fFresh,fRoleReceipt)
            || !TicketValid(guard) || !ready()) return Retain(scope,RoleFreshHostReleased);
        fRoleFreshHostReleaseAttempted=true;
        fFresh->host.release_status=fFresh->io.ReleaseHostPage();
        fFresh->host.released=fFresh->host.release_status==B_OK;
        if (!fFresh->host.released || !fFresh->io.FreshRecoveryHostReleased() || !TicketValid(guard)
            || !scope.FreshRoleHostReleaseComplete(*this,*fOld,*fFresh,fRoleReceipt)
            || !TicketValid(guard) || !fFresh->host.released || fFresh->host.release_status!=B_OK
            || !fFresh->io.FreshRecoveryHostReleased()) return Retain(scope,RoleFreshHostReleased);
        fStage=RoleFreshHostReleased;return true;
    }
    // Record role-owner retirement without destroying it: TicketValid and
    // the graph still reference this owner until the session is detached.
    template<class Scope>
    bool RecordRoleOwnerRetirement(pioneer_driver_recovery_guard_v2_info* guard,Scope& scope) {
        if (!RoleOwnerRetirementReady(fTicket)) return false;
        auto allowed=[&]() {
            return TicketValid(guard) && RoleOwnerRetirementReady(fTicket)
                && scope.FreshRoleOwnerRetirementAllowed(*this,*fOld,*fFresh,fRoleReceipt)
                && TicketValid(guard) && RoleOwnerRetirementReady(fTicket);
        };
        if (!allowed() || !PioneerQ35RecordFabricRoleRetirement(*fOld,fTicket,allowed)
            || !allowed()) return Retain(scope,RoleOwnerRetired);
        fStage=RoleOwnerRetired;return true;
    }
    // Detach only after both retained owners independently confirm their
    // matching retirement receipts. The role, graph, snapshots, old bank and
    // PCI reservation remain with the old context. The metadata-only scope
    // must prove no remaining reference to fresh-session memory and continuing
    // physical exclusion; it must not access hardware in its callback.
    // On success this deletes the session and its fresh context; the caller
    // must discard the pointer and separately complete owner/reservation cleanup.
    template<class Scope>
    bool RetireRoleSession(pioneer_driver_recovery_guard_v2_info* guard,Scope& scope) {
        if (!RoleSessionRetirementReady(fTicket)) return false;
        auto ready=[&]() {
            return TicketValid(guard) && RoleSessionRetirementReady(fTicket)
                && PioneerQ35FabricRoleRetirementMatches(*fOld,fTicket);
        };
        if (!ready() || !scope.FreshRoleSessionRetirementAllowed(*this,*fOld,*fFresh,fRoleReceipt)
            || !ready() || !PioneerQ35PrepareFabricRoleSessionDetach(*fOld,fTicket,ready)
            || !ready()) return Retain(scope,RoleOwnerRetired);
        fOld->recoveryFreshSession=nullptr;
        delete this;return true;
    }
#endif

    // Retire only after the post-transfer fleet/DMA audit and PCI idle
    // transition. The exact old snapshot stays alive through old-bank
    // destruction; the fresh bank is destroyed first. A failed attempt
    // keeps the original context and all still-owned resources retained.
    // On success this deletes the fresh session, so the caller must discard
    // its pointer and revalidate the recovery guard before generic finish.
    template<class Scope>
    bool Retire(pioneer_driver_recovery_guard_v2_info* guard,Scope& scope) {
        if (fStage!=TransportClosed || !fOld || !fFresh
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
            || fRole // role/graph owner retirement has a separate contract
#endif
            ) return false;
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
        auto retire=[&](const auto& proof) {
            return fOld->qwen35QualificationOwner.RetireModifiedAfterReset(proof,fTicket,
                fRestore,fOld->banks,fFresh->banks,*this,scope);
        };
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
        const bool retired=fFabric?retire(fOld->recoveryFabricEvidence):retire(fOld->recoveryFinishProof);
#else
        const bool retired=retire(fOld->recoveryFinishProof);
#endif
        if (!retired) return Retain(scope,OwnerRetired);
        fStage=OwnerRetired;
        if (!RetiredTicketValid(guard,scratch,snapshot)
            || !scope.FreshRecoveryAfterOwnerRetired(*this,*fOld,*fFresh,fRestore)
            || !RetiredTicketValid(guard,scratch,snapshot)) return Retain(scope,OwnerRetired);
        fOld->recoveryFreshSession=nullptr;
        delete this;return true;
    }
};
