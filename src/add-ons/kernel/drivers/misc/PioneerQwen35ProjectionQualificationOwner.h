/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_PROJECTION_QUALIFICATION_OWNER_H
#define PIONEER_QWEN35_PROJECTION_QUALIFICATION_OWNER_H

#include "PioneerQwen35ProjectionScratch.h"
#include "PioneerDriverReloadState.h"
#include "../../../../../tools/pioneer/PioneerQwen35DataflowProjectionQualificationService.h"
#include <new>
#include <stdlib.h>

// The store context owns this holder as a member declared BEFORE its bank
// owner, so C++ destroys the sticky bank claim before these pointees. An
// active holder blocks ordinary context teardown; only a proven clean retire
// or a separate verified recovery may permit destruction. This holder never
// grants permission to touch the card or clears a hardware fault.
class PioneerQ35ProjectionQualificationOwner {
    PioneerQ35ProjectionScratch* scratch=nullptr;
    PioneerQwen35::DataflowProjectionQualificationService* qualification=nullptr;
    uint32_t* snapshot=nullptr;
    bool attempted=false,active=false,retired=false,retained=false;
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
    bool retirementPrepared=false;
#endif
public:
    enum ScratchOperation : unsigned { Capture=0,Stage=1,Verify=2,Restore=3 };
    PioneerQ35ProjectionQualificationOwner()=default;
    PioneerQ35ProjectionQualificationOwner(const PioneerQ35ProjectionQualificationOwner&)=delete;
    PioneerQ35ProjectionQualificationOwner& operator=(const PioneerQ35ProjectionQualificationOwner&)=delete;
    ~PioneerQ35ProjectionQualificationOwner() {
        delete qualification;delete scratch;free(snapshot);
    }
    bool Allocate() {
        if (attempted) return false;
        attempted=true;
        scratch=new(std::nothrow) PioneerQ35ProjectionScratch;
        qualification=new(std::nothrow) PioneerQwen35::DataflowProjectionQualificationService;
        snapshot=static_cast<uint32_t*>(malloc(PioneerQ35ProjectionScratch::SavedWords*sizeof(uint32_t)));
        if (!scratch || !qualification || !snapshot) {
            delete qualification;qualification=nullptr;delete scratch;scratch=nullptr;free(snapshot);snapshot=nullptr;
            return false;
        }
        active=true;return true;
    }
    bool Active() const { return active && !retired; }
    bool Retained() const { return retained; }
    bool Retired() const { return retired; }
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
    bool RetirementPrepared() const { return Active() && retirementPrepared && !retained; }
#endif
    bool Owns(const PioneerQ35ProjectionScratch& candidate) const { return Active() && scratch==&candidate; }
    PioneerQ35ProjectionScratch* Scratch() const { return Active()?scratch:nullptr; }
    PioneerQwen35::DataflowProjectionQualificationService* Qualification() const {
        return Active()?qualification:nullptr;
    }
    uint32_t* Snapshot() const { return Active()?snapshot:nullptr; }
    void Retain() { if (active && !retired) retained=true; }

    // A diagnostic that never staged scratch has not written its claimed
    // bank pages. This does not authorize a reset: the caller must separately
    // hold the recovery ticket and prove device, firmware and DMA recovery.
    bool UnmodifiedResetEligible(const PioneerResidentBanks& banks,const tt_dram_samples_result& dram,
        const PioneerResidentLease& lease) const {
        if (!Active() || !scratch || !qualification || !snapshot || !qualification->IsUnstarted()
            || banks.Quarantined() || !lease.ready || lease.pending || lease.recovery_required) return false;
        if (scratch->IsEmpty()) return !banks.Q35StreamClaimed();
        return scratch->IsCaptured()
            && banks.Q35StreamOwnerMatches(dram,scratch,PioneerQwen35::DataflowCoreCount);
    }
    // Retire the inert owner only after the exact verified reset ticket and
    // disposal of old mappings. Its storage stays alive until bank teardown.
    bool RetireUnmodifiedAfterReset(const PioneerDriverRecoveryProof& proof,uint64_t ticket,
        bool oldMappingsDisposed,const PioneerResidentBanks& banks,const tt_dram_samples_result& dram,
        const PioneerResidentLease& lease) {
        if (!ticket || !oldMappingsDisposed || !proof.ReadyToRetireOneOwner()
            || proof.reset_generation!=ticket || proof.firmware_generation!=ticket
            || !UnmodifiedResetEligible(banks,dram,lease)) return false;
        retained=true;retired=true;return true;
    }

#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
    // Metadata retirement after a separate fresh session restored and reread
    // the modified scratch. The scope must prove that session's post-transfer
    // fleet/DMA audit, idle PCI state and host-page release under the same
    // retained ticket. The old claim and snapshot remain allocated until
    // their containing context is destroyed; this never clears the cold latch.
    template<class Receipt,class Session,class Scope>
    bool RetireModifiedAfterReset(const PioneerDriverRecoveryProof& proof,uint64_t ticket,
        const Receipt& receipt,const PioneerResidentBanks& oldBanks,const PioneerResidentBanks& freshBanks,
        const Session& session,Scope& scope) {
        if (!Active() || !scratch || !snapshot || !ticket || !proof.ReadyToRetireOneOwner()
            || proof.reset_generation!=ticket || proof.firmware_generation!=ticket
            || !scratch->ResetRestoreEligible(snapshot)
            || !receipt.VerifiedFor(*scratch,ticket,oldBanks,freshBanks)
            || !scope.QualificationModifiedResetRetirementAllowed(*this,*scratch,receipt,session)
            || !scratch->ResetRestoreEligible(snapshot)
            || !receipt.VerifiedFor(*scratch,ticket,oldBanks,freshBanks)) return false;
        retained=true;retired=true;return true;
    }
    bool RetiredModifiedIdentity(const PioneerQ35ProjectionScratch* expected,const uint32_t* saved) const {
        return active && retired && retained && expected && expected==scratch && saved && saved==snapshot;
    }
#endif

#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
    // Prepare while the fleet remains held and the exact sticky bank claim
    // and restored scratch are still owned. This does not free anything or
    // authorize resuming hardware; a separate scope proof does that.
    template<class Scope> bool PrepareRetirement(Scope& scope,const PioneerResidentBanks& banks,
        const tt_dram_samples_result& dram) {
        if (!Active() || retained || retirementPrepared || !scratch || !qualification
            || !scratch->IsRestored() || !qualification->IsAudited()
            || !scratch->MatchesBankClaimOwner(banks,dram)
            || !scope.QualificationOwnerPrepareRetirementAllowed(*this,*scratch,*qualification)) return false;
        retirementPrepared=true;return true;
    }
#endif

    // This permits session teardown only; the sticky claim and its owner
    // remain alive until the bank object itself is destroyed. The scope must
    // prove service close, drained transfers and exclusive session identity.
    template<class Scope> bool Retire(Scope& scope,const PioneerResidentBanks& banks,
        const tt_dram_samples_result& dram) {
        if (!Active() || retained || !scratch || !qualification || !scratch->IsRestored()
            || !qualification->IsAudited() || !scratch->MatchesBankClaimOwner(banks,dram)
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
            || !retirementPrepared
#endif
            || !scope.QualificationOwnerRetirementAllowed(*this,*scratch,*qualification)) return false;
        retired=true;return true;
    }
    // Allocation by itself has not changed a bank. A caller may abandon only
    // before a claim or scratch access, with independent scope approval.
    template<class Scope> bool AbandonUnclaimed(Scope& scope,const PioneerResidentBanks& banks) {
        if (!Active() || retained || !scratch || !scratch->IsEmpty() || banks.Q35StreamClaimed()
            || !scope.QualificationOwnerAbandonAllowed(*this)) return false;
        retired=true;return true;
    }
    // Snapshot completed but staging never began, so the claimed bank pages
    // remain unchanged. Keep the owner alive until bank-context destruction.
    template<class Scope> bool AbandonCapturedUnmodified(Scope& scope,const PioneerResidentBanks& banks,
        const tt_dram_samples_result& dram) {
        if (!Active() || retained || !scratch || !qualification || !scratch->IsCaptured()
            || !qualification->IsUnstarted() || !scratch->MatchesBankClaimOwner(banks,dram)
            || !scope.QualificationOwnerAbandonCapturedAllowed(*this,*scratch,*qualification)) return false;
        retired=true;return true;
    }
    // A staged diagnostic may be cancelled before any command begins, after
    // every overwritten scratch page has been restored and rechecked.
    template<class Scope> bool AbandonRestoredUnrun(Scope& scope,const PioneerResidentBanks& banks,
        const tt_dram_samples_result& dram) {
        if (!Active() || retained || !scratch || !qualification || !scratch->IsRestored()
            || !qualification->IsUnstarted() || !scratch->MatchesBankClaimOwner(banks,dram)
            || !scope.QualificationOwnerAbandonRestoredAllowed(*this,*scratch,*qualification)) return false;
        retired=true;return true;
    }
};
#endif
