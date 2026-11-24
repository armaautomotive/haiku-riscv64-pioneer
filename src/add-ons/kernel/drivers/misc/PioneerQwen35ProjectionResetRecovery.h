/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_PROJECTION_RESET_RECOVERY_H
#define PIONEER_QWEN35_PROJECTION_RESET_RECOVERY_H
#if !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || !defined(PIONEER_QWEN35_DEVICE_DISPATCH) \
    || !defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) || defined(PIONEER_QWEN35_RECOVERY_QUALIFY)
#error Projection reset restore is restricted to the non-loadable snapshot witness
#endif
#include "PioneerQwen35ProjectionScratch.h"
#include "PioneerDriverReloadState.h"

// Heap-owned by a future locked recovery session. This restores only the
// complete saved diagnostic scratch to a separately leased post-reset bank.
// Scope must prove the exact ticket, card/bank identity across reset, fresh
// mapping, exclusive writers, and transfer authority at every call. A receipt
// alone never clears the cold latch, frees either bank claim or authorizes a
// reset. This is not an inference path.
class PioneerQ35ProjectionResetRecovery {
    using Fixture=PioneerQwen35::DataflowProjectionQualification;
    enum Phase { PhaseFresh, PhaseFaulted, PhaseVerified } phase=PhaseFresh;
    const PioneerQ35ProjectionScratch* owner=nullptr;
    const PioneerResidentBanks *oldOwner=nullptr,*newOwner=nullptr;
    uint64_t resetTicket=0;
    unsigned failedPage=Fixture::ScratchPages;bool failedWrite=false;
    uint32_t page[Fixture::PageBytes/sizeof(uint32_t)]{};
    tt_resident_store_result result{};
    PioneerResidentStoreTrace trace{};
    template<class Scope>
    bool Retain(Scope& scope,PioneerQ35ProjectionScratch& scratch,
        PioneerResidentBanks& oldBanks,PioneerResidentBanks& newBanks) {
        oldBanks.Quarantine();newBanks.Quarantine();scope.RetainRecovery(scratch);return false;
    }
public:
    PioneerQ35ProjectionResetRecovery()=default;
    PioneerQ35ProjectionResetRecovery(const PioneerQ35ProjectionResetRecovery&)=delete;
    PioneerQ35ProjectionResetRecovery& operator=(const PioneerQ35ProjectionResetRecovery&)=delete;
    bool Faulted() const { return phase==PhaseFaulted; }
    unsigned FailedPage() const { return failedPage; }
    bool FailedWrite() const { return failedWrite; }
    const tt_resident_store_result& LastTransfer() const { return result; }
    const PioneerResidentStoreTrace& LastTrace() const { return trace; }
    bool VerifiedFor(const PioneerQ35ProjectionScratch& scratch,uint64_t ticket,
        const PioneerResidentBanks& oldBanks,const PioneerResidentBanks& newBanks) const {
        return phase==PhaseVerified && owner==&scratch && ticket && ticket==resetTicket
            && oldOwner==&oldBanks && newOwner==&newBanks && !newBanks.Quarantined()
            && oldBanks.Q35StreamClaimMatches(&scratch,scratch.ranges,PioneerQwen35::DataflowCoreCount)
            && newBanks.Q35StreamClaimMatches(&scratch,scratch.ranges,PioneerQwen35::DataflowCoreCount);
    }
    template<class IO,class Scope>
    bool Restore(PioneerQ35ProjectionScratch& scratch,uint64_t ticket,const PioneerDriverRecoveryProof& proof,
        IO& io,Scope& scope,PioneerResidentBanks& oldBanks,const tt_dram_samples_result& oldDram,
        PioneerResidentBanks& newBanks,const tt_dram_samples_result& newDram,
        const tt_compute_noc_result& noc,const tt_host_page_result& host,PioneerResidentLease& lease,
        const uint8_t* initialized,unsigned initializedPages) {
        if (phase!=PhaseFresh || !ticket || !proof.ReadyToRetireOneOwner()
            || proof.reset_generation!=ticket || proof.firmware_generation!=ticket
            || &oldBanks==&newBanks
            || !oldBanks.Q35RecoveryRoutesMatch(oldDram,newBanks,newDram)
            || !oldBanks.Q35StreamClaimMatches(&scratch,scratch.ranges,PioneerQwen35::DataflowCoreCount)
            || !scratch.ResetRestoreEligible(scratch.saved)
            || !newBanks.Matches(newDram) || newBanks.Q35StreamClaimed() || !initialized
            || initializedPages!=PioneerResidentBanks::Pages || !lease.ready || lease.pending || lease.recovery_required
            || !scope.RecoveryAdmissionAllowed(scratch,io,oldBanks,oldDram,newBanks,newDram,noc,host,lease,ticket,
                scratch.saved,PioneerQ35ProjectionScratch::SavedWords,initialized,initializedPages))
            return false;
        owner=&scratch;oldOwner=&oldBanks;newOwner=&newBanks;resetTicket=ticket;phase=PhaseFaulted;
        if (!newBanks.ReserveQ35Stream(newDram,&scratch,scratch.ranges,PioneerQwen35::DataflowCoreCount,
                initialized,initializedPages)) {
            return Retain(scope,scratch,oldBanks,newBanks);
        }
        for (unsigned pass=0;pass<2;pass++) {
            for (unsigned n=0;n<Fixture::ScratchPages;n++) {
                const bool write=pass==0;const unsigned offset=scratch.firstOffset+n*Fixture::PageBytes;
                failedPage=n;failedWrite=write;
                if (!oldBanks.Q35StreamClaimMatches(&scratch,scratch.ranges,PioneerQwen35::DataflowCoreCount)
                    || !oldBanks.Q35RecoveryRoutesMatch(oldDram,newBanks,newDram)
                    || !newBanks.Q35StreamClaimMatches(&scratch,scratch.ranges,PioneerQwen35::DataflowCoreCount)
                    || !newBanks.Matches(newDram)
                    || !scope.RecoveryTransferAllowed(scratch,io,oldBanks,newBanks,noc,host,lease,ticket,scratch.saved,
                        scratch.bank,offset,write)
                    || !oldBanks.Q35RecoveryRoutesMatch(oldDram,newBanks,newDram)) {
                    return Retain(scope,scratch,oldBanks,newBanks);
                }
                if (write) memcpy(page,scratch.saved+n*(Fixture::PageBytes/4),Fixture::PageBytes);
                if (!PioneerResidentStorePage(io,newDram,noc,host,lease,PioneerResidentBanks::Capacity,
                        offset,write,page,result,&newBanks,scratch.bank,&scratch,&trace)) {
                    return Retain(scope,scratch,oldBanks,newBanks);
                }
                if (!write && memcmp(page,scratch.saved+n*(Fixture::PageBytes/4),Fixture::PageBytes)!=0) {
                    return Retain(scope,scratch,oldBanks,newBanks);
                }
            }
        }
        if (!oldBanks.Q35StreamClaimMatches(&scratch,scratch.ranges,PioneerQwen35::DataflowCoreCount)
            || !oldBanks.Q35RecoveryRoutesMatch(oldDram,newBanks,newDram)
            || !newBanks.Q35StreamClaimMatches(&scratch,scratch.ranges,PioneerQwen35::DataflowCoreCount)
            || !newBanks.Matches(newDram) || !lease.ready || lease.pending || lease.recovery_required
            || !scope.RecoveryCompleteAllowed(scratch,io,oldBanks,newBanks,noc,host,lease,ticket,scratch.saved)
            || !oldBanks.Q35StreamClaimMatches(&scratch,scratch.ranges,PioneerQwen35::DataflowCoreCount)
            || !oldBanks.Q35RecoveryRoutesMatch(oldDram,newBanks,newDram)
            || !newBanks.Q35StreamClaimMatches(&scratch,scratch.ranges,PioneerQwen35::DataflowCoreCount)
            || !newBanks.Matches(newDram) || !lease.ready || lease.pending || lease.recovery_required) {
            return Retain(scope,scratch,oldBanks,newBanks);
        }
        failedPage=Fixture::ScratchPages;failedWrite=false;phase=PhaseVerified;return true;
    }
};
#endif
