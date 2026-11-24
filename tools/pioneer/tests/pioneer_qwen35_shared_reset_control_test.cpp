/* SPDX-License-Identifier: MIT */
// Compile the real scope method against controlled owner and hardware proofs.
#define PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS 1
#define PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS 1
#include <cassert>
#include <cstdint>
#include <cstdio>
namespace PioneerQwen35 { enum { DataflowRolePhysicalExtraCode=2 }; }
struct PioneerSessionIO {};
struct Binding { bool match=true;unsigned calls=0; };
struct Context { Binding* qwen35SharedRoleBinding=nullptr; };
static bool PioneerQ35SharedRoleResetTransition(Binding* binding,unsigned core,uint32_t expected,uint32_t desired) {
    if (!binding) return false;
    ++binding->calls;
    return binding->match && core==7 && expected==0x400 && desired==0x7c1;
}
struct Scope {
    Context* fContext;
    mutable unsigned reads=0;
    unsigned failOn=0;
    bool ProveRoleMemoryReadSafe(PioneerSessionIO&,unsigned core,unsigned which) const {
        ++reads;
        return reads!=failOn && core==7 && which==PioneerQwen35::DataflowRolePhysicalExtraCode;
    }
#include "pioneer_q35_scope_reset_control.inc"
};
int main() {
    Binding binding;Context context{&binding};PioneerSessionIO io;
    Scope good{&context};
    assert(good.ProveRoleControlWriteSafe(io,7,0x400,0x7c1) && good.reads==2 && binding.calls==1);
    Scope before{&context};before.failOn=1;binding.calls=0;
    assert(!before.ProveRoleControlWriteSafe(io,7,0x400,0x7c1) && before.reads==1 && !binding.calls);
    Scope after{&context};after.failOn=2;binding.calls=0;
    assert(!after.ProveRoleControlWriteSafe(io,7,0x400,0x7c1) && after.reads==2 && binding.calls==1);
    Scope wrong{&context};binding.calls=0;
    assert(!wrong.ProveRoleControlWriteSafe(io,7,0x400,0x7c0) && wrong.reads==1 && binding.calls==1);
    Scope absent{&context};context.qwen35SharedRoleBinding=nullptr;
    assert(!absent.ProveRoleControlWriteSafe(io,7,0x400,0x7c1) && absent.reads==1);
    std::puts("PASS actual shared reset-control scope rechecks hardware and owner transition; SIMULATED ONLY");
}
