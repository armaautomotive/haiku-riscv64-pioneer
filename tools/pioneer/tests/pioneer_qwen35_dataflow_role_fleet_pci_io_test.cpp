/* SPDX-License-Identifier: MIT */
// Concrete fleet PCI adapter and real Haiku mapper over simulated MMIO.
#define PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY 1
#include <cstring>
#define main qwen35_haiku_scope_test_main
#include "pioneer_qwen35_dataflow_role_haiku_scope_test.cpp"
#undef main

struct FleetPciTestScope {
    bool exclusive=true,held=true,routing=true,stable=true,read=true,write=true,control=true;
    bool ProveExclusiveRewrite(PioneerSessionIO&) const { return gLocked && exclusive; }
    bool ProveRoleMemoryReadSafe(PioneerSessionIO&,unsigned,unsigned) const { return held && stable && read; }
    bool ProveRoleMemoryWriteSafe(PioneerSessionIO&,unsigned,unsigned,unsigned,unsigned) const
        { return held && stable && write; }
    bool ProveRoleControlWriteSafe(PioneerSessionIO&,unsigned,uint32_t,uint32_t) const
        { return held && stable && control; }
    bool ProveRoleFleetHeldAndQuiet(PioneerSessionIO&,unsigned) const { return held; }
    bool ProveRoleDefaultBootRouting(PioneerSessionIO&,unsigned) const { return held && routing; }
    bool ProveRoleLocalRamStable(PioneerSessionIO&,unsigned) const { return held && stable; }
};
int main()
{
    PioneerStoreContext context; FakeService service; FakeGraph graph;
    context.qwen35DataflowService=&service; context.qwen35GraphOwner=&graph;
    PioneerProbeCookie cookie; cookie.context=&context; sRuntimeOwner=&cookie;
    ResetMaps(); gAllowWrite=true; assert(AcquireProbeLock(&cookie,true)==B_OK);
    FleetPciTestScope scope;
    PioneerQwen35RoleHaikuMap platform;
    DataflowRoleWritableViews<PioneerQwen35RoleHaikuMap> views(platform);
    PioneerQwen35RoleRouteGuard<PioneerSessionIO,FleetPciTestScope> guard(context.io,scope);
    assert(views.Open(context.io.profile.bar0_host_start,context.io.profile.bar0_host_end,guard));
    PioneerQwen35RoleFleetPciIO<PioneerSessionIO,FleetPciTestScope,decltype(views)> io(context.io,scope,views);
    for (unsigned core=0;core<DataflowBarrierWorkers;core++)
        assert(io.AllRolesHeld(core) && io.DefaultBootRouting(core) && io.LocalRamStable(core));
    context.io.reset=0x46800; assert(!io.AllRolesHeld(7)); context.io.reset=0x47800;
    context.io.nocDoorbell[1][3]=1; assert(!io.AllRolesHeld(7)); context.io.nocDoorbell[1][3]=0;
    context.io.nocOutstanding[0][31]=1; assert(!io.AllRolesHeld(7)); context.io.nocOutstanding[0][31]=0;
    gMemory[DataflowRolePhysicalControl][0x23c/4]|=1U;
    assert(!io.DefaultBootRouting(7));
    gMemory[DataflowRolePhysicalControl][0x23c/4]&=~1U;
    gMemory[DataflowRolePhysicalControl][0x234/4]|=1U;
    assert(!io.DefaultBootRouting(7));
    gMemory[DataflowRolePhysicalControl][0x234/4]&=~1U;
    uint32_t words[]={0x11223344,0x55667788},out[2]={};
    assert(io.WriteBoot(7,DataflowRoleBriscBootAddress,words[0]));
    assert(io.ReadBoot(7,DataflowRoleBriscBootAddress,out[0]) && out[0]==words[0]);
    assert(io.WriteBoot(7,DataflowRoleNcriscBootAddress,words[1]));
    assert(io.ReadBoot(7,DataflowRoleNcriscBootAddress,out[0]) && out[0]==words[1]);
    assert(!io.WriteBoot(7,0x6000,words[0]));
    assert(io.Write(7,DataflowRoleExtraL1Address+8,words,sizeof(words)));
    assert(io.Read(7,DataflowRoleExtraL1Address+8,out,sizeof(out)) && !memcmp(out,words,sizeof(words)));
    assert(!io.Write(7,DataflowRoleExtraL1Limit-4,words,sizeof(words)));
    assert(io.WriteShim(7,DataflowRoleWriterShimAddress,words,sizeof(words)));
    assert(io.ReadShim(7,DataflowRoleWriterShimAddress,out,sizeof(out)) && !memcmp(out,words,sizeof(words)));
    assert(!io.WriteShim(7,DataflowRoleWriterShimLimit-4,words,sizeof(words)));
    for (unsigned role=0;role<DataflowRoleCount;role++) {
        const uint32_t base=role==DataflowRoleReader?DataflowRoleBriscLocalRam:DataflowRoleNcriscLocalRam;
        assert(io.WriteRam(7,role,base+8,words,sizeof(words)));
        assert(io.ReadRam(7,role,base+8,out,sizeof(out)) && !memcmp(out,words,sizeof(words)));
        assert(!io.WriteRam(7,role,base+DataflowRoleLocalRamBytes-4,words,sizeof(words)));
    }
    assert(!io.WriteRam(7,DataflowRoleCount,DataflowRoleBriscLocalRam,words,4));
    const uint32_t original=gMemory[DataflowRolePhysicalControl][0x224/4];
    assert(io.WriteResetDisable(7,original|DataflowRolePreserveResetMask));
    assert(!io.WriteResetDisable(7,(original|DataflowRolePreserveResetMask)^1));
    assert(io.WriteResetDisable(7,original));
    scope.write=false;
    assert(!io.Write(7,DataflowRoleExtraL1Address,words,4)); scope.write=true;
    scope.held=false;
    const unsigned selected=context.io.runtimeCore;
    assert(!io.AllRolesHeld(8) && context.io.runtimeCore==selected);
    assert(!io.Write(7,DataflowRoleExtraL1Address,words,4)); scope.held=true;
    context.io.mapping=false;
    assert(!io.Read(7,DataflowRoleExtraL1Address,out,4)); context.io.mapping=true;
    scope.routing=false; assert(!io.DefaultBootRouting(7)); scope.routing=true;
    scope.stable=false; assert(!io.LocalRamStable(7)); scope.stable=true;
    assert(views.Close()); gAllowWrite=false;
    assert(release_sem(sLock)==B_OK && !gLocked);
    std::puts("PASS role fleet PCI range translation and guarded MMIO on 120 simulated routes; NO HARDWARE");
}
