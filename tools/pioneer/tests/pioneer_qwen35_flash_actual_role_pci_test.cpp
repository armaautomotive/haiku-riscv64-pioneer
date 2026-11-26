/* SPDX-License-Identifier: MIT */
// Actual role PCI adapter and flash view over fake role/MMIO platforms.
#define PIONEER_Q35_FLEET_TEST_NO_MAIN 1
#define PIONEER_Q35_FLASH_PCI_FIXTURE_ONLY 1
#define PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS 1
#include "pioneer_qwen35_dataflow_role_fleet_pci_io_test.cpp"
#include "pioneer_qwen35_flash_code_pci_view_test.cpp"
#include "../PioneerQwen35DataflowFlashRolePciScope.h"
struct ActualRoleFlashGuard : Guard {
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
using ActualRoleIO=PioneerQwen35RoleFleetPciIO<PioneerSessionIO,FleetPciTestScope,
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
int main() {
    PioneerStoreContext context;FakeService service;FakeGraph graph;
    context.qwen35DataflowService=&service;context.qwen35GraphOwner=&graph;
    PioneerProbeCookie cookie;cookie.context=&context;sRuntimeOwner=&cookie;
    ResetMaps();gAllowWrite=true;assert(AcquireProbeLock(&cookie,true)==B_OK);
    FleetPciTestScope roleGuard;PioneerQwen35RoleHaikuMap rolePlatform;
    DataflowRoleWritableViews<PioneerQwen35RoleHaikuMap> views(rolePlatform);
    PioneerQwen35RoleRouteGuard<PioneerSessionIO,FleetPciTestScope> route(context.io,roleGuard);
    assert(views.Open(context.io.profile.bar0_host_start,context.io.profile.bar0_host_end,route));
    ActualRoleFlashGuard guard;Platform flashPlatform{guard};
    DataflowFlashCodePciView<Platform> flashView(flashPlatform);
    assert(flashView.Open(0x100000000ULL,0x11fffffffULL,guard));
    ActualScope io(flashView,guard,context.io,roleGuard,views);
    gMemory[DataflowRolePhysicalExtraCode][0]=0x1234abcd;
    flashPlatform.words[7*DataflowFlashCodeWords]=0x73ace024;
    uint32_t observed=0;
    assert(io.Read(7,DataflowRoleExtraL1Address,&observed,4) && observed==0x1234abcd);
    assert(io.FlashReadAllowed(7,DataflowFlashCodeAddress,4));
    assert(io.Read(7,DataflowFlashCodeAddress,&observed,4) && observed==0x73ace024);
    const unsigned reads=flashPlatform.reads;
    assert(!io.Read(7,DataflowFlashCodeLimit-4,&observed,8) && flashPlatform.reads==reads);
    guard.retire=true;assert(flashView.Close(guard));
    assert(views.Close());gAllowWrite=false;assert(release_sem(sLock)==B_OK && !gLocked);
    std::puts("PASS actual role PCI adapter and flash view routing; SIMULATED ONLY");
}
