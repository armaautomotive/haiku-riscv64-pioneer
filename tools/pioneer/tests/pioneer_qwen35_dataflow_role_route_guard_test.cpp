/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35DataflowRoleRouteGuard.h"
#include <cassert>
#include <cstdio>
using namespace PioneerQwen35;
static tt_pioneer_profile GoodProfile()
{
    tt_pioneer_profile p{};
    p.version=1; p.stage=6; p.ready=1; p.card_bus=3; p.root_bus=2;
    p.root_buses=0x003f0100; p.root_memory=0x0000fff0; p.root_prefetch=0x0001fff1;
    p.controller=0x7062000000; p.config=0x4800000000; p.config_length=0x1000;
    p.ranges[0]={0x4810800000,0x800000,0x400000,2,0};
    p.ranges[1]={0x4890000000,0x90000000,0x30000000,1,8};
    p.ranges[2]={0x48c0000000,0xc0000000,0x10000000,1,0};
    p.ranges[3]={0x4900000000,0x4900000000,0x200000000,1,12};
    p.ranges[4]={0x4b00000000,0x4b00000000,0x100000000,1,4};
    p.bar0_host_start=0x4900000000; p.bar0_host_end=0x491fffffff;
    p.bar2_host_start=0x4920000000; p.bar2_host_end=0x49200fffff;
    return p;
}
struct MockIO {
    tt_pioneer_profile profile=GoodProfile();
    unsigned runtimeCore=0,tlbReads=0,routeReads=0;
    bool fleetEnabled=true,mapping=true,changeTlb=false,changeRoute=false;
    bool DataflowPanelMappingStable() const { return mapping; }
    uint32_t DramTlbRead(unsigned slot,unsigned word) {
        const uint32_t route=DataflowBarrierWorkerRoute(runtimeCore);
        uint32_t value=BlackholeDramFixedTlbWord(slot?0xffa00000U:0U,route&63,(route>>6)&63,false,word);
        if (changeTlb && ++tlbReads>6) value^=1;
        return value;
    }
    uint32_t ResidentNocRead(unsigned offset) {
        assert(offset==DataflowNocLogicalIdOffset);
        uint32_t value=DataflowBarrierWorkerRoute(runtimeCore);
        if (changeRoute && ++routeReads>1) value^=1;
        return value;
    }
    void Pause() {}
};
struct Scope {
    bool exclusive=true,memorySafe=false;
    bool ProveExclusiveRewrite(MockIO&) const { return exclusive; }
    bool ProveRoleMemoryReadSafe(MockIO&,unsigned core,unsigned) const { return memorySafe && core<120; }
};
int main()
{
    MockIO io; Scope scope; PioneerQwen35RoleRouteGuard<MockIO,Scope> guard(io,scope);
    assert(guard.ProvePinnedBar(io.profile.bar0_host_start,io.profile.bar0_host_end));
    for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
        io.runtimeCore=core; assert(guard.ProveSelectedRoute(core));
    }
    assert(!guard.ProveSelectedRoute(DataflowBarrierWorkers));
    io.runtimeCore=7; assert(!guard.ProveSelectedRoute(8));
    assert(!guard.ProveReadAllowed(7,DataflowRolePhysicalBriscRam,0));
    scope.memorySafe=true; assert(guard.ProveReadAllowed(7,DataflowRolePhysicalBriscRam,0));
    scope.memorySafe=false; assert(guard.ProveReadAllowed(7,DataflowRolePhysicalPc,0x140));
    assert(!guard.ProveReadAllowed(7,DataflowRolePhysicalNoc0,0x20c));
    io.mapping=false; assert(!guard.ProveSelectedRoute(7)); io.mapping=true;
    io.changeTlb=true; io.tlbReads=0; assert(!guard.ProveSelectedRoute(7)); io.changeTlb=false;
    io.changeRoute=true; io.routeReads=0; assert(!guard.ProveSelectedRoute(7)); io.changeRoute=false;
    scope.exclusive=false; assert(!guard.ProveSelectedRoute(7)); scope.exclusive=true;
    io.profile.bar0_host_end--; assert(!guard.ProvePinnedBar(io.profile.bar0_host_start,io.profile.bar0_host_end));
    std::puts("PASS pinned Qwen35 role BAR and read-only selected-route guard; SIMULATED ONLY");
}
