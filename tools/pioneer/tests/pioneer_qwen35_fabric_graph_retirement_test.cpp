/* SPDX-License-Identifier: MIT */
// Exercise real retirement ordering with simulated graph and sticky bank owner.
#include <cassert>
#include <cstdint>
#include <cstdio>
struct Graph { bool ready=true;bool FabricContextRetirementReady(uint64_t ticket) const { return ready && ticket==7; } };
struct Context {
    Graph* qwen35GraphOwner=nullptr;void* recoveryFreshSession=nullptr;void* qwen35SharedRoleBinding=nullptr;
    bool recoveryFabricGraphRetirementAttempted=false,recoveryFabricGraphRetirementRecorded=false;
};
struct Check { unsigned calls=0,failAt=0;bool live=true;
    bool operator()() { if (++calls==failAt) live=false;return live; } };
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35FabricGraphRetirement.h"
struct GraphSwap { Context& context;Graph* replacement;
    bool operator()() { context.qwen35GraphOwner=replacement;return true; } };
int main() {
    unsigned checks=0;
    for (unsigned fail=0;;fail++) {
        if (fail && fail>checks) break;
        Graph graph;Context ctx{&graph};Check check;check.failAt=fail;
        const bool recorded=PioneerQ35RecordFabricGraphRetirement(ctx,7,check);
        if (!fail) {
            assert(recorded && ctx.recoveryFabricGraphRetirementAttempted
                && ctx.recoveryFabricGraphRetirementRecorded && ctx.qwen35GraphOwner==&graph);
            checks=check.calls;assert(checks>=4);
        } else {
            assert(!recorded && !ctx.recoveryFabricGraphRetirementRecorded && ctx.qwen35GraphOwner==&graph);
            if (ctx.recoveryFabricGraphRetirementAttempted) {
                Check again;assert(!PioneerQ35RecordFabricGraphRetirement(ctx,7,again));
            }
        }
    }
    for (unsigned denial=0;denial<5;denial++) {
        Graph graph;int role=1,session=2;Context ctx{&graph};Check check;
        if (denial==0) graph.ready=false;
        if (denial==1) ctx.qwen35SharedRoleBinding=&role;
        if (denial==2) ctx.recoveryFreshSession=&session;
        if (denial==3) ctx.recoveryFabricGraphRetirementAttempted=true;
        if (denial==4) ctx.recoveryFabricGraphRetirementRecorded=true;
        assert(!PioneerQ35RecordFabricGraphRetirement(ctx,7,check) && ctx.qwen35GraphOwner==&graph);
    }
    Graph graph,other;Context ctx{&graph};GraphSwap swap{ctx,&other};
    assert(!PioneerQ35RecordFabricGraphRetirement(ctx,7,swap)
        && !ctx.recoveryFabricGraphRetirementAttempted && ctx.qwen35GraphOwner==&other);
    std::printf("PASS graph retirement preserves sticky bank owner: %u simulated ticket-loss checks; NO HARDWARE\n",checks);
}
