/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_PROJECTION_SCRATCH_H
#define PIONEER_QWEN35_PROJECTION_SCRATCH_H

#ifndef PIONEER_QWEN35_DEVICE_DISPATCH
#error Fixed projection scratch requires the private Q35 bank reservation
#endif
#include "PioneerResidentStore.h"
#include "../../../../../tools/pioneer/PioneerQwen35DataflowProjectionQualification.h"
#include <string.h>

enum PioneerQ35ScratchFailureKind : unsigned {
    PioneerQ35ScratchFailureNone=0, PioneerQ35ScratchFailureAdmission=1,
    PioneerQ35ScratchFailureTransfer=2, PioneerQ35ScratchFailureFill=3,
    PioneerQ35ScratchFailureRestoreMismatch=4
};
enum PioneerQ35ScratchPhase : unsigned {
    PioneerQ35ScratchEmpty=0, PioneerQ35ScratchCapturing=1, PioneerQ35ScratchCaptured=2,
    PioneerQ35ScratchStaging=3, PioneerQ35ScratchStaged=4, PioneerQ35ScratchRunning=5,
    PioneerQ35ScratchStopped=6, PioneerQ35ScratchRestoring=7, PioneerQ35ScratchRestored=8,
    PioneerQ35ScratchFailed=9
};
struct PioneerQ35ScratchFailure {
    unsigned kind=0,phase=0,bank=0,page=0,offset=0,write=0,capturedPages=0,dirtyPages=0;
    bool leasePending=false,leaseRecoveryRequired=false;
    tt_resident_store_result transfer{};
    PioneerResidentStoreTrace trace{};
};
struct PioneerQ35ScratchObservation {
    unsigned step=0;
    bool write=false;
    uint32_t counters[4]{};
};

// Heap-owned by the exclusive diagnostic session until its entire bank
// context is destroyed. ReserveQ35Stream is sticky: even after successful
// restoration this object must outlive the bank claim. Scope must prove the
// locked session and held worker-zero transfer resources for every page, and
// must independently prove the resident fleet stopped before MarkStopped.
// RetainScratch must retain the owner and snapshots and quarantine the
// session when a transfer or its authorization becomes uncertain.
// This is not authority to start a worker or an inference fallback.
class PioneerQ35ProjectionScratch {
    using Fixture=PioneerQwen35::DataflowProjectionQualification;
    enum Phase { Empty=PioneerQ35ScratchEmpty, Capturing=PioneerQ35ScratchCapturing,
        Captured=PioneerQ35ScratchCaptured, Staging=PioneerQ35ScratchStaging, Staged=PioneerQ35ScratchStaged,
        Running=PioneerQ35ScratchRunning, Stopped=PioneerQ35ScratchStopped, Restoring=PioneerQ35ScratchRestoring,
        Restored=PioneerQ35ScratchRestored, Failed=PioneerQ35ScratchFailed };
    Phase phase=Empty;
    unsigned bank=0,firstOffset=0,capturedPages=0,dirtyPages=0;
    uint32_t* saved=nullptr;
    uint32_t page[Fixture::PageBytes/sizeof(uint32_t)]{};
    PioneerQ35StreamBankRange ranges[PioneerQwen35::DataflowCoreCount]{};
    bool outputVerified=false,transferFailed=false;
    PioneerQ35ScratchFailure failure{};

    void RecordFailure(unsigned kind,unsigned index,bool write,const PioneerResidentLease& lease,
        const tt_resident_store_result* result=nullptr,const PioneerResidentStoreTrace* trace=nullptr) {
        failure.kind=kind;failure.phase=unsigned(phase);failure.bank=bank;failure.page=index;
        failure.offset=index<Fixture::ScratchPages?firstOffset+index*Fixture::PageBytes:0;
        failure.write=write;failure.capturedPages=capturedPages;failure.dirtyPages=dirtyPages;
        failure.leasePending=lease.pending;failure.leaseRecoveryRequired=lease.recovery_required;
        if (result) failure.transfer=*result;
        if (trace) failure.trace=*trace;
    }

    template<class IO,class Scope>
    bool Transfer(IO& io,Scope& scope,const tt_dram_samples_result& dram,
        const tt_compute_noc_result& noc,const tt_host_page_result& host,
        PioneerResidentLease& lease,PioneerResidentBanks& banks,unsigned index,
        bool write,uint32_t* words) {
        if (index>=Fixture::ScratchPages || !words
            || !scope.ScratchTransferAllowed(this,bank,firstOffset+index*Fixture::PageBytes,write)) {
            RecordFailure(PioneerQ35ScratchFailureAdmission,index,write,lease);
            phase=Failed;scope.RetainScratch(this);return false;
        }
        tt_resident_store_result result{};PioneerResidentStoreTrace trace{};
        const bool okay=PioneerResidentStorePage(io,dram,noc,host,lease,
            PioneerResidentBanks::Capacity,firstOffset+index*Fixture::PageBytes,
            write,words,result,&banks,bank,this,&trace);
        if (!okay) {
            // A failed transfer can leave a posted operation. Never retry or
            // free the claim/snapshot under this transaction's own authority.
            RecordFailure(PioneerQ35ScratchFailureTransfer,index,write,lease,&result,&trace);
            transferFailed=true;phase=Failed;scope.RetainScratch(this);
        }
        return okay;
    }

    bool FillPage(unsigned index) {
        memset(page,0,sizeof(page));
        uint8_t* bytes=reinterpret_cast<uint8_t*>(page);
        if (index<Fixture::WeightPages)
            return Fixture::FillWeightBytes(index*Fixture::PageBytes,bytes,Fixture::PageBytes);
        index-=Fixture::WeightPages;
        if (index<Fixture::InputPages) {
            const unsigned offset=index*Fixture::PageBytes;
            const unsigned remaining=Fixture::InputBytes-offset;
            return Fixture::FillInputBytes(offset,bytes,remaining<Fixture::PageBytes?remaining:Fixture::PageBytes);
        }
        index-=Fixture::InputPages;
        if (index>=Fixture::OutputPages) return false;
        const unsigned offset=index*Fixture::PageBytes;
        const unsigned remaining=Fixture::OutputBytes-offset;
        return Fixture::FillOutputBytes(offset,bytes,remaining<Fixture::PageBytes?remaining:Fixture::PageBytes);
    }
public:
    static constexpr unsigned SavedWords=Fixture::ScratchPages*Fixture::PageBytes/sizeof(uint32_t);
    static_assert(unsigned(Fixture::ScratchPages)>=unsigned(PioneerQwen35::DataflowCoreCount),
        "the private reservation partitions the entire scratch span");
    PioneerQ35ProjectionScratch()=default;
    PioneerQ35ProjectionScratch(const PioneerQ35ProjectionScratch&)=delete;
    PioneerQ35ProjectionScratch& operator=(const PioneerQ35ProjectionScratch&)=delete;
    bool IsEmpty() const { return phase==Empty; }
    bool IsCaptured() const { return phase==Captured; }
    bool IsStaged() const { return phase==Staged; }
    bool IsRunning() const { return phase==Running; }
    bool IsRestored() const { return phase==Restored; }
    bool FailedTransfer() const { return transferFailed; }
    const PioneerQ35ScratchFailure& Failure() const { return failure; }
    // Inspect the retained command and counters twice without changing the
    // lease, bank quarantine, host page or failure. Even a matching sample
    // cannot prove lifetime exclusion or permit restoration/reuse.
    template<class IO,class Scope>
    bool ObserveLateTransfer(IO& io,Scope& scope,const tt_dram_samples_result& dram,
        const tt_compute_noc_result& noc,const tt_host_page_result& host,
        const PioneerResidentLease& lease,PioneerQ35ScratchObservation& observation) const {
        observation={};
        const auto& r=failure.transfer;const auto& trace=failure.trace;
        if (phase!=Failed || !transferFailed || failure.kind!=PioneerQ35ScratchFailureTransfer
            || !failure.leasePending || !failure.leaseRecoveryRequired || !lease.pending || !lease.recovery_required
            || r.state!=TT_STORE_TIMEOUT || !r.recovery_required || !trace.issued || trace.step>=3
            || r.phase!=trace.step+1 || r.requests!=r.completed+1 || !r.polls[trace.step]
            || !scope.ScratchRetainedObservationAllowed(this,io)) return false;
        uint32_t first[4]={};
        for (unsigned pass=0;pass<2;pass++) {
            if (!scope.ScratchRetainedObservationAllowed(this,io)
                || !BlackholeResidentSourceStable(io,noc,lease.config|0x10000)
                || !BlackholeDramSourceStable(io,dram) || !BlackholeHostReadMapStable(io,host)
                || !BlackholeHostReadPciQuiet(io,0x402)) return false;
            for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++)
                if (io.DramTlbRead(s,w)!=BlackholeDramFixedTlbWord(s?0xffa00000:0,1,2,false,w)) return false;
            if (io.ResidentNocRead(0x40) || !BlackholeResidentIdle(io)) return false;
            for (unsigned w=0;w<10;w++) if (io.ResidentNocRead(w*4)!=trace.command[w]) return false;
            const uint32_t expected[4]={1,64,1,1};
            for (unsigned w=0;w<4;w++) {
                // The private read path deliberately never samples the
                // unsupported Blackhole read-data-word counter at 0x20c.
                if (!trace.write && w==1) continue;
                const uint32_t current=io.ResidentNocRead(trace.write?BlackholeHostWriteCounter(w):0x208+w*4);
                if (uint32_t(current-r.before[trace.step][w])!=expected[w]
                    || (pass && current!=first[w])) return false;
                first[w]=current;
            }
            if (!scope.ScratchRetainedObservationAllowed(this,io)) return false;
            io.Pause();
        }
        observation.step=trace.step;observation.write=trace.write;
        for (unsigned w=0;w<4;w++) observation.counters[w]=first[w];
        return true;
    }
    bool OutputVerified() const { return outputVerified; }
    bool MatchesStagedLease(unsigned whichBank,unsigned address,unsigned bytes) const {
        return phase==Staged && whichBank==bank && address==PioneerResidentBanks::Base+firstOffset
            && bytes==Fixture::ScratchBytes;
    }
    bool MatchesBankClaimOwner(const PioneerResidentBanks& banks,const tt_dram_samples_result& dram) const {
        return phase!=Empty && phase!=Failed
            && banks.Q35StreamOwnerMatches(dram,this,PioneerQwen35::DataflowCoreCount);
    }
    // Checks the live bank owner's sticky claim, not merely this object's
    // recorded address. The scope must still prove session/firmware lifetime
    // and compare its independently audited routes before execution.
    bool MatchesBankClaim(const PioneerResidentBanks& banks,const tt_dram_samples_result& dram) const {
        if (!MatchesBankClaimOwner(banks,dram) || bank>=PioneerQwen35::Banks) return false;
        const uint32_t expectedRoute=dram.translated[bank][0]|(dram.translated[bank][1]<<6);
        for (unsigned n=0;n<Fixture::ScratchPages;n++) {
            uint32_t address=0,route=0,index=0;
            const unsigned offset=firstOffset+n*Fixture::PageBytes;
            if (!banks.Q35StreamPage(dram,this,bank,offset,false,address,route,index)
                || address!=PioneerResidentBanks::Base+offset || route!=expectedRoute
                || index!=bank*(PioneerResidentBanks::Capacity/Fixture::PageBytes)+offset/Fixture::PageBytes)
                return false;
        }
        return true;
    }

    template<class IO,class Scope>
    bool Capture(IO& io,Scope& scope,const tt_dram_samples_result& dram,
        const tt_compute_noc_result& noc,const tt_host_page_result& host,
        PioneerResidentLease& lease,PioneerResidentBanks& banks,unsigned whichBank,unsigned offset,
        const uint8_t* initialized,unsigned initializedPages,uint32_t* snapshot,unsigned snapshotWords) {
        if (phase!=Empty || !snapshot || snapshotWords!=SavedWords || whichBank>=PioneerQwen35::Banks
            || (offset&(Fixture::PageBytes-1)) || offset>=PioneerResidentBanks::Capacity
            || Fixture::ScratchBytes>PioneerResidentBanks::Capacity-offset
            || !scope.ScratchReserveAllowed(this,whichBank,offset,Fixture::ScratchBytes)) return false;
        // Exactly 120 disjoint ranges cover the fixed contiguous scratch span.
        // The existing bank owner checks every page is uninitialized and
        // blocks normal bank access to the claim.
        for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
            const unsigned pages=core+1==PioneerQwen35::DataflowCoreCount
                ?Fixture::ScratchPages-(PioneerQwen35::DataflowCoreCount-1):1;
            ranges[core]={whichBank,offset+core*Fixture::PageBytes,pages*Fixture::PageBytes};
        }
        if (!banks.ReserveQ35Stream(dram,this,ranges,PioneerQwen35::DataflowCoreCount,
            initialized,initializedPages)) return false;
        bank=whichBank;firstOffset=offset;saved=snapshot;phase=Capturing;
        for (unsigned n=0;n<Fixture::ScratchPages;n++) {
            if (!Transfer(io,scope,dram,noc,host,lease,banks,n,false,saved+n*(Fixture::PageBytes/4)))
                return false;
            capturedPages=n+1;
        }
        phase=Captured;return true;
    }

    template<class IO,class Scope>
    bool Stage(IO& io,Scope& scope,const tt_dram_samples_result& dram,
        const tt_compute_noc_result& noc,const tt_host_page_result& host,
        PioneerResidentLease& lease,PioneerResidentBanks& banks) {
        if (phase!=Captured || !scope.ScratchStageAllowed(this)) return false;
        phase=Staging;
        for (unsigned n=0;n<Fixture::ScratchPages;n++) {
            if (!FillPage(n)) {
                RecordFailure(PioneerQ35ScratchFailureFill,n,true,lease);
                phase=Failed;scope.RetainScratch(this);return false;
            }
            // Mark before the attempted store; a failed write can be posted.
            dirtyPages=n+1;
            if (!Transfer(io,scope,dram,noc,host,lease,banks,n,true,page)) return false;
        }
        phase=Staged;return true;
    }
    template<class Scope> bool MarkRunning(Scope& scope) {
        if (phase!=Staged || !scope.ScratchRunAllowed(this)) return false;
        phase=Running;return true;
    }
    template<class Scope> bool MarkStopped(Scope& scope) {
        if (phase!=Running || !scope.ScratchStopped(this)) return false;
        phase=Stopped;return true;
    }

    template<class IO,class Scope>
    bool VerifyOutput(IO& io,Scope& scope,const tt_dram_samples_result& dram,
        const tt_compute_noc_result& noc,const tt_host_page_result& host,
        PioneerResidentLease& lease,PioneerResidentBanks& banks) {
        if (phase!=Stopped || outputVerified || !scope.ScratchOutputAllowed(this)) return false;
        bool valid=true;
        for (unsigned n=0;n<Fixture::OutputPages;n++) {
            if (!Transfer(io,scope,dram,noc,host,lease,banks,Fixture::WeightPages+Fixture::InputPages+n,false,page)) return false;
            const unsigned first=n*Fixture::PageBytes/4;
            const unsigned limit=(first+Fixture::PageBytes/4<Fixture::Rows)?first+Fixture::PageBytes/4:Fixture::Rows;
            for (unsigned row=first;row<limit;row++)
                if (!Fixture::OutputMatches(row,page[row-first])) valid=false;
        }
        outputVerified=valid;return valid;
    }

    template<class IO,class Scope>
    bool Restore(IO& io,Scope& scope,const tt_dram_samples_result& dram,
        const tt_compute_noc_result& noc,const tt_host_page_result& host,
        PioneerResidentLease& lease,PioneerResidentBanks& banks) {
        if ((phase!=Staged && phase!=Stopped) || !saved || transferFailed
            || dirtyPages!=Fixture::ScratchPages || !scope.ScratchRestoreAllowed(this)) return false;
        phase=Restoring;
        for (unsigned n=0;n<Fixture::ScratchPages;n++)
            if (!Transfer(io,scope,dram,noc,host,lease,banks,n,true,saved+n*(Fixture::PageBytes/4))) return false;
        // Audit every page again after all writes, including cross-page damage.
        for (unsigned n=0;n<Fixture::ScratchPages;n++) {
            if (!Transfer(io,scope,dram,noc,host,lease,banks,n,false,page)) return false;
            if (memcmp(page,saved+n*(Fixture::PageBytes/4),Fixture::PageBytes)!=0) {
                RecordFailure(PioneerQ35ScratchFailureRestoreMismatch,n,false,lease);
                phase=Failed;scope.RetainScratch(this);return false;
            }
        }
        phase=Restored;return true;
    }
};
#endif
