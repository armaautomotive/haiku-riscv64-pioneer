/* SPDX-License-Identifier: MIT */
// Exercise the exact reset-finish guard and ticket ordering without hardware.
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35ProjectionQualificationOwner.h"
#include <cassert>
#include <cstdio>
using status_t=int;
static constexpr status_t B_OK=0,B_DEV_NOT_READY=-5,B_BAD_VALUE=-6;
struct Context {
    PioneerQ35ProjectionQualificationOwner qwen35QualificationOwner;
    PioneerResidentBanks banks;PioneerResidentLease lease;tt_dram_samples_result dram{};
    struct A { tt_dram_samples_result& dram; };struct B { A baseline; };
    struct C { B baseline; };struct D { C baseline; } baseline{{{{dram}}}};
    struct IO { bool postResetDisposeComplete=true; } io;
    void* recoveryMapping=nullptr;
};
struct Guard {
    unsigned calls=0,failAt=0;
    status_t validate(uint64_t) { return ++calls==failAt?B_BAD_VALUE:B_OK; }
};
static status_t Finish(Context* context,Guard* guard,const PioneerDriverRecoveryProof& proof,uint64_t ticket) {
    auto retain=[](status_t status) { return status; };
#include "pioneer_q35_projection_reset_finish.inc"
    return B_OK;
}
static PioneerDriverRecoveryProof Proof(uint64_t ticket) {
    PioneerDriverRecoveryProof p;p.flags=PIONEER_RECOVERY_PROOF_REQUIRED & ~PIONEER_RECOVERY_PROOF_RETAINED_OWNERS_RELEASED;
    p.core_count=120;p.retained_owners=1;p.reset_generation=ticket;p.firmware_generation=ticket;return p;
}
int main() {
    constexpr uint64_t ticket=23;
    for (unsigned failure=0;failure<5;failure++) {
        Context c;Guard guard;assert(c.qwen35QualificationOwner.Allocate());c.lease.ready=true;
        auto* saved=c.qwen35QualificationOwner.Snapshot();assert(saved);saved[0]=0x12345678;
        auto proof=Proof(ticket);
        if (failure==1) guard.failAt=1;
        if (failure==2) proof.flags^=PIONEER_RECOVERY_PROOF_NOC_IDLE;
        if (failure==3) c.io.postResetDisposeComplete=false;
        if (failure==4) guard.failAt=2;
        const status_t result=Finish(&c,&guard,proof,ticket);
        if (!failure) assert(result==B_OK && guard.calls==2 && c.qwen35QualificationOwner.Retired());
        else if (failure==4) assert(result==B_BAD_VALUE && guard.calls==2 && c.qwen35QualificationOwner.Retired());
        else assert(result!=B_OK && guard.calls==1 && c.qwen35QualificationOwner.Active());
        assert(saved[0]==0x12345678); // still owned until context/bank destruction
    }
    std::puts("PASS reset finish validates ticket before and after unmodified retirement; SIMULATED ONLY");
}
