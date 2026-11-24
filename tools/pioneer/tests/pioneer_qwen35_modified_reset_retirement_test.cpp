/* SPDX-License-Identifier: MIT */
// Exercise the actual modified-scratch owner retirement against simulated IO.
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS 1
#define PIONEER_QWEN35_PROJECTION_SCRATCH_MOCK_ONLY 1
#include "pioneer_qwen35_projection_scratch_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35ProjectionQualificationOwner.h"
#define PIONEER_QWEN35_RECOVERY_RUN_ENABLE 1
#define PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS 1
#define PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY 1

struct ResetContext {
    PioneerQ35ProjectionQualificationOwner& qwen35QualificationOwner;
    PioneerResidentBanks& banks;PioneerResidentLease& lease;
    struct A { tt_dram_samples_result& dram; };struct B { A baseline; };
    struct C { B baseline; };struct D { C baseline; } baseline;
    bool qwen35QualificationInternalActive=false;
    void *qwen35DataflowService=nullptr,*qwen35DataflowMemory=nullptr,*qwen35DataflowAuthority=nullptr;
    void *qwen35DataflowProgramScratch=nullptr,*qwen35GraphOwner=nullptr,*qwen35SharedRoleBinding=nullptr;
    void* recoveryFreshSession=nullptr;
    void* recoveryFabricArcOwner=nullptr;
    struct { void* qwen35BorrowedRoleScope=nullptr; } io;
    ResetContext(PioneerQ35ProjectionQualificationOwner& owner,PioneerResidentBanks& b,
        PioneerResidentLease& l,tt_dram_samples_result& d):qwen35QualificationOwner(owner),banks(b),lease(l),baseline{{{{d}}}} {}
};
static constexpr int B_NOT_ALLOWED=-7;
static int ResetAllowed(ResetContext* context) {
#include "pioneer_q35_projection_owner_reset_guard.inc"
    return 0;
}

struct Receipt {
    const PioneerQ35ProjectionScratch* owner=nullptr;
    const PioneerResidentBanks *oldBanks=nullptr,*freshBanks=nullptr;
    mutable bool valid=true;
    bool VerifiedFor(const PioneerQ35ProjectionScratch& scratch,uint64_t ticket,
        const PioneerResidentBanks& old,const PioneerResidentBanks& fresh) const {
        return valid && &scratch==owner && &old==oldBanks && &fresh==freshBanks && ticket==37
            && old.Q35StreamClaimIdentity(owner,PioneerQwen35::DataflowCoreCount);
    }
};
struct Session {};
struct Scope {
    bool allowed=true,invalidate=false;
    bool QualificationModifiedResetRetirementAllowed(const PioneerQ35ProjectionQualificationOwner&,
        const PioneerQ35ProjectionScratch&,const Receipt& receipt,const Session&) const {
        if (invalidate) receipt.valid=false;
        return allowed;
    }
};
static PioneerDriverRecoveryProof Proof() {
    PioneerDriverRecoveryProof p;
    p.flags=PIONEER_RECOVERY_PROOF_REQUIRED & ~PIONEER_RECOVERY_PROOF_RETAINED_OWNERS_RELEASED;
    p.core_count=120;p.retained_owners=1;p.reset_generation=37;p.firmware_generation=37;return p;
}
int main() {
    (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;
    auto old=std::make_unique<ScratchDevice<>>();auto fresh=std::make_unique<ScratchDevice<>>();
    auto owner=std::make_unique<PioneerQ35ProjectionQualificationOwner>();assert(owner->Allocate());
    auto* scratch=owner->Scratch();auto* snapshot=owner->Snapshot();assert(scratch && snapshot);
    Receipt receipt{scratch,&old->banks,&fresh->banks};Session session;Scope scope;auto proof=Proof();
    assert(scratch->Capture(old->io,old->scope,old->dram,old->noc,old->host,old->lease,old->banks,
        0,Fixture::ScratchOffset,old->initialized.data(),old->initialized.size(),snapshot,
        PioneerQ35ProjectionScratch::SavedWords));
    assert(!owner->ModifiedResetEligible(old->banks));
    assert(!owner->RetireModifiedAfterReset(proof,37,receipt,old->banks,fresh->banks,session,scope));
    assert(scratch->Stage(old->io,old->scope,old->dram,old->noc,old->host,old->lease,old->banks));
    old->banks.Quarantine();assert(!owner->ModifiedResetEligible(old->banks));owner->Retain();
    PioneerQ35StreamBankRange wrongRanges[PioneerQwen35::DataflowCoreCount]{};
    for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++)
        wrongRanges[core]={0,Fixture::ScratchOffset+(1000+core)*Fixture::PageBytes,Fixture::PageBytes};
    assert(fresh->banks.ReserveQ35Stream(fresh->dram,scratch,wrongRanges,PioneerQwen35::DataflowCoreCount,
        fresh->initialized.data(),fresh->initialized.size()));
    assert(fresh->banks.Q35StreamClaimIdentity(scratch,PioneerQwen35::DataflowCoreCount)
        && owner->ModifiedResetEligible(old->banks) && !owner->ModifiedResetEligible(fresh->banks));
    ResetContext context(*owner,old->banks,old->lease,old->dram);
    assert(ResetAllowed(&context)==B_NOT_ALLOWED); // Generic controller remains closed.
    context.qwen35DataflowService=&context;context.qwen35DataflowMemory=&context;
    context.qwen35SharedRoleBinding=&context;
    assert(ResetAllowed(&context)==B_NOT_ALLOWED);
    context.recoveryFreshSession=&context;assert(ResetAllowed(&context)==B_NOT_ALLOWED);
    context.recoveryFreshSession=nullptr;
    context.recoveryFabricArcOwner=&context;assert(ResetAllowed(&context)==B_NOT_ALLOWED);
    context.recoveryFabricArcOwner=nullptr;
    auto bad=proof;bad.flags^=PIONEER_RECOVERY_PROOF_NOC_IDLE;
    assert(!owner->RetireModifiedAfterReset(bad,37,receipt,old->banks,fresh->banks,session,scope));
    assert(!owner->RetireModifiedAfterReset(proof,38,receipt,old->banks,fresh->banks,session,scope));
    receipt.valid=false;
    assert(!owner->RetireModifiedAfterReset(proof,37,receipt,old->banks,fresh->banks,session,scope));
    receipt.valid=true;scope.allowed=false;
    assert(!owner->RetireModifiedAfterReset(proof,37,receipt,old->banks,fresh->banks,session,scope));
    scope.allowed=true;scope.invalidate=true;
    assert(!owner->RetireModifiedAfterReset(proof,37,receipt,old->banks,fresh->banks,session,scope));
    assert(owner->Active() && owner->Retained() && !owner->Retired());
    scope.invalidate=false;receipt.valid=true;
    assert(owner->RetireModifiedAfterReset(proof,37,receipt,old->banks,fresh->banks,session,scope));
    assert(owner->Retired() && !owner->Active() && owner->RetiredModifiedIdentity(scratch,snapshot)
        && !owner->RetiredModifiedIdentity(scratch,fresh->snapshot.data())
        && old->banks.Q35StreamClaimIdentity(scratch,PioneerQwen35::DataflowCoreCount)
        && snapshot[0]==old->original[0]);
    assert(!owner->RetireModifiedAfterReset(proof,37,receipt,old->banks,fresh->banks,session,scope));
    // The sticky bank claim must be destroyed before its old owner/snapshot.
    old.reset();fresh.reset();owner.reset();
    std::puts("PASS modified reset retirement keeps exact scratch owner and snapshot; SIMULATED ONLY");
}
