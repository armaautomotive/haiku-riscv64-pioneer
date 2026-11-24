/* SPDX-License-Identifier: MIT */
// The owner keeps its scratch allocation until bank teardown after a verified
// reset; only never-staged, quiescent diagnostic claims are eligible.
#define PIONEER_QWEN35_PROJECTION_SCRATCH_MOCK_ONLY 1
#include "pioneer_qwen35_projection_scratch_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35ProjectionQualificationOwner.h"
static PioneerDriverRecoveryProof Proof(uint64_t ticket) {
    PioneerDriverRecoveryProof p;
    p.flags=PIONEER_RECOVERY_PROOF_REQUIRED & ~PIONEER_RECOVERY_PROOF_RETAINED_OWNERS_RELEASED;
    p.core_count=120;p.retained_owners=1;p.reset_generation=ticket;p.firmware_generation=ticket;
    return p;
}
int main() {
    (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;
    constexpr uint64_t ticket=19;
    {
        auto owner=std::make_unique<PioneerQ35ProjectionQualificationOwner>();
        auto device=std::make_unique<ScratchDevice<>>();assert(owner->Allocate());
        assert(owner->UnmodifiedResetEligible(device->banks,device->dram,device->lease));
        auto proof=Proof(ticket);
        assert(!owner->RetireUnmodifiedAfterReset(proof,ticket,false,device->banks,device->dram,device->lease));
        assert(!owner->RetireUnmodifiedAfterReset(proof,ticket+1,true,device->banks,device->dram,device->lease));
        proof.flags^=PIONEER_RECOVERY_PROOF_NOC_IDLE;
        assert(!owner->RetireUnmodifiedAfterReset(proof,ticket,true,device->banks,device->dram,device->lease));
        assert(owner->Active() && !owner->Retired());
        proof=Proof(ticket);
        assert(owner->RetireUnmodifiedAfterReset(proof,ticket,true,device->banks,device->dram,device->lease)
            && owner->Retired() && owner->Retained() && !owner->Active());
        assert(!owner->RetireUnmodifiedAfterReset(proof,ticket,true,device->banks,device->dram,device->lease));
    }
    for (unsigned staged=0;staged<2;staged++) {
        auto owner=std::make_unique<PioneerQ35ProjectionQualificationOwner>();
        auto device=std::make_unique<ScratchDevice<>>();assert(owner->Allocate());
        auto* scratch=owner->Scratch();auto* snapshot=owner->Snapshot();assert(scratch && snapshot);
        assert(scratch->Capture(device->io,device->scope,device->dram,device->noc,device->host,device->lease,
            device->banks,0,Fixture::ScratchOffset,device->initialized.data(),device->initialized.size(),
            snapshot,PioneerQ35ProjectionScratch::SavedWords));
        assert(owner->UnmodifiedResetEligible(device->banks,device->dram,device->lease));
        device->lease.pending=true;
        assert(!owner->UnmodifiedResetEligible(device->banks,device->dram,device->lease));
        device->lease.pending=false;
        device->lease.recovery_required=true;
        assert(!owner->UnmodifiedResetEligible(device->banks,device->dram,device->lease));
        device->lease.recovery_required=false;
        if (staged) assert(device->Stage(*scratch));
        owner->Retain();
        const auto proof=Proof(ticket);
        if (staged) {
            assert(!owner->UnmodifiedResetEligible(device->banks,device->dram,device->lease)
                && !owner->RetireUnmodifiedAfterReset(proof,ticket,true,device->banks,device->dram,device->lease)
                && owner->Active());
        } else {
            const uint32_t saved=snapshot[0];
            assert(owner->RetireUnmodifiedAfterReset(proof,ticket,true,device->banks,device->dram,device->lease)
                && owner->Retired() && device->banks.Q35StreamClaimed() && snapshot[0]==saved);
        }
    }
    {
        auto owner=std::make_unique<PioneerQ35ProjectionQualificationOwner>();
        auto device=std::make_unique<ScratchDevice<>>();assert(owner->Allocate());
        device->banks.Quarantine();const auto proof=Proof(ticket);
        assert(!owner->UnmodifiedResetEligible(device->banks,device->dram,device->lease)
            && !owner->RetireUnmodifiedAfterReset(proof,ticket,true,device->banks,device->dram,device->lease));
    }
    std::puts("PASS verified-reset retirement only for unmodified scratch claims; SIMULATED ONLY");
}
