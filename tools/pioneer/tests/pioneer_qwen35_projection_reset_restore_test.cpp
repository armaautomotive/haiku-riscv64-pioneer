/* SPDX-License-Identifier: MIT */
// Saved scratch is restored via a fresh claimed bank over simulated NoC IO.
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS 1
#define PIONEER_QWEN35_PROJECTION_SCRATCH_MOCK_ONLY 1
#include "pioneer_qwen35_projection_scratch_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35ProjectionResetRecovery.h"
struct ResetScope {
    PioneerQ35ProjectionScratch* owner=nullptr;uint64_t expectedTicket=0;
    const PioneerResidentBanks* oldBanks=nullptr;PioneerResidentBanks* newBanks=nullptr;
    const uint32_t* snapshot=nullptr;
    const tt_compute_noc_result* noc=nullptr;const tt_host_page_result* host=nullptr;PioneerResidentLease* lease=nullptr;
    bool allowed=true,retained=false,complete=true,quarantineAtComplete=false;
    unsigned calls=0,failAt=UINT32_MAX,quarantineAt=UINT32_MAX;
    template<class IO> bool RecoveryAdmissionAllowed(PioneerQ35ProjectionScratch& scratch,IO&,
        const PioneerResidentBanks& oldOwner,const tt_dram_samples_result&,PioneerResidentBanks& newOwner,
        const tt_dram_samples_result&,const tt_compute_noc_result& actualNoc,const tt_host_page_result& actualHost,
        PioneerResidentLease& actualLease,uint64_t ticket,const uint32_t* saved,unsigned words,
        const uint8_t* initialized,unsigned pages) {
        return allowed && &scratch==owner && &oldOwner==oldBanks && &newOwner==newBanks && ticket==expectedTicket
            && &actualNoc==noc && &actualHost==host && &actualLease==lease
            && saved==snapshot && words==PioneerQ35ProjectionScratch::SavedWords
            && initialized && pages==PioneerResidentBanks::Pages;
    }
    template<class IO> bool RecoveryTransferAllowed(PioneerQ35ProjectionScratch& scratch,IO&,
        const PioneerResidentBanks& oldOwner,PioneerResidentBanks& newOwner,const tt_compute_noc_result& actualNoc,
        const tt_host_page_result& actualHost,PioneerResidentLease& actualLease,uint64_t ticket,
        const uint32_t* saved,unsigned bank,unsigned offset,bool) {
        ++calls;if (calls==quarantineAt) newOwner.Quarantine();
        return calls!=failAt && allowed && &scratch==owner && &oldOwner==oldBanks && &newOwner==newBanks
            && &actualNoc==noc && &actualHost==host && &actualLease==lease && ticket==expectedTicket && saved==snapshot && bank==0
            && offset>=Fixture::ScratchOffset && offset<Fixture::ScratchOffset+Fixture::ScratchBytes;
    }
    template<class IO> bool RecoveryCompleteAllowed(PioneerQ35ProjectionScratch& scratch,IO&,
        const PioneerResidentBanks& oldOwner,PioneerResidentBanks& newOwner,const tt_compute_noc_result& actualNoc,
        const tt_host_page_result& actualHost,PioneerResidentLease& actualLease,uint64_t ticket,const uint32_t* saved) {
        if (quarantineAtComplete) newOwner.Quarantine();
        return complete && allowed && &scratch==owner && &oldOwner==oldBanks && &newOwner==newBanks
            && &actualNoc==noc && &actualHost==host && &actualLease==lease && ticket==expectedTicket && saved==snapshot;
    }
    void RetainRecovery(PioneerQ35ProjectionScratch& scratch) { assert(&scratch==owner);retained=true; }
};
static PioneerDriverRecoveryProof ResetProof(uint64_t ticket) {
    PioneerDriverRecoveryProof p;p.flags=PIONEER_RECOVERY_PROOF_REQUIRED & ~PIONEER_RECOVERY_PROOF_RETAINED_OWNERS_RELEASED;
    p.core_count=120;p.retained_owners=1;p.reset_generation=ticket;p.firmware_generation=ticket;return p;
}
template<class Device>
static bool Restore(PioneerQ35ProjectionResetRecovery& recovery,PioneerQ35ProjectionScratch& scratch,
    uint64_t ticket,const PioneerDriverRecoveryProof& proof,ScratchDevice<>& oldDevice,Device& fresh,ResetScope& scope) {
    return recovery.Restore(scratch,ticket,proof,fresh.io,scope,oldDevice.banks,oldDevice.dram,
        fresh.banks,fresh.dram,fresh.noc,fresh.host,fresh.lease,fresh.initialized.data(),fresh.initialized.size());
}
int main() {
    (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;
    constexpr uint64_t ticket=37;
    for (unsigned scenario=0;scenario<7;scenario++) {
        auto scratch=std::make_unique<PioneerQ35ProjectionScratch>();
        auto oldDevice=std::make_unique<ScratchDevice<>>();
        auto fresh=std::make_unique<ScratchDevice<>>();
        auto recovery=std::make_unique<PioneerQ35ProjectionResetRecovery>();
        assert(oldDevice->Capture(*scratch) && oldDevice->Stage(*scratch));
        oldDevice->banks.Quarantine(); // identity survives; old bank access does not
        PioneerQ35StreamBankRange ranges[PioneerQwen35::DataflowCoreCount]{};
        for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
            const unsigned pages=core+1==PioneerQwen35::DataflowCoreCount
                ?Fixture::ScratchPages-(PioneerQwen35::DataflowCoreCount-1):1;
            ranges[core]={0,Fixture::ScratchOffset+core*Fixture::PageBytes,pages*Fixture::PageBytes};
        }
        assert(oldDevice->banks.Q35StreamClaimMatches(scratch.get(),ranges,PioneerQwen35::DataflowCoreCount));
        ranges[119].offset+=Fixture::PageBytes;
        assert(!oldDevice->banks.Q35StreamClaimMatches(scratch.get(),ranges,PioneerQwen35::DataflowCoreCount));
        ResetScope scope;scope.owner=scratch.get();scope.expectedTicket=ticket;
        scope.oldBanks=&oldDevice->banks;scope.newBanks=&fresh->banks;scope.snapshot=oldDevice->snapshot.data();
        scope.noc=&fresh->noc;scope.host=&fresh->host;scope.lease=&fresh->lease;
        auto proof=ResetProof(ticket);
        auto bad=proof;bad.flags^=PIONEER_RECOVERY_PROOF_NOC_IDLE;
        assert(!Restore(*recovery,*scratch,ticket,bad,*oldDevice,*fresh,scope)
            && !recovery->Faulted() && !fresh->banks.Q35StreamClaimed() && !scope.retained);
        scope.snapshot=nullptr;
        assert(!Restore(*recovery,*scratch,ticket,proof,*oldDevice,*fresh,scope)
            && !recovery->Faulted() && !fresh->banks.Q35StreamClaimed());
        scope.snapshot=oldDevice->snapshot.data();
        scope.lease=nullptr;
        assert(!Restore(*recovery,*scratch,ticket,proof,*oldDevice,*fresh,scope)
            && !recovery->Faulted() && !fresh->banks.Q35StreamClaimed());
        scope.lease=&fresh->lease;
        fresh->lease.pending=true;
        assert(!Restore(*recovery,*scratch,ticket,proof,*oldDevice,*fresh,scope)
            && !recovery->Faulted() && !fresh->banks.Q35StreamClaimed());
        fresh->lease.pending=false;
        if (scenario==1) scope.failAt=4;
        if (scenario==2) fresh->io.failRequest=fresh->io.doorbells+1;
        if (scenario==3) scope.complete=false;
        if (scenario==4) fresh->initialized[Fixture::ScratchOffset/4096]=1;
        if (scenario==5) scope.quarantineAt=4;
        if (scenario==6) scope.quarantineAtComplete=true;
        const bool okay=Restore(*recovery,*scratch,ticket,proof,*oldDevice,*fresh,scope);
        if (!scenario) {
            assert(okay && !scope.retained && fresh->banks.Q35StreamClaimed()
                && recovery->VerifiedFor(*scratch,ticket,oldDevice->banks,fresh->banks)
                && !recovery->VerifiedFor(*scratch,ticket+1,oldDevice->banks,fresh->banks));
            for (unsigned n=0;n<Fixture::ScratchPages;n++) {
                const auto& page=fresh->io.memory.at(fresh->io.Key(18|(12<<6),Fixture::ScratchAddress+n*4096));
                for (unsigned i=0;i<1024;i++) assert(page[i]==oldDevice->original[n*1024+i]);
            }
            fresh->banks.Quarantine();
            assert(!recovery->VerifiedFor(*scratch,ticket,oldDevice->banks,fresh->banks));
        } else {
            assert(!okay && recovery->Faulted() && scope.retained
                && fresh->banks.Q35StreamClaimed()==(scenario!=4)
                && oldDevice->banks.Quarantined() && fresh->banks.Quarantined()
                && !recovery->VerifiedFor(*scratch,ticket,oldDevice->banks,fresh->banks));
            if (scenario==1) assert(recovery->FailedPage()==3 && recovery->FailedWrite());
            if (scenario==2) assert(recovery->FailedPage()==0 && recovery->FailedWrite()
                && fresh->lease.recovery_required);
            const unsigned calls=scope.calls;
            assert(!Restore(*recovery,*scratch,ticket,proof,*oldDevice,*fresh,scope) && scope.calls==calls);
        }
    }
    {
        auto scratch=std::make_unique<PioneerQ35ProjectionScratch>();
        auto oldDevice=std::make_unique<ScratchDevice<>>();auto fresh=std::make_unique<ScratchDevice<>>();
        auto recovery=std::make_unique<PioneerQ35ProjectionResetRecovery>();
        assert(oldDevice->Capture(*scratch)); // no page was staged
        ResetScope scope;scope.owner=scratch.get();scope.expectedTicket=ticket;
        scope.oldBanks=&oldDevice->banks;scope.newBanks=&fresh->banks;scope.snapshot=oldDevice->snapshot.data();
        scope.noc=&fresh->noc;scope.host=&fresh->host;scope.lease=&fresh->lease;
        assert(!Restore(*recovery,*scratch,ticket,ResetProof(ticket),*oldDevice,*fresh,scope)
            && !recovery->Faulted() && !fresh->banks.Q35StreamClaimed());
    }
    {
        auto scratch=std::make_unique<PioneerQ35ProjectionScratch>();
        auto oldDevice=std::make_unique<ScratchDevice<>>();auto fresh=std::make_unique<ScratchDevice<>>();
        auto recovery=std::make_unique<PioneerQ35ProjectionResetRecovery>();
        oldDevice->io.failRequest=oldDevice->io.doorbells+1;
        assert(!oldDevice->Capture(*scratch) && scratch->FailedTransfer());
        oldDevice->banks.Quarantine();
        ResetScope scope;scope.owner=scratch.get();scope.expectedTicket=ticket;
        scope.oldBanks=&oldDevice->banks;scope.newBanks=&fresh->banks;scope.snapshot=oldDevice->snapshot.data();
        scope.noc=&fresh->noc;scope.host=&fresh->host;scope.lease=&fresh->lease;
        assert(!Restore(*recovery,*scratch,ticket,ResetProof(ticket),*oldDevice,*fresh,scope)
            && !recovery->Faulted() && !fresh->banks.Q35StreamClaimed());
    }
    {
        auto scratch=std::make_unique<PioneerQ35ProjectionScratch>();
        auto oldDevice=std::make_unique<ScratchDevice<>>();auto fresh=std::make_unique<ScratchDevice<>>();
        auto recovery=std::make_unique<PioneerQ35ProjectionResetRecovery>();
        assert(oldDevice->Capture(*scratch));
        oldDevice->io.failRequest=oldDevice->io.doorbells+1;
        assert(!oldDevice->Stage(*scratch) && scratch->FailedTransfer() && oldDevice->lease.recovery_required);
        oldDevice->banks.Quarantine();
        ResetScope scope;scope.owner=scratch.get();scope.expectedTicket=ticket;
        scope.oldBanks=&oldDevice->banks;scope.newBanks=&fresh->banks;scope.snapshot=oldDevice->snapshot.data();
        scope.noc=&fresh->noc;scope.host=&fresh->host;scope.lease=&fresh->lease;
        assert(Restore(*recovery,*scratch,ticket,ResetProof(ticket),*oldDevice,*fresh,scope)
            && recovery->VerifiedFor(*scratch,ticket,oldDevice->banks,fresh->banks)
            && scratch->FailedTransfer() && !scratch->IsRestored() && oldDevice->banks.Quarantined());
    }
    {
        auto scratch=std::make_unique<PioneerQ35ProjectionScratch>();
        auto oldDevice=std::make_unique<ScratchDevice<>>();
        auto fresh=std::make_unique<ScratchDevice<CrossCorruptBanksMock>>();
        auto recovery=std::make_unique<PioneerQ35ProjectionResetRecovery>();
        assert(oldDevice->Capture(*scratch) && oldDevice->Stage(*scratch));fresh->io.corruptDuringRestore=true;
        ResetScope scope;scope.owner=scratch.get();scope.expectedTicket=ticket;
        scope.oldBanks=&oldDevice->banks;scope.newBanks=&fresh->banks;scope.snapshot=oldDevice->snapshot.data();
        scope.noc=&fresh->noc;scope.host=&fresh->host;scope.lease=&fresh->lease;
        assert(!Restore(*recovery,*scratch,ticket,ResetProof(ticket),*oldDevice,*fresh,scope)
            && recovery->Faulted() && scope.retained && oldDevice->banks.Quarantined()
            && fresh->banks.Quarantined() && !recovery->FailedWrite() && recovery->FailedPage()==0);
    }
    std::puts("PASS post-reset 680-page scratch restore, full audit and fault retention; SIMULATED ONLY");
}
