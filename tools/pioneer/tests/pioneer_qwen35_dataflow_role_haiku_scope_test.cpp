/* SPDX-License-Identifier: MIT */
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_DATAFLOW_REWRITE 1
#include <KernelExport.h>
#include "../PioneerQwen35DataflowRolePhysicalPlan.h"
#include "../PioneerQwen35DataflowOwner.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPciProfile.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeDramSamplesProbe.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeResidentReadProbe.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
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
struct PioneerStoreContext;
#ifdef PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY
class PioneerQwen35RoleBorrowedScopeWitness;
#endif
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
class PioneerQwen35SharedRoleBinding;
namespace PioneerQwen35 { struct DataflowWorkerSnapshot; }
#endif
struct PioneerProbeCookie { uint32_t magic=0x50313030; team_id team=11; PioneerStoreContext* context=nullptr; };
struct FakeService {
    bool ready=true;
    bool capturePending=false;
    bool Ready() const { return ready; }
    bool SharedRoleCapturePending() const { return capturePending; }
};
struct FakeGraph { bool retained=false; const void* run=nullptr; bool closePermitted=false; unsigned ackCore=UINT32_MAX; };
struct PioneerSessionIO {
    tt_pioneer_profile profile=GoodProfile();
    PioneerStoreContext* qwen35DataflowContext=nullptr;
#ifdef PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY
    PioneerQwen35RoleBorrowedScopeWitness* qwen35BorrowedRoleScope=nullptr;
#endif
    unsigned runtimeCore=0; bool fleetEnabled=true,mapping=true;
    bool simulateArc=false,tlbOverride=false;
    bool firmwareMatches=false;
    unsigned selectCalls=0,tlbWrites=0;
    uint32_t slot1[3]{};
    uint32_t reset=0x47800,nocDoorbell[2][4]{},nocOutstanding[2][32]{};
    uint32_t nocConfig[2]{},nocAvailable[2]{},nocOverflow[2]{};
    mutable unsigned resetReads=0;
    int resetChangeAt=-1;
    unsigned sharedResetWrites=0; bool dropSharedResetWrite=false;
    DataflowRoleMailbox roleBoxes[DataflowBarrierWorkers]{};
    DataflowWorkerMailbox workerBoxes[DataflowBarrierWorkers]{};
    DataflowQueueControl queueBoxes[DataflowBarrierWorkers][DataflowCircularBufferCount]{};
    unsigned roleStopWrites=0; bool dropRoleStopWrite=false;
    unsigned computeStopWrites=0,holdWrites=0; bool dropComputeStopWrite=false,dropHoldWrite=false;
    uint32_t rolePc[2]{};
    uint32_t computePc=0;
    bool SelectRuntimeCore(unsigned core) {
        if (core>=DataflowBarrierWorkers) return false;
        ++selectCalls; runtimeCore=core; tlbOverride=false; return true;
    }
    bool MapQ38Panel() const { return mapping; }
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
    bool StopAndAuditAll(unsigned& stage,unsigned& core) { stage=0; core=DataflowBarrierWorkers; return true; }
    bool SharedCaptureAndStageHeld();
    bool SharedArmAndBeginRelease();
    bool SharedFleetHeldAndQuiescent();
    bool SharedStartCore(unsigned);
    bool SharedCoreReady(unsigned,const DataflowWorkerSnapshot&);
    bool SharedRuntimeStable(unsigned);
    bool SharedFaultCoreSettled(unsigned,const DataflowWorkerSnapshot&);
    bool SharedCoreIdleForWrite(unsigned,unsigned,unsigned);
    bool SharedStopAndHoldAll(unsigned&,unsigned&);
    bool SharedRestoreRolesHeld();
    bool SharedVerifyRolesAfterResidentRestore();
    bool SharedFinalizeBeforeResume();
#endif
    bool DataflowPanelMappingStable() const { return mapping; }
    bool SharedRoleFirmwareMatches() const { return firmwareMatches; }
    uint32_t DramTlbRead(unsigned slot,unsigned word) {
        if (slot==1 && tlbOverride) return slot1[word];
        const uint32_t route=DataflowBarrierWorkerRoute(runtimeCore);
        return BlackholeDramFixedTlbWord(slot?0xffa00000U:0U,route&63,(route>>6)&63,false,word);
    }
    void DramTlbWrite(unsigned slot,unsigned word,uint32_t value) {
        assert(simulateArc && slot==1 && word<3);
        if (!tlbOverride) {
            for (unsigned i=0;i<3;i++) slot1[i]=DramTlbRead(1,i);
            tlbOverride=true;
        }
        ++tlbWrites; slot1[word]=value;
    }
    bool SharedArcResetMapped() const { return simulateArc; }
    uint32_t SharedArcL2CpuResetRead() { assert(simulateArc && tlbOverride); return 0x0f; }
    uint32_t ScratchRead(unsigned word) { assert(simulateArc && tlbOverride && word==2); return 5; }
    uint32_t ResetRead() const {
        return int(resetReads++)==resetChangeAt ? 0x6000 : reset;
    }
    bool SharedRoleHoldReset() { ++holdWrites; if (!dropHoldWrite) reset=0x47800; return true; }
    uint32_t RuntimePcRead() const { return computePc; }
    bool SharedRoleReleaseReset() { ++sharedResetWrites; if (!dropSharedResetWrite) reset=0x6000; return true; }
    uint32_t SharedRolePcRead(unsigned role) { return role<2?rolePc[role]:UINT32_MAX; }
    bool ReadCoreMemory(unsigned core,unsigned address,void* output,unsigned bytes) {
        if (core>=DataflowBarrierWorkers || !output) return false;
        runtimeCore=core;
        if (address==DataflowRoleMailboxAddress && bytes==sizeof(DataflowRoleMailbox))
            memcpy(output,&roleBoxes[core],bytes);
        else if (address==DataflowWorkerMailboxAddress && bytes==sizeof(DataflowWorkerMailbox))
            memcpy(output,&workerBoxes[core],bytes);
        else if (address==DataflowQueueStateAddress && bytes==sizeof(queueBoxes[core]))
            memcpy(output,queueBoxes[core],bytes);
        else return false;
        return true;
    }
    bool ReadWorkerSnapshot(unsigned core,DataflowWorkerSnapshot& snapshot) {
        if (core>=DataflowBarrierWorkers) return false;
        const auto& box=workerBoxes[core];
        snapshot={box.commandSequence,box.acknowledgedSequence,box.completedSequence,
            box.state,box.failure,box.phase,0,0};
        return true;
    }
    void Q38PanelWrite(unsigned address,uint32_t value) {
        if (address==DataflowRoleMailboxAddress+offsetof(DataflowRoleMailbox,stopRequest)) {
            ++roleStopWrites; if (!dropRoleStopWrite) roleBoxes[runtimeCore].stopRequest=value;
        } else {
            assert(address==DataflowWorkerMailboxAddress+offsetof(DataflowWorkerMailbox,shutdownSequence));
            ++computeStopWrites; if (!dropComputeStopWrite) workerBoxes[runtimeCore].shutdownSequence=value;
        }
    }
    uint32_t Q38PanelRead(unsigned address) {
        if (address==DataflowRoleMailboxAddress+offsetof(DataflowRoleMailbox,stopRequest))
            return roleBoxes[runtimeCore].stopRequest;
        assert(address==DataflowWorkerMailboxAddress+offsetof(DataflowWorkerMailbox,shutdownSequence));
        return workerBoxes[runtimeCore].shutdownSequence;
    }
    uint32_t ResidentNocRead(unsigned offset) {
        // Match the actual PioneerSessionIO wrapper rather than returning
        // values for registers that its read-only filter would reject.
        if (!BlackholeResidentReadRegister(offset)) return UINT32_MAX;
        const unsigned noc=offset>=0x10000?1:0,at=offset-(noc?0x10000:0);
        if (at==0x100) return nocConfig[noc];
        if (at==0x64) return nocAvailable[noc];
        if (at==0x68) return nocOverflow[noc];
        if (at==DataflowNocLogicalIdOffset) return DataflowBarrierWorkerRoute(runtimeCore);
        for (unsigned i=0;i<4;i++) if (at==0x40+i*0x800) return nocDoorbell[noc][i];
        if (at>=0x240 && at<0x2c0 && !(at&3)) return nocOutstanding[noc][(at-0x240)/4];
        assert(false); return UINT32_MAX;
    }
    void Pause() {}
};
struct PioneerResidentBanks { static constexpr uint64 Total=123456; };
struct PioneerStoreContext {
    PioneerSessionIO io; bool bankStorageOnly=true; uint64 bytes=PioneerResidentBanks::Total;
    FakeService* qwen35DataflowService=nullptr; FakeGraph* qwen35GraphOwner=nullptr;
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
    PioneerQwen35SharedRoleBinding* qwen35SharedRoleBinding=nullptr;
#endif
    PioneerStoreContext() { io.qwen35DataflowContext=this; }
};
static bool PioneerQ35GraphOwnerRetained(const FakeGraph* graph) { return graph && graph->retained; }
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
static bool PioneerQ35GraphOwnerOwnsRetainedRun(const FakeGraph* graph,const void* run) {
    return graph && graph->retained && run && graph->run==run;
}
static bool PioneerQ35GraphOwnerFaultClosePermitted(const FakeGraph* graph,const void* run) {
    return PioneerQ35GraphOwnerOwnsRetainedRun(graph,run) && graph->closePermitted;
}
static bool PioneerQ35GraphOwnerFaultAckPermitted(const FakeGraph* graph,const void* run,unsigned core) {
    return PioneerQ35GraphOwnerOwnsRetainedRun(graph,run) && graph->ackCore==core;
}
#endif
static PioneerProbeCookie* sRuntimeOwner=nullptr;
static bool sColdRecoveryRequired=false,sReloadQuiesced=false,sBankStorageOnly=true;
static uint64 sReloadGeneration=42;
static int32_t sLock=1;
static bool gLocked=false,gReleaseFail=false;
static bool gAllowWrite=false;
static unsigned gReleaseCalls=0,gMapCalls=0,gUnmapCalls=0;
static int gFailMap=-1,gFailUnmap=-1;
static thread_id gThread=5;
static team_id gTeam=11;
static std::vector<std::vector<uint32_t>> gMemory;
static std::vector<bool> gMapped;
static status_t AcquireProbeLock(PioneerProbeCookie* cookie,bool bank) {
    if (!bank || gLocked || sColdRecoveryRequired || sReloadQuiesced || sRuntimeOwner!=cookie) return -1;
    gLocked=true; return B_OK;
}
thread_id find_thread(const char*) { return gThread; }
status_t get_thread_info(thread_id id,thread_info* info) {
    if (id!=gThread || !info) return -1;
    info->team=gTeam; return B_OK;
}
status_t release_sem(int32_t lock) {
    assert(lock==sLock && gLocked); ++gReleaseCalls;
    if (gReleaseFail) return -1;
    gLocked=false; return B_OK;
}
area_id map_physical_memory(const char*,phys_addr_t physical,size_t bytes,uint32_t addressMode,
    uint32_t flags,void** output) {
    assert(gLocked && addressMode==B_ANY_KERNEL_ADDRESS && (flags&B_KERNEL_READ_AREA)
        && (flags&B_UNCACHED_MEMORY) && bool(flags&B_KERNEL_WRITE_AREA)==gAllowWrite);
    const unsigned index=gMapCalls++;
    DataflowRolePhysicalWindow plan{}; assert(DataflowRolePhysicalWindowPlan(index,plan));
    assert(physical==0x4900000000ULL+plan.offset && bytes==plan.bytes);
    if (int(index)==gFailMap) return -1;
    gMemory[index].assign(bytes/4,0x76540000U+index); gMapped[index]=true;
    *output=gMemory[index].data(); return int32_t(index);
}
status_t delete_area(area_id area) {
    assert(gLocked && area>=0 && unsigned(area)<gMapped.size() && gMapped[area]);
    ++gUnmapCalls;
    if (area==gFailUnmap) { gFailUnmap=-1; return -1; }
    gMapped[area]=false; return B_OK;
}
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35DataflowRoleHaikuScopeWitness.inc"
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
static void (*gBorrowedScopeHook)(PioneerSessionIO&)=nullptr;
#endif
#ifdef PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY
// Mirrors the real control path: create the private scope only after the
// session lock has been acquired and detach before releasing that lock.
static status_t Qwen35DataflowControl(PioneerProbeCookie* cookie,uint32_t mode,void*,size_t) {
    if (AcquireProbeLock(cookie,true)!=B_OK) return -1;
    auto* context=cookie->context;
    PioneerQwen35RoleBorrowedScopeWitness scope(cookie,context);
    if (mode==1) gTeam=12;
    const bool attached=scope.Attach();
    if (mode==1) {
        assert(!attached && !context->io.qwen35BorrowedRoleScope);
        gTeam=11;
    } else {
        assert(attached && context->io.qwen35BorrowedRoleScope==&scope && !scope.Attach());
        if (mode==2) {
            context->qwen35GraphOwner->retained=true;
            assert(!scope.ProveExclusiveRewrite(context->io));
            context->qwen35GraphOwner->retained=false;
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
        } else if (mode==6 || mode==7) {
            auto* graph=context->qwen35GraphOwner;
            int run=0,otherRun=0; FakeGraph other;
            graph->retained=true;graph->run=&run;
            assert(!scope.ProveExclusiveRewrite(context->io));
            if (mode==6) {
                assert(!scope.BeginRetainedFault(context->io,graph,&otherRun));
                assert(!scope.BeginRetainedFault(context->io,graph,&run)); // one-shot failed authority
            } else {
                assert(scope.BeginRetainedFault(context->io,graph,&run));
                assert(scope.ProveExclusiveRewrite(context->io));
                assert(scope.RetainedFaultAuditActive(context->io));
                assert(!scope.FaultAcknowledgementAuthorized(context->io,3));
                graph->ackCore=3;
                assert(scope.FaultAcknowledgementAuthorized(context->io,3)
                    && !scope.FaultAcknowledgementAuthorized(context->io,4));
                assert(!scope.ProveRoleFaultAuditSafe(context->io,0)
                    && !scope.ProveRoleFaultAckSafe(context->io,0));
                assert(!scope.PromoteRetainedFaultClose(context->io));
                graph->closePermitted=true;
                assert(scope.PromoteRetainedFaultClose(context->io) && scope.FaultCloseAuthorized(context->io));
                assert(!scope.RetainedFaultAuditActive(context->io));
                assert(!scope.FaultAcknowledgementAuthorized(context->io,3));
                assert(!scope.ProveRoleMemoryWriteSafe(context->io,0,0,0,4));
                graph->run=&otherRun;assert(!scope.ProveExclusiveRewrite(context->io));graph->run=&run;
                context->qwen35GraphOwner=&other;assert(!scope.ProveExclusiveRewrite(context->io));
                assert(!scope.EndRetainedFault(context->io,graph,&run));
                context->qwen35GraphOwner=graph;
                graph->run=nullptr; // retained allocation can disappear after successful close
                assert(scope.EndRetainedFault(context->io,graph,&run));
                assert(!scope.ProveExclusiveRewrite(context->io));
                assert(!scope.BeginRetainedFault(context->io,graph,&run));
            }
            graph->retained=false;graph->run=nullptr;graph->closePermitted=false;graph->ackCore=UINT32_MAX;
#endif
        } else {
            context->qwen35DataflowService->ready=false; // installing is not Ready
            assert(scope.ProveExclusiveRewrite(context->io));
            context->qwen35DataflowService->ready=true;
            assert(!scope.ProveRoleMemoryReadSafe(context->io,0,0));
            assert(!scope.ProveRoleMemoryWriteSafe(context->io,0,0,0,4));
            assert(!scope.ProveRoleControlWriteSafe(context->io,0,0,0x3c0));
            const unsigned savedCore=context->io.runtimeCore;
            const bool savedFirmware=context->io.firmwareMatches;
            context->io.runtimeCore=7; context->io.firmwareMatches=true;
            assert(scope.ProveRoleBootRoutingInspectionAllowed(context->io,7));
            assert(!scope.ProveRoleBootRoutingInspectionAllowed(context->io,8));
            assert(!scope.ProveRoleBootRoutingInspectionAllowed(context->io,DataflowBarrierWorkers));
            context->io.mapping=false;
            assert(!scope.ProveRoleBootRoutingInspectionAllowed(context->io,7));
            context->io.mapping=true;
            gThread=6; assert(!scope.ProveRoleBootRoutingInspectionAllowed(context->io,7)); gThread=5;
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
            context->io.firmwareMatches=false;
            assert(!scope.ProveRoleBootRoutingInspectionAllowed(context->io,7));
#endif
            context->io.runtimeCore=savedCore; context->io.firmwareMatches=savedFirmware;
            ++sReloadGeneration; assert(!scope.ProveExclusiveRewrite(context->io)); --sReloadGeneration;
            gThread=6; assert(!scope.ProveExclusiveRewrite(context->io)); gThread=5;
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
            if (mode==5 && gBorrowedScopeHook) gBorrowedScopeHook(context->io);
#endif
        }
        if (mode==3) {
            gThread=6; assert(!scope.Detach()); gThread=5;
            assert(sColdRecoveryRequired && !context->io.qwen35BorrowedRoleScope);
        } else assert(scope.Detach() && !context->io.qwen35BorrowedRoleScope);
        assert(!scope.Attach()); // borrowed scopes cannot be re-used
    }
    if (mode==4) gReleaseFail=true;
    const status_t release=release_sem(sLock);
    if (release!=B_OK) { sColdRecoveryRequired=true; return release; }
    assert(!gLocked);
    return B_OK;
}
#endif
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
static status_t Qwen35DataflowGenerate(PioneerProbeCookie* cookie,void*,size_t) {
    if (AcquireProbeLock(cookie,true)!=B_OK) return -1;
    auto* context=cookie->context;
    PioneerQwen35RoleBorrowedScopeWitness scope(cookie,context);
    const bool attached=scope.Attach();
    if (attached) {
        assert(context->io.qwen35BorrowedRoleScope==&scope && scope.ProveExclusiveRewrite(context->io));
        assert(scope.Detach() && !context->io.qwen35BorrowedRoleScope);
    }
    const status_t released=release_sem(sLock);
    if (released!=B_OK) sColdRecoveryRequired=true;
    return attached?released:-1;
}
#endif
static void ResetMaps() {
    gMapCalls=gUnmapCalls=0; gFailMap=gFailUnmap=-1;
    gAllowWrite=false;
    gMemory.assign(DataflowRolePhysicalWindowCount,{});
    gMapped.assign(DataflowRolePhysicalWindowCount,false);
}
int main()
{
    PioneerStoreContext context; FakeService service; FakeGraph graph;
    context.qwen35DataflowService=&service; context.qwen35GraphOwner=&graph;
    PioneerProbeCookie cookie; cookie.context=&context; sRuntimeOwner=&cookie;
    {
        PioneerQwen35RoleHaikuScopeWitness scope;
        assert(scope.Open(&cookie) && gLocked && scope.ProveExclusiveRewrite(context.io));
        assert(!scope.ProveRoleMemoryReadSafe(context.io,0,DataflowRolePhysicalBriscRam));
        gThread=6; assert(!scope.Close() && gLocked); gThread=5;
        assert(scope.Close() && !gLocked && !scope.Close());
    }
#ifdef PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY
    for (unsigned mode=0;mode<5;mode++) {
        const unsigned releases=gReleaseCalls;
        const status_t status=Qwen35DataflowControl(&cookie,mode,nullptr,0);
        assert(gReleaseCalls==releases+1);
        if (mode==4) {
            assert(status!=B_OK && sColdRecoveryRequired && gLocked);
            gReleaseFail=false; gLocked=false; // simulation only
        } else assert(status==B_OK && !gLocked);
        sColdRecoveryRequired=false;
    }
#endif
    {
        PioneerQwen35RoleHaikuScopeWitness scope; gTeam=12;
        assert(!scope.Open(&cookie) && !gLocked); gTeam=11;
    }
    {
        ResetMaps(); PioneerQwen35RoleHaikuViewsWitness owner;
        assert(owner.Open(&cookie) && gLocked);
        uint32_t value=0;
        assert(owner.Read(0,DataflowRolePhysicalPc,0x140,value));
        assert(value==0x76540000U+DataflowRolePhysicalPc);
        value=77;
        assert(!owner.Read(0,DataflowRolePhysicalBriscRam,0,value) && value==77);
        assert(!owner.Read(0,DataflowRolePhysicalNoc0,0x20c,value));
        gMemory[DataflowRolePhysicalControl][0x1b0/4]=0x47800;
        for (unsigned noc=0;noc<2;noc++) {
            const unsigned window=noc?DataflowRolePhysicalNoc1:DataflowRolePhysicalNoc0;
            std::fill(gMemory[window].begin(),gMemory[window].end(),0);
        }
        for (unsigned core=0;core<120;core++) {
            context.io.runtimeCore=core;
            for (unsigned noc=0;noc<2;noc++) {
                const unsigned window=noc?DataflowRolePhysicalNoc1:DataflowRolePhysicalNoc0;
                gMemory[window][DataflowNocLogicalIdOffset/4]=DataflowBarrierWorkerRoute(core);
            }
            assert(owner.AuditHeldAndQuiet(core));
        }
        context.io.runtimeCore=0;
        for (unsigned noc=0;noc<2;noc++) {
            const unsigned window=noc?DataflowRolePhysicalNoc1:DataflowRolePhysicalNoc0;
            gMemory[window][DataflowNocLogicalIdOffset/4]=DataflowBarrierWorkerRoute(0);
        }
        gMemory[DataflowRolePhysicalControl][0x1b0/4]=0x46800;
        assert(!owner.AuditHeldAndQuiet(0));
        gMemory[DataflowRolePhysicalControl][0x1b0/4]=0x47800;
        for (unsigned noc=0;noc<2;noc++) {
            const unsigned window=noc?DataflowRolePhysicalNoc1:DataflowRolePhysicalNoc0;
            for (unsigned initiator=0;initiator<4;initiator++) {
                const unsigned index=(0x40+initiator*0x800)/4;
                gMemory[window][index]=1; assert(!owner.AuditHeldAndQuiet(0));
                gMemory[window][index]=0;
            }
            for (unsigned id=0;id<32;id++) {
                const unsigned index=(0x240+id*4)/4;
                gMemory[window][index]=1; assert(!owner.AuditHeldAndQuiet(0));
                gMemory[window][index]=0;
            }
            gMemory[window][DataflowNocLogicalIdOffset/4]=DataflowBarrierWorkerRoute(1);
            assert(!owner.AuditHeldAndQuiet(0));
            gMemory[window][DataflowNocLogicalIdOffset/4]=DataflowBarrierWorkerRoute(0);
        }
        assert(owner.AuditHeldAndQuiet(0));
        assert(!owner.Read(0,DataflowRolePhysicalBriscRam,0,value));
        assert(owner.Close() && !gLocked && gUnmapCalls==DataflowRolePhysicalWindowCount);
    }
    {
        ResetMaps(); gFailMap=4; PioneerQwen35RoleHaikuViewsWitness owner;
        assert(!owner.Open(&cookie) && gLocked && gMapCalls==5);
        gFailUnmap=2; assert(!owner.Close() && gLocked);
        assert(owner.Close() && !gLocked);
    }
#ifdef PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY
    {
        ResetMaps(); gAllowWrite=true; PioneerQwen35RoleHaikuWritableViewsWitness owner;
        assert(owner.Open(&cookie) && gLocked && gMapCalls==DataflowRolePhysicalControl+1);
        const uint32_t value=0x12345678U;
        assert(!owner.Write(0,DataflowRolePhysicalExtraCode,0,&value,sizeof(value)));
        assert(gMemory[DataflowRolePhysicalExtraCode][0]==0x76540000U+DataflowRolePhysicalExtraCode);
        assert(!owner.WriteResetDisable(0,0,DataflowRolePreserveResetMask));
        assert(owner.Close() && !gLocked);
        gAllowWrite=false;
    }
#endif
    {
        PioneerQwen35RoleHaikuScopeWitness scope;
        assert(scope.Open(&cookie)); gReleaseFail=true;
        const unsigned before=gReleaseCalls;
        assert(!scope.Close() && sColdRecoveryRequired && gReleaseCalls==before+1);
        assert(!scope.Close() && gReleaseCalls==before+1);
        gReleaseFail=false; gLocked=false; sColdRecoveryRequired=false;
    }
    std::puts("PASS real Haiku scope witness lock, held/NoC status samples, map and cleanup faults; SIMULATED ONLY");
    return 0;
}
