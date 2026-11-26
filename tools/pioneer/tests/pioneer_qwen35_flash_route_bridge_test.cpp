/* SPDX-License-Identifier: MIT */
// Real role-session TLB/NoC route checks over fake device state only.
#define PIONEER_Q35_FLASH_ACTUAL_ROLE_FIXTURE_ONLY 1
#include "pioneer_qwen35_flash_actual_role_pci_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35DataflowFlashRouteGuard.h"
struct BridgeSelectionLedger {bool pending=false,uncertain=false;unsigned started=0,confirmed=0;};
struct BridgeAuthority : ActualRoleFlashGuard {
    BridgeSelectionLedger& selection;
    BridgeAuthority(PioneerSessionIO& session,BridgeSelectionLedger& state):ActualRoleFlashGuard(session),selection(state) {}
    bool ProvePinnedBar(uint64_t start,uint64_t end) const {
        return pinned && start==0x4900000000ULL && end==0x491fffffffULL;
    }
    bool ProveFlashMappingSafe(uint64_t address,unsigned bytes) const {
        return mapping && address==0x49190d0000ULL && bytes==DataflowFlashCodeBytes;
    }
    bool CanSelectFlashRoute(unsigned core) const {return core<DataflowBarrierWorkers && !selection.pending && !selection.uncertain;}
    bool BeginFlashRouteSelection(unsigned core) {
        if (!CanSelectFlashRoute(core)) return false;
        selection.pending=true;++selection.started;return true;
    }
    bool SelectFlashRoute(unsigned core) {return Guard::SelectFlashRoute(core);}
    bool ConfirmFlashRouteSelection(unsigned core) {
        if (!selection.pending || !Guard::ProveSelectedRoute(core)) return false;
        selection.pending=false;++selection.confirmed;return true;
    }
    void RetainUncertainFlashRouteSelection(unsigned) {selection.pending=false;selection.uncertain=true;}
};
using BridgeGuard=PioneerQwen35FlashRouteGuard<PioneerSessionIO,ActualRoleScope,BridgeAuthority>;
using BridgeScope=DataflowFlashRolePciScope<ActualRoleIO,DataflowFlashCodePciView<Platform>,BridgeGuard>;
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
    BridgeSelectionLedger selection;
    BridgeGuard guard(context.io,roleGuard,context.io,selection);Platform flashPlatform{guard};
    DataflowFlashCodePciView<Platform> flashView(flashPlatform);
    assert(flashView.Open(context.io.profile.bar0_host_start,context.io.profile.bar0_host_end,guard));
    DataflowFlashRoleRouteLedger ledger;guard.expectedLedger=&ledger;
    BridgeScope io(flashView,guard,ledger,context.io,roleGuard,views);
    const uint32_t input=0x23456789;uint32_t output=0;
    assert(io.FlashWriteAllowed(7,DataflowFlashCodeAddress,4));
    assert(io.Write(7,DataflowFlashCodeAddress,&input,4));
    assert(io.FlashReadAllowed(7,DataflowFlashCodeAddress,4));
    assert(io.Read(7,DataflowFlashCodeAddress,&output,4) && output==input);
    assert(selection.started==2 && selection.confirmed==2 && !selection.uncertain);
    assert(context.io.SelectRuntimeCore(8));
    const unsigned reads=flashPlatform.reads;
    assert(!io.Read(7,DataflowFlashCodeAddress,&output,4) && flashPlatform.reads==reads);
    context.io.failNextSelection=true;
    const unsigned before=context.io.selectCalls;
    assert(!io.FlashReadAllowed(9,DataflowFlashCodeAddress,4));
    assert(selection.uncertain && selection.started==3 && selection.confirmed==2
        && context.io.selectCalls==before+1);
    assert(!io.FlashReadAllowed(9,DataflowFlashCodeAddress,4) && context.io.selectCalls==before+1);
    BridgeGuard recreated(context.io,roleGuard,context.io,selection);
    assert(!recreated.SelectFlashRoute(9) && context.io.selectCalls==before+1);
    guard.retire=true;assert(flashView.Close(guard));
    assert(views.Close());gPerCoreRoleWindows=false;gAllowWrite=false;assert(release_sem(sLock)==B_OK && !gLocked);
    std::puts("PASS real role TLB/NoC checks and retained failed flash route selection; SIMULATED ONLY");
}
