/* SPDX-License-Identifier: MIT */
// Compiles the actual Haiku binding with simulated kernel/session facilities.
// All physical proofs remain denied and no mapping or card access is made.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS 1
#include <cstdlib>
#include <new>
#include "../PioneerQwen35DataflowOwner.h"
#include "PioneerQwen35SharedRoleImages.h"
#include "PioneerQwen35DataflowServiceImage.h"
#define main qwen35_haiku_scope_test_main
#include "pioneer_qwen35_dataflow_role_haiku_scope_test.cpp"
#undef main
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35DataflowSharedRoleBinding.inc"

static void ExerciseBinding(PioneerSessionIO& io)
{
    auto* context=io.qwen35DataflowContext;
    assert(context && io.qwen35BorrowedRoleScope);
    context->qwen35SharedRoleBinding=PioneerQ35CreateSharedRoleBinding(*context);
    assert(context->qwen35SharedRoleBinding && gLocked);
    io.runtimeCore=7;
    assert(!io.SharedCaptureAndStageHeld());
    assert(!io.SharedCaptureAndStageHeld()); // one-shot, even after a denied proof
    assert(!io.SharedArmAndBeginRelease() && !io.SharedFleetHeldAndQuiescent());
    assert(!io.SharedStartCore(0));
    DataflowWorkerSnapshot snapshot{};
    assert(!io.SharedCoreReady(0,snapshot));
    assert(!gMapCalls && io.runtimeCore==7 && io.reset==0x47800);
    unsigned stage=99,core=99;
    assert(io.SharedStopAndHoldAll(stage,core) && stage==0 && core==DataflowBarrierWorkers);
    assert(io.SharedRestoreRolesHeld() && io.SharedVerifyRolesAfterResidentRestore());
    assert(io.SharedFinalizeBeforeResume());
    assert(PioneerQ35DestroySharedRoleBinding(context->qwen35SharedRoleBinding));
    context->qwen35SharedRoleBinding=nullptr;
}

static void ExerciseStatusSampling(PioneerSessionIO& io)
{
    auto* context=io.qwen35DataflowContext;
    assert(context && io.qwen35BorrowedRoleScope);
    context->qwen35SharedRoleBinding=PioneerQ35CreateSharedRoleBinding(*context);
    assert(context->qwen35SharedRoleBinding);
    io.runtimeCore=7; io.simulateArc=true; io.selectCalls=io.tlbWrites=0;
    // ARC inspection and both 120-core sweeps can finish, but they cannot
    // authorize the independent fleet ownership proof or role mapping.
    assert(!io.SharedCaptureAndStageHeld());
    assert(io.tlbWrites==6 && io.selectCalls==241 && io.runtimeCore==7);
    assert(!gMapCalls && !io.sharedResetWrites && !io.holdWrites);
    assert(!io.SharedArmAndBeginRelease() && !io.SharedStartCore(0));
    assert(PioneerQ35DestroySharedRoleBinding(context->qwen35SharedRoleBinding));
    context->qwen35SharedRoleBinding=nullptr;
    io.simulateArc=false;
}

int main()
{
    PioneerStoreContext context; FakeService service; FakeGraph graph;
    context.qwen35DataflowService=&service; context.qwen35GraphOwner=&graph;
    PioneerProbeCookie cookie; cookie.context=&context; sRuntimeOwner=&cookie;
    ResetMaps(); gBorrowedScopeHook=ExerciseBinding;
    assert(Qwen35DataflowControl(&cookie,5,nullptr,0)==B_OK);
    assert(Qwen35DataflowGenerate(&cookie,nullptr,0)==B_OK);
    assert(Qwen35DataflowControl(&cookie,6,nullptr,0)==B_OK);
    assert(Qwen35DataflowControl(&cookie,7,nullptr,0)==B_OK);
    service.capturePending=true; gBorrowedScopeHook=ExerciseStatusSampling;
    assert(Qwen35DataflowControl(&cookie,5,nullptr,0)==B_OK);
    service.capturePending=false;
    assert(!gLocked && !context.qwen35SharedRoleBinding && !context.io.qwen35BorrowedRoleScope);
    gBorrowedScopeHook=nullptr;
    std::puts("PASS actual shared role binding denies mapping, reset and release without fleet proof; SIMULATED ONLY");
}
