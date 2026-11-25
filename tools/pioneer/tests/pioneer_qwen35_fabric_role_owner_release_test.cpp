/* SPDX-License-Identifier: MIT */
// Exercise the real release controller with simulated retained owners.
#include <cassert>
#include <cstdint>
#include <cstdio>
static unsigned destroyed=0;
struct Role {
    bool ready=true,attempted=false;
    ~Role() { ++destroyed; }
    template<class Check> bool BeginFabricDestroy(uint64_t ticket,Check& check) {
        if (!check() || !ready || attempted || ticket!=7 || !check()) return false;
        attempted=true;return true;
    }
};
struct Context { Role* qwen35SharedRoleBinding=nullptr;int* qwen35GraphOwner=nullptr;void* recoveryFreshSession=nullptr; };
struct Check {
    unsigned calls=0,failAt=0;bool live=true;
    bool operator()() { if (++calls==failAt) live=false;return live; }
};
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35FabricRoleOwnerRelease.h"
struct GraphSwap {
    Context& context;int* replacement;
    bool operator()() { context.qwen35GraphOwner=replacement;return true; }
};
int main() {
    unsigned checks=0;
    for (unsigned fail=0;;fail++) {
        if (fail && fail>checks) break;
        destroyed=0;int graph=1;Context ctx{new Role,&graph,nullptr};Check check;check.failAt=fail;
        const bool released=PioneerQ35ReleaseFabricRoleOwner(ctx,7,check);
        if (!fail) {
            assert(released && !ctx.qwen35SharedRoleBinding && ctx.qwen35GraphOwner==&graph && destroyed==1);
            checks=check.calls;assert(checks>=5);
        } else {
            assert(!released && ctx.qwen35GraphOwner==&graph && destroyed<=1);
            if (ctx.qwen35SharedRoleBinding) {
                assert(!destroyed);
                if (ctx.qwen35SharedRoleBinding->attempted) {
                    Check again;
                    assert(!PioneerQ35ReleaseFabricRoleOwner(ctx,7,again) && !destroyed);
                }
                delete ctx.qwen35SharedRoleBinding;ctx.qwen35SharedRoleBinding=nullptr;
            } else assert(destroyed==1);
        }
    }
    for (unsigned denial=0;denial<4;denial++) {
        destroyed=0;int graph=1,session=2;Context ctx{new Role,&graph,nullptr};Check check;
        if (denial==0) ctx.recoveryFreshSession=&session;
        if (denial==1) ctx.qwen35SharedRoleBinding->ready=false;
        if (denial==2) ctx.qwen35SharedRoleBinding->attempted=true;
        if (denial==3) ctx.qwen35GraphOwner=nullptr;
        assert(!PioneerQ35ReleaseFabricRoleOwner(ctx,7,check) && !destroyed);
        delete ctx.qwen35SharedRoleBinding;
    }
    {
        destroyed=0;int graph=1,other=2;Context ctx{new Role,&graph,nullptr};GraphSwap check{ctx,&other};
        assert(!PioneerQ35ReleaseFabricRoleOwner(ctx,7,check) && !destroyed
            && ctx.qwen35GraphOwner==&other && !ctx.qwen35SharedRoleBinding->attempted);
        delete ctx.qwen35SharedRoleBinding;
    }
    std::printf("PASS role-owner release: %u simulated ticket-loss checks, no destruction replay; NO HARDWARE\n",checks);
}
