/* SPDX-License-Identifier: MIT */
// The transaction uses the production bank reservation and page-transfer
// functions over a simulated NoC. No physical device or inference is used.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35ProjectionScratch.h"
#include <memory>
#include <vector>

using Fixture=PioneerQwen35::DataflowProjectionQualification;

struct ScratchScope {
    const void* owner=nullptr;
    bool allowed=true,stopped=false,retained=false;
    bool ScratchReserveAllowed(const void* p,unsigned bank,unsigned offset,unsigned bytes) {
        if (!allowed || owner || bank!=0 || offset!=0 || bytes!=Fixture::ScratchBytes) return false;
        owner=p;return true;
    }
    bool ScratchTransferAllowed(const void* p,unsigned bank,unsigned offset,bool) const {
        return allowed && p==owner && bank==0 && offset<Fixture::ScratchBytes && !(offset&4095);
    }
    bool ScratchStageAllowed(const void* p) const { return allowed && p==owner; }
    bool ScratchRunAllowed(const void* p) const { return allowed && p==owner; }
    bool ScratchStopped(const void* p) const { return allowed && stopped && p==owner; }
    bool ScratchOutputAllowed(const void* p) const { return allowed && stopped && p==owner; }
    bool ScratchRestoreAllowed(const void* p) const { return allowed && p==owner; }
    void RetainScratch(const void* p) { assert(p==owner);retained=true; }
};

struct CrossCorruptBanksMock : BanksMock {
    bool corruptDuringRestore=false;
    void ResidentNocWrite(uint32_t offset,uint32_t value) {
        BanksMock::ResidentNocWrite(offset,value);
        if (corruptDuringRestore && offset==0x40 && value==1 && command[7]==0x212
            && command[3]==PioneerResidentBanks::Base+(Fixture::ScratchPages-1)*4096) {
            memory[Key(18|(12<<6),PioneerResidentBanks::Base)][0]^=1;
            corruptDuringRestore=false;
        }
    }
};

template<class Raw=BanksMock> struct ScratchDevice {
    Raw io;tt_dram_samples_result dram=BankProof(io);tt_compute_noc_result noc=io.Compute();
    tt_host_page_result host{};PioneerResidentLease lease;PioneerResidentBanks banks;
    std::vector<uint8_t> initialized=std::vector<uint8_t>(PioneerResidentBanks::Pages,0);
    std::vector<uint32_t> original=std::vector<uint32_t>(PioneerQ35ProjectionScratch::SavedWords);
    std::vector<uint32_t> snapshot=std::vector<uint32_t>(PioneerQ35ProjectionScratch::SavedWords);
    ScratchScope scope;
    ScratchDevice() {
        assert(banks.Bind(dram) && Prepared(io,host,lease));
        for (unsigned n=0;n<Fixture::ScratchPages;n++) {
            auto& data=io.memory[io.Key(18|(12<<6),PioneerResidentBanks::Base+n*4096)];
            for (unsigned i=0;i<1024;i++) data[i]=original[n*1024+i]=0xa5500000U^(n*104729U+i*17U);
        }
    }
    bool Capture(PioneerQ35ProjectionScratch& owner) {
        return owner.Capture(io,scope,dram,noc,host,lease,banks,0,0,initialized.data(),
            initialized.size(),snapshot.data(),snapshot.size());
    }
    bool Stage(PioneerQ35ProjectionScratch& owner) { return owner.Stage(io,scope,dram,noc,host,lease,banks); }
    bool Verify(PioneerQ35ProjectionScratch& owner) { return owner.VerifyOutput(io,scope,dram,noc,host,lease,banks); }
    bool Restore(PioneerQ35ProjectionScratch& owner) { return owner.Restore(io,scope,dram,noc,host,lease,banks); }
    void SetOutput() {
        const unsigned first=Fixture::WeightPages+Fixture::InputPages;
        for (unsigned row=0;row<Fixture::Rows;row++) {
            auto& data=io.memory[io.Key(18|(12<<6),PioneerResidentBanks::Base+(first+row/1024)*4096)];
            data[row%1024]=Fixture::ExpectedBits(row);
        }
    }
};

#ifndef PIONEER_QWEN35_PROJECTION_SCRATCH_MOCK_ONLY
int main()
{
    (void)&Input; (void)&Prepare; (void)&Upload; (void)&Close;
    {
        auto owner=std::make_unique<PioneerQ35ProjectionScratch>();
        auto device=std::make_unique<ScratchDevice<>>();
        assert(!owner->Stage(device->io,device->scope,device->dram,device->noc,
            device->host,device->lease,device->banks));
        assert(device->Capture(*owner) && owner->IsCaptured() && device->snapshot==device->original);
        assert(!device->banks.Range(device->dram,0,0)); // claimed pages are private
        assert(device->Stage(*owner) && owner->IsStaged());
        assert(owner->MatchesStagedLease(0,PioneerResidentBanks::Base,Fixture::ScratchBytes)
            && !owner->MatchesStagedLease(1,PioneerResidentBanks::Base,Fixture::ScratchBytes)
            && !owner->MatchesStagedLease(0,PioneerResidentBanks::Base+4096,Fixture::ScratchBytes)
            && !owner->MatchesStagedLease(0,PioneerResidentBanks::Base,Fixture::ScratchBytes-1));
        PioneerResidentBanks otherBanks;
        assert(owner->MatchesBankClaim(device->banks,device->dram)
            && owner->MatchesBankClaimOwner(device->banks,device->dram)
            && !owner->MatchesBankClaim(otherBanks,device->dram));
        assert(owner->MarkRunning(device->scope));
        assert(!owner->MatchesStagedLease(0,PioneerResidentBanks::Base,Fixture::ScratchBytes));
        assert(!device->Verify(*owner) && !device->Restore(*owner) && !owner->MarkStopped(device->scope));
        device->SetOutput();device->scope.stopped=true;
        assert(owner->MarkStopped(device->scope));
        auto& firstOutput=device->io.memory[device->io.Key(18|(12<<6),PioneerResidentBanks::Base
            +(Fixture::WeightPages+Fixture::InputPages)*4096)];
        firstOutput[0]^=1;
        assert(!device->Verify(*owner) && !owner->OutputVerified());
        device->SetOutput();
        assert(device->Verify(*owner) && owner->OutputVerified());
        assert(device->Restore(*owner) && owner->IsRestored() && !device->scope.retained);
        for (unsigned n=0;n<Fixture::ScratchPages;n++) {
            const auto& data=device->io.memory.at(device->io.Key(18|(12<<6),PioneerResidentBanks::Base+n*4096));
            for (unsigned i=0;i<1024;i++) assert(data[i]==device->original[n*1024+i]);
        }
        // Claim remains sticky even after verified restore; the session owner
        // must keep the scratch object alive until bank-context destruction.
        assert(device->banks.Q35StreamClaimed() && !device->banks.Range(device->dram,0,0));
    }
    {
        auto owner=std::make_unique<PioneerQ35ProjectionScratch>();
        auto device=std::make_unique<ScratchDevice<>>();
        device->initialized[20]=1;
        assert(!device->Capture(*owner) && !device->banks.Q35StreamClaimed());
    }
    {
        auto owner=std::make_unique<PioneerQ35ProjectionScratch>();
        auto device=std::make_unique<ScratchDevice<>>();
        assert(device->Capture(*owner));
        device->io.failRequest=device->io.doorbells+1;
        assert(!device->Stage(*owner) && owner->FailedTransfer() && device->scope.retained);
        assert(!device->Restore(*owner) && device->banks.Q35StreamClaimed());
    }
    {
        auto owner=std::make_unique<PioneerQ35ProjectionScratch>();
        auto device=std::make_unique<ScratchDevice<CrossCorruptBanksMock>>();
        assert(device->Capture(*owner) && device->Stage(*owner));
        device->io.corruptDuringRestore=true;
        assert(!device->Restore(*owner) && !owner->IsRestored() && device->scope.retained);
        assert(device->banks.Q35StreamClaimed());
    }
    std::puts("PASS private 680-page Q4_K scratch capture, stage, output and verified restoration; SIMULATED DEVICE");
    return 0;
}
#endif
