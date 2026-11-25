/* SPDX-License-Identifier: MIT */
// Compose the real service owner and shared-role binding over simulated IO.
// The role qualification admission is a test stub and no device is accessed.
#define TEST_REAL_SERVICE_INTEGRATION 1
#define TEST_SHARED_POST_RESET 1
#define PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS 1
#include "pioneer_qwen35_dataflow_shared_role_binding_test.cpp"
#define main existing_service_owner_test_main
#include "pioneer_qwen35_dataflow_service_owner_test.cpp"
#undef main
#include <memory>

struct ComposedServiceIO : MockDataflowIO {
    PioneerSessionIO& roleIO;
    explicit ComposedServiceIO(PioneerSessionIO& io):roleIO(io) {
        // Qualification starts from a prepared fleet with the processors held.
        for (unsigned core=0;core<DataflowCoreCount;core++)
            originalRunning[core]=started[core]=false;
    }
    bool SharedCaptureAndStageHeld() { return roleIO.SharedCaptureAndStageHeld(); }
    bool SharedArmAndBeginRelease() { return roleIO.SharedArmAndBeginRelease(); }
    bool SharedStopAndHoldAll(unsigned& stage,unsigned& core) {
        return roleIO.SharedStopAndHoldAll(stage,core);
    }
    bool SharedRestoreRolesHeld() { return roleIO.SharedRestoreRolesHeld(); }
    bool SharedVerifyRolesAfterResidentRestore() { return roleIO.SharedVerifyRolesAfterResidentRestore(); }
    bool SharedFinalizeBeforeResume() { return roleIO.SharedFinalizeBeforeResume(); }
};

static void ExerciseFaultedStageService(PioneerSessionIO& io) {
    auto* context=io.qwen35DataflowContext;
    assert(context && context->qwen35DataflowService && io.qwen35BorrowedRoleScope);
    DataflowServiceOwner service;
    context->qwen35DataflowService->real=&service;
    ResetMaps();gAllowWrite=true;
    context->qwen35SharedRoleBinding=PioneerQ35CreateSharedRoleBinding(*context);
    auto* binding=context->qwen35SharedRoleBinding;assert(binding);
    assert(PioneerQ35SelectSharedRoleAutoRestore(binding));
    io.simulateArc=io.simulateHdma=io.firmwareMatches=true;
    assert(io.SelectRuntimeCore(119));
    gPerCoreRoleWindows=true;gRoleWindowCore=gRoleSelectedCore=io.runtimeCore;
    gQualificationHeldOwnerAllowed=true;
    auto serviceIO=std::make_unique<ComposedServiceIO>(io);
    uint32_t code[96]{},dispatch[80]{};
    DataflowServiceImageView image={DataflowServiceBootWord,code,96,dispatch,80};
    assert(service.Install(image,*serviceIO)==DataflowServiceStartFailed && service.Faulted());
    assert(serviceIO->startCount==0 && io.sharedResetWrites==0);
    gQualificationHeldOwnerAllowed=false;gQualificationHeldRollbackOwnerAllowed=true;
    assert(PioneerQ35CanAbortUnstagedQualification(binding));
    assert(service.Close(*serviceIO)==DataflowServiceClosed && service.Closed());
    assert(serviceIO->restoreCount==DataflowCoreCount && serviceIO->resumeCount==1
        && serviceIO->startCount==0 && io.sharedResetWrites==0 && !sColdRecoveryRequired);
    for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
        assert(serviceIO->restored[core] && serviceIO->held[core] && !serviceIO->started[core]);
        for (unsigned window=0;window<=DataflowRolePhysicalExtraCode;window++)
            assert(gRoleTileWindows[core][window][0]==RoleWindowInitial(core,window));
    }
    for (bool mapped:gMapped) assert(!mapped);
    assert(PioneerQ35DestroySharedRoleBinding(binding));
    context->qwen35SharedRoleBinding=nullptr;context->qwen35DataflowService->real=nullptr;
    gQualificationHeldRollbackOwnerAllowed=false;gPerCoreRoleWindows=false;
    for (auto& windows:gRoleTileWindows) windows.clear();
    io.simulateArc=io.simulateHdma=io.firmwareMatches=false;ResetMaps();
}

int main() {
    PioneerStoreContext context;FakeService service;FakeGraph graph;
    context.qwen35DataflowService=&service;context.qwen35GraphOwner=&graph;
    PioneerProbeCookie cookie;cookie.context=&context;sRuntimeOwner=&cookie;sQwen35ExclusiveCookie=&cookie;
    gBorrowedScopeHook=ExerciseFaultedStageService;
    assert(Qwen35DataflowControl(&cookie,5,nullptr,0)==B_OK);
    assert(!gLocked && !context.qwen35SharedRoleBinding && !context.io.qwen35BorrowedRoleScope);
    gBorrowedScopeHook=nullptr;
    std::puts("PASS real service and shared binding pre-release fault cleanup; SIMULATED ONLY");
}
