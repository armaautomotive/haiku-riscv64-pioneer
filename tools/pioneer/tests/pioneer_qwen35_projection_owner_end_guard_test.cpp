/* SPDX-License-Identifier: MIT */
// Execute the exact extracted session-end guard with the real owner state.
// The context and retention callback are simulated; no hardware is accessed.
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35ProjectionQualificationOwner.h"
#include <cassert>
#include <cstdio>

struct Context {
    PioneerQ35ProjectionQualificationOwner qwen35QualificationOwner;
    PioneerResidentBanks banks;
    PioneerResidentLease lease;
    tt_dram_samples_result dram{};
    struct A { tt_dram_samples_result& dram; };struct B { A baseline; };
    struct C { B baseline; };struct D { C baseline; } baseline{{{{dram}}}};
    void* qwen35DataflowService=nullptr;void* qwen35DataflowMemory=nullptr;
    void* qwen35DataflowAuthority=nullptr;void* qwen35DataflowProgramScratch=nullptr;void* qwen35GraphOwner=nullptr;
    bool qwen35QualificationInternalActive=false;
    unsigned retained=0;
};
struct Result { unsigned retained=0; };
static void RetainQ38DispatchOwner(Context& c,Result& result) { c.retained++;result.retained++; }
static bool End(Context& c,Result& result,bool& clean) {
#include "pioneer_q35_projection_owner_end_guard.inc"
    return true;
}
static bool Unsafe(Context& c) {
    auto& qwen35QualificationOwner=c.qwen35QualificationOwner;
    const bool qwen35QualificationInternalActive=c.qwen35QualificationInternalActive;
#include "pioneer_q35_projection_owner_unsafe_guard.inc"
    return false;
}
static constexpr int B_NOT_ALLOWED=-7;
static int Reset(Context* context) {
#include "pioneer_q35_projection_owner_reset_guard.inc"
    return 0;
}
struct Scope {
    bool QualificationOwnerAbandonAllowed(const PioneerQ35ProjectionQualificationOwner&) const { return true; }
};
int main() {
    {
        Context c;Result result;bool clean=true;
        assert(!Unsafe(c) && Reset(&c)==0 && End(c,result,clean) && clean && !result.retained);
    }
    {
        Context c;Result result;bool clean=true;
        assert(c.qwen35QualificationOwner.Allocate());
        assert(Unsafe(c) && Reset(&c)==B_NOT_ALLOWED);
        c.lease.ready=true;assert(Reset(&c)==0);
        c.lease.pending=true;assert(Reset(&c)==B_NOT_ALLOWED);c.lease.pending=false;
        c.qwen35DataflowService=&c;assert(Reset(&c)==B_NOT_ALLOWED);c.qwen35DataflowService=nullptr;
        c.qwen35GraphOwner=&c;assert(Reset(&c)==B_NOT_ALLOWED);c.qwen35GraphOwner=nullptr;
        c.qwen35QualificationInternalActive=true;
        assert(!Unsafe(c) && Reset(&c)==B_NOT_ALLOWED);
        c.qwen35QualificationInternalActive=false;
        assert(!End(c,result,clean) && !clean && c.qwen35QualificationOwner.Active()
            && c.qwen35QualificationOwner.Retained() && c.retained==1 && result.retained==1);
    }
    {
        Context c;Result result;bool clean=true;Scope scope;
        assert(c.qwen35QualificationOwner.Allocate()
            && c.qwen35QualificationOwner.AbandonUnclaimed(scope,c.banks));
        assert(!Unsafe(c) && Reset(&c)==0 && End(c,result,clean) && clean && !c.retained && !result.retained);
    }
    std::puts("PASS actual diagnostic owner session-end guard, active retention and clean abandonment; SIMULATED ONLY");
    return 0;
}
