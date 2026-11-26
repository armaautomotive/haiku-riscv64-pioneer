/* SPDX-License-Identifier: MIT */
// Actual role PCI adapter and flash view over fake role/MMIO platforms.
#define PIONEER_Q35_FLEET_TEST_NO_MAIN 1
#define PIONEER_Q35_FLASH_PCI_FIXTURE_ONLY 1
#define PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS 1
#define PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS 1
#define PIONEER_QWEN35_RECOVERY_RUN_ENABLE 1
#define TEST_SHARED_POST_RESET 1
#include "pioneer_qwen35_dataflow_role_fleet_pci_io_test.cpp"
#include "pioneer_qwen35_flash_code_pci_view_test.cpp"
#include "../PioneerQwen35DataflowFlashRolePciScope.h"
struct ActualRoleFlashGuard : Guard {
    PioneerSessionIO& io;
    const void* expectedLedger=nullptr;
    explicit ActualRoleFlashGuard(PioneerSessionIO& session):io(session) {}
    bool ProveFlashRoleLedgerBound(const void* role,const void* flash,const void* ledger) const {
        return role && flash && ledger && ledger==expectedLedger;
    }
    bool SelectFlashRoute(unsigned core) {
        return Guard::SelectFlashRoute(core) && io.SelectRuntimeCore(core) && io.runtimeCore==core;
    }
    bool ProveSelectedRoute(unsigned core) const {
        return Guard::ProveSelectedRoute(core) && io.runtimeCore==core;
    }
    bool ProveFlashRoleRouteSame(const void* role,const void* flash,unsigned core) const {
        return role && flash && ProveSelectedRoute(core);
    }
    bool ProveFlashRoleWriteDrained(const void* role,const void* flash) const {
        return role && flash && drained;
    }
    bool ProveFlashRoleCommonAuthority(const void* a,const void* b,const void* c,const void* d) {return a && b && c && d;}
    bool ProveFlashRoleSnapshotsSafe(const uint32_t* a,unsigned aw,const uint32_t* b,unsigned bw,bool) {
        return a && b && aw && bw;
    }
    bool ProveFlashRoleRamSnapshotSafe(const uint32_t* a,unsigned aw,const uint32_t* b,unsigned bw,bool) {
        return a && b && aw && bw;
    }
    bool ProveFlashRoleImagesSafe(const uint32_t* a,unsigned aw,const uint32_t* b,unsigned bw,
        const uint32_t* c,unsigned cw,const uint32_t* d,unsigned dw) {
        return a && aw && b && bw && c && cw && d && dw;
    }
    bool ProveFlashRoleRestoreSafe(bool retry) {return !retry || drained;}
};
struct ActualRoleScope : FleetPciTestScope {
    bool ProveRoleHeldImageMutationAllowed(PioneerSessionIO&,bool) const {return true;}
};
using ActualRoleIO=PioneerQwen35RoleFleetPciIO<PioneerSessionIO,ActualRoleScope,
    DataflowRoleWritableViews<PioneerQwen35RoleHaikuMap>>;
using ActualScope=DataflowFlashRolePciScope<ActualRoleIO,DataflowFlashCodePciView<Platform>,ActualRoleFlashGuard>;
// Instantiate the real parent against the actual role adapter. The compiler
// must find every held-only operation; this does not supply physical proofs.
bool CompileHeldComposition(DataflowRoleFleetRecoveryOwner& owner,ActualScope& io,
    uint32_t* code,uint32_t* ram,uint32_t* flash,const uint32_t* image) {
    return owner.CaptureAllStacklessHeldImages(code,owner.CodeWords(),ram,owner.RamWords(),io)
        && owner.CaptureFlashAll(flash,DataflowFlashFleetSnapshotWords,io)
        && owner.StageFlashAll(image,1,io)
        && owner.StageHeldImages(image,1,image,1,image,1,image,1,io)
        && owner.RestoreHeldImagesBeforeRelease(io) && owner.VerifyAfterResidentRestore(io);
}
#ifndef PIONEER_Q35_FLASH_ACTUAL_ROLE_FIXTURE_ONLY
int main() {
    PioneerStoreContext context;FakeService service;FakeGraph graph;
    context.qwen35DataflowService=&service;context.qwen35GraphOwner=&graph;
    PioneerProbeCookie cookie;cookie.context=&context;sRuntimeOwner=&cookie;
    ResetMaps();gAllowWrite=true;assert(AcquireProbeLock(&cookie,true)==B_OK);
    gPerCoreRoleWindows=true;gRoleWindowCore=gRoleSelectedCore=context.io.runtimeCore;
    ActualRoleScope roleGuard;PioneerQwen35RoleHaikuMap rolePlatform;
    DataflowRoleWritableViews<PioneerQwen35RoleHaikuMap> views(rolePlatform);
    PioneerQwen35RoleRouteGuard<PioneerSessionIO,ActualRoleScope> route(context.io,roleGuard);
    assert(views.Open(context.io.profile.bar0_host_start,context.io.profile.bar0_host_end,route));
    ActualRoleFlashGuard guard(context.io);Platform flashPlatform{guard};
    DataflowFlashCodePciView<Platform> flashView(flashPlatform);
    assert(flashView.Open(0x100000000ULL,0x11fffffffULL,guard));
    DataflowFlashRoleRouteLedger ledger;
    guard.expectedLedger=&ledger;
    ActualScope io(flashView,guard,ledger,context.io,roleGuard,views);
    gRoleTileWindows[7][DataflowRolePhysicalExtraCode][0]=0x1234abcd;
    flashPlatform.words[7*DataflowFlashCodeWords]=0x73ace024;
    uint32_t observed=0;
    assert(io.Read(7,DataflowRoleExtraL1Address,&observed,4) && observed==0x1234abcd);
    assert(io.FlashReadAllowed(7,DataflowFlashCodeAddress,4));
    assert(io.Read(7,DataflowFlashCodeAddress,&observed,4) && observed==0x73ace024);
    assert(io.FlashReadAllowed(7,DataflowFlashCodeAddress,4) && context.io.SelectRuntimeCore(8));
    const unsigned readsBeforeDrift=flashPlatform.reads;
    assert(!io.Read(7,DataflowFlashCodeAddress,&observed,4) && flashPlatform.reads==readsBeforeDrift);
    const unsigned reads=flashPlatform.reads;
    assert(!io.Read(7,DataflowFlashCodeLimit-4,&observed,8) && flashPlatform.reads==reads);
    // Both actual adapters participate in the held-only owner lifecycle.
    // The role mapper and authority are simulated and share fake storage.
    roleGuard.imageInspection=true;
    // Flush the selected mapped window to the fake per-core backing store.
    assert(context.io.SelectRuntimeCore(8));
    std::vector<std::vector<std::vector<uint32_t>>> originalRole;
    for (const auto& tile:gRoleTileWindows) originalRole.push_back(tile);
    const auto originalFlash=flashPlatform.words;
    DataflowRoleFleetRecoveryOwner owner;
    std::vector<uint32_t> code(owner.CodeWords()),ram(owner.RamWords()),flash(DataflowFlashFleetSnapshotWords);
    const uint32_t image[]={0x12345678},reader[]={0x11},writer[]={0x22},rs[]={0x33},ws[]={0x44};
    assert(owner.SelectAutoRestoreMode());
    assert(owner.CaptureAllStacklessHeldImages(code.data(),code.size(),ram.data(),ram.size(),io));
    assert(owner.CaptureFlashAll(flash.data(),flash.size(),io));
    assert(owner.StageFlashAll(image,1,io));
    assert(owner.StageHeldImages(reader,1,writer,1,rs,1,ws,1,io));
    assert(!owner.BeginReleaseAttempt(io));
    assert(owner.RestoreHeldImagesBeforeRelease(io) && owner.VerifyAfterResidentRestore(io));
    assert(context.io.SelectRuntimeCore((context.io.runtimeCore+1)%DataflowBarrierWorkers));
    for (unsigned core=0;core<DataflowBarrierWorkers;core++) assert(gRoleTileWindows[core]==originalRole[core]);
    assert(flashPlatform.words==originalFlash);
    guard.retire=true;assert(flashView.Close(guard));
    assert(views.Close());gPerCoreRoleWindows=false;gAllowWrite=false;assert(release_sem(sLock)==B_OK && !gLocked);
    std::puts("PASS actual role/flash PCI adapters and held fleet capture, stage and restore; SIMULATED ONLY");
}
#endif
