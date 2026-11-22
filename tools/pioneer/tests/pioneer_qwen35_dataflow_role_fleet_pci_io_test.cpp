/* SPDX-License-Identifier: MIT */
// Concrete fleet PCI adapter and real Haiku mapper over simulated MMIO.
#define PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY 1
#include <cstring>
#define main qwen35_haiku_scope_test_main
#include "pioneer_qwen35_dataflow_role_haiku_scope_test.cpp"
#undef main

struct FleetPciTestScope {
    bool exclusive=true,held=true,routing=true,stable=true,read=true,write=true,control=true,release=false,stop=false,park=false;
    bool ProveExclusiveRewrite(PioneerSessionIO&) const { return gLocked && exclusive; }
    bool ProveRoleMemoryReadSafe(PioneerSessionIO&,unsigned,unsigned) const { return held && stable && read; }
    bool ProveRoleMemoryWriteSafe(PioneerSessionIO&,unsigned,unsigned,unsigned,unsigned) const
        { return held && stable && write; }
    bool ProveRoleControlWriteSafe(PioneerSessionIO&,unsigned,uint32_t,uint32_t) const
        { return held && stable && control; }
    bool ProveRoleFleetHeldAndQuiet(PioneerSessionIO&,unsigned) const { return held; }
    bool ProveRoleDefaultBootRouting(PioneerSessionIO&,unsigned) const { return held && routing; }
    bool ProveRoleLocalRamStable(PioneerSessionIO&,unsigned) const { return held && stable; }
    bool ProveRoleReleaseSafe(PioneerSessionIO&,unsigned) const { return release; }
    bool ProveRoleStopSafe(PioneerSessionIO&,unsigned,bool) const { return stop; }
    bool ProveRolePostStopSafe(PioneerSessionIO&,unsigned) const { return park; }
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
    bool attempted=false;
    context.io.runtimeCore=7;
    assert(!io.ReleaseCore(8,attempted) && !attempted && context.io.runtimeCore==7
        && !context.io.sharedResetWrites);
    scope.release=true;
    gMemory[DataflowRolePhysicalControl][0x224/4]&=~uint32_t(DataflowRolePreserveResetMask);
    assert(!io.ReleaseCore(8,attempted) && !attempted && !context.io.sharedResetWrites);
    gMemory[DataflowRolePhysicalControl][0x224/4]|=DataflowRolePreserveResetMask;
    assert(io.ReleaseCore(8,attempted) && attempted && context.io.sharedResetWrites==1
        && context.io.reset==0x6000);
    assert(!io.ReleaseCore(8,attempted) && context.io.sharedResetWrites==1);
    context.io.reset=0x47800; context.io.dropSharedResetWrite=true; attempted=false;
    assert(!io.ReleaseCore(9,attempted) && attempted && context.io.sharedResetWrites==2);
    assert(!io.ReleaseCore(9,attempted) && context.io.sharedResetWrites==2);
    context.io.dropSharedResetWrite=false;
    bool allAttempts[DataflowBarrierWorkers]{};
    for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
        context.io.reset=0x47800;
        assert(io.ReleaseCore(core,allAttempts[core]) && allAttempts[core]);
    }
    assert(context.io.sharedResetWrites==2+DataflowBarrierWorkers);
    auto initRoleBox=[&](unsigned core) {
        auto& box=context.io.roleBoxes[core];
        box.magic=DataflowRoleMailboxMagic; box.version=DataflowRoleMailboxVersion;
        for (unsigned role=0;role<DataflowRoleCount;role++)
            assert(DataflowRoleMailboxPublishStarted(box,role));
    };
    initRoleBox(8);
    bool stopAttempted=false; uint32_t stopToken=0;
    context.io.runtimeCore=7;
    assert(!io.RequestStop(8,false,stopAttempted,stopToken)
        && !stopAttempted && !stopToken && context.io.runtimeCore==7 && !context.io.roleStopWrites);
    scope.stop=true;
    assert(io.RequestStop(8,false,stopAttempted,stopToken) && stopAttempted && stopToken==1);
    assert(io.StopRequestObserved(8,stopToken) && context.io.roleStopWrites==1);
    assert(!io.RequestStop(8,false,stopAttempted,stopToken) && context.io.roleStopWrites==1);
    context.io.rolePc[DataflowRoleReader]=DataflowRoleReaderShimAddress+0x30;
    context.io.rolePc[DataflowRoleWriter]=DataflowRoleWriterShimAddress+0x30;
    auto& cleanBox=context.io.roleBoxes[8];
    scope.park=true;
    assert(!io.RolesStoppedAndParked(8,stopToken,context.io.rolePc[0],context.io.rolePc[1]));
    assert(DataflowRoleMailboxObserveStop(cleanBox,DataflowRoleReader));
    assert(!io.RolesStoppedAndParked(8,stopToken,context.io.rolePc[0],context.io.rolePc[1]));
    assert(DataflowRoleMailboxObserveStop(cleanBox,DataflowRoleWriter));
    scope.park=false;
    assert(!io.RolesStoppedAndParked(8,stopToken,context.io.rolePc[0],context.io.rolePc[1]));
    scope.park=true;
    assert(io.RolesStoppedAndParked(8,stopToken,context.io.rolePc[0],context.io.rolePc[1]));
    assert(!io.RolesStoppedAndParked(8,stopToken,context.io.rolePc[0]+4,context.io.rolePc[1]));
    assert(!io.RolesStoppedAndParked(8,stopToken,context.io.rolePc[0],DataflowRoleWriterShimLimit));
    initRoleBox(9); stopAttempted=false; stopToken=0; context.io.dropRoleStopWrite=true;
    assert(!io.RequestStop(9,false,stopAttempted,stopToken) && stopAttempted && stopToken==1);
    assert(!io.StopRequestObserved(9,stopToken) && context.io.roleStopWrites==2);
    context.io.roleBoxes[9].stopRequest=stopToken; // a delayed MMIO completion
    assert(io.StopRequestObserved(9,stopToken));
    assert(!io.RequestStop(9,false,stopAttempted,stopToken) && context.io.roleStopWrites==2);
    context.io.dropRoleStopWrite=false;
    initRoleBox(10); auto& faultBox=context.io.roleBoxes[10];
    assert(DataflowRoleMailboxIssue(faultBox,1));
    for (unsigned role=0;role<DataflowRoleCount;role++) {
        assert(DataflowRoleMailboxBegin(faultBox,role,1));
        assert(DataflowRoleMailboxComplete(faultBox,role,1,role!=DataflowRoleReader));
    }
    stopAttempted=false; stopToken=0;
    assert(!io.RequestStop(10,false,stopAttempted,stopToken) && !stopAttempted);
    assert(io.RequestStop(10,true,stopAttempted,stopToken) && stopAttempted && stopToken==1);
    assert(io.StopRequestObserved(10,stopToken) && faultBox.acknowledged==0);
    assert(views.Close()); gAllowWrite=false;
    assert(release_sem(sLock)==B_OK && !gLocked);
    std::puts("PASS role fleet PCI range translation and guarded MMIO on 120 simulated routes; NO HARDWARE");
}
