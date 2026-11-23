/* SPDX-License-Identifier: MIT */
// Compiles the actual Haiku binding with simulated kernel/session facilities.
// All physical proofs remain denied and no mapping or card access is made.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS 1
#ifdef TEST_SHARED_POST_RESET
#define PIONEER_QWEN35_RECOVERY_RUN_ENABLE 1
#define PIONEER_QWEN35_SHARED_ROLE_OWNER_TEST 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ35GraphRecoveryProof.h"
#endif
#include <cstdlib>
#include <new>
#include "../PioneerQwen35DataflowOwner.h"
#include "PioneerQwen35SharedRoleImages.h"
#include "PioneerQwen35DataflowServiceImage.h"
#define main qwen35_haiku_scope_test_main
#include "pioneer_qwen35_dataflow_role_haiku_scope_test.cpp"
#undef main
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35DataflowSharedRoleBinding.inc"

#ifdef TEST_SHARED_POST_RESET
struct PioneerQ35SharedRoleBindingTestAccess {
    struct Guard { bool ProvePinnedBar(uint64_t,uint64_t) { return true; } };
    static bool OpenViews(PioneerQwen35SharedRoleBinding& binding) {
        Guard guard;
        return binding.fViews.Open(binding.fContext.io.profile.bar0_host_start,
            binding.fContext.io.profile.bar0_host_end,guard);
    }
};
static void ExerciseResetRetirement(PioneerSessionIO& io)
{
    auto* context=io.qwen35DataflowContext;
    context->qwen35SharedRoleBinding=PioneerQ35CreateSharedRoleBinding(*context);
    auto*& binding=context->qwen35SharedRoleBinding;
    assert(binding && gLocked);
    ResetMaps();gAllowWrite=true;
    assert(PioneerQ35SharedRoleBindingTestAccess::OpenViews(*binding));
    assert(!binding->CanDestroy());
    PioneerDriverRecoveryProof proof{};
    proof.flags=PIONEER_RECOVERY_PROOF_REQUIRED & ~PIONEER_RECOVERY_PROOF_RETAINED_OWNERS_RELEASED;
    proof.core_count=DataflowBarrierWorkers;proof.retained_owners=1;
    proof.reset_generation=proof.firmware_generation=9;
    assert(!PioneerQ35ReleaseSharedRoleAfterReset(binding,proof,9) && binding && !gUnmapCalls);
    io.postResetDisposeComplete=true;
    context->recoveryMapping=reinterpret_cast<void*>(1);
    assert(!PioneerQ35ReleaseSharedRoleAfterReset(binding,proof,9) && binding && !gUnmapCalls);
    context->recoveryMapping=nullptr;
    assert(!PioneerQ35ReleaseSharedRoleAfterReset(binding,proof,8) && binding && !gUnmapCalls);
    auto bad=proof;bad.flags=0;
    assert(!PioneerQ35ReleaseSharedRoleAfterReset(binding,bad,9) && binding && !gUnmapCalls);
    gFailUnmap=2;
    assert(!PioneerQ35ReleaseSharedRoleAfterReset(binding,proof,9) && binding && gMapped[2]);
    assert(!binding->CanDestroy() && !io.SharedStartCore(0));
    assert(PioneerQ35ReleaseSharedRoleAfterReset(binding,proof,9) && !binding);
    for (bool mapped:gMapped) assert(!mapped);
    io.postResetDisposeComplete=false;gAllowWrite=false;
}
#endif

static void ExerciseBinding(PioneerSessionIO& io)
{
    auto* context=io.qwen35DataflowContext;
    assert(context && io.qwen35BorrowedRoleScope);
    context->qwen35SharedRoleBinding=PioneerQ35CreateSharedRoleBinding(*context);
    assert(context->qwen35SharedRoleBinding && gLocked);
    io.runtimeCore=7;
    assert(!context->qwen35SharedRoleBinding->StacklessCopiesCaptured(0));
    assert(!io.qwen35BorrowedRoleScope->ProveRoleLocalRamStable(io,7));
    assert(!io.SharedCaptureAndStageHeld());
    assert(!io.SharedCaptureAndStageHeld()); // one-shot, even after a denied proof
    assert(!io.SharedArmAndBeginRelease() && !io.SharedFleetHeldAndQuiescent());
    assert(!io.SharedStartCore(0) && !io.SharedPrepareRoleStacks(0));
    assert(!io.SharedStartCore(1));
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
    io.runtimeCore=7; io.simulateArc=true; io.firmwareMatches=true; io.selectCalls=io.tlbWrites=0;
    auto* scope=io.qwen35BorrowedRoleScope;
    assert(scope->ProveRoleImageInspectionAllowed(io,7));
    assert(!scope->ProveRoleImageInspectionAllowed(io,8));
    io.firmwareMatches=false;
    assert(!scope->ProveRoleImageInspectionAllowed(io,7));
    io.firmwareMatches=true;
    io.mapping=false;
    assert(!scope->ProveRoleImageInspectionAllowed(io,7));
    io.mapping=true;
    context->qwen35DataflowService->capturePending=false;
    assert(!scope->ProveRoleImageInspectionAllowed(io,7));
    context->qwen35DataflowService->capturePending=true;
    unsigned sensitive=0;
    for (unsigned noc=0;noc<2;noc++) {
        io.nocConfig[noc]=1U<<12; io.resetReads=0;
        assert(!PioneerQwen35WithSelectedClock(io,7,[&]() { ++sensitive; return io.ResetRead()!=0; }));
        assert(!sensitive && !io.resetReads);
        io.nocConfig[noc]=0;
    }
    assert(PioneerQwen35WithSelectedClock(io,7,[&]() { ++sensitive; return io.ResetRead()==0x47800; }));
    assert(sensitive==1);
    for (unsigned noc=0;noc<2;noc++) {
        io.nocConfig[noc]=1U<<16; io.nocAvailable[noc]=0x10101010U;
        assert(BlackholeResidentReadRegister(noc*0x10000+0x64));
        assert(BlackholeResidentReadRegister(noc*0x10000+0x68));
        assert(!BlackholeResidentReadRegister(noc*0x10000+0x60));
        assert(!BlackholeResidentWriteRegister(noc*0x10000+0x64));
        assert(!BlackholeResidentWriteRegister(noc*0x10000+0x68));
    }
    // ARC inspection and both 120-core sweeps can finish, but they cannot
    // authorize the independent fleet ownership proof or role mapping.
    assert(!io.SharedCaptureAndStageHeld());
    assert(io.tlbWrites==6 && io.selectCalls==241 && io.runtimeCore==7);
    assert(!gMapCalls && !io.sharedResetWrites && !io.holdWrites);
    assert(!io.SharedArmAndBeginRelease() && !io.SharedStartCore(0));
    assert(PioneerQ35DestroySharedRoleBinding(context->qwen35SharedRoleBinding));
    context->qwen35SharedRoleBinding=nullptr;
    io.nocConfig[0]=io.nocConfig[1]=io.nocAvailable[0]=io.nocAvailable[1]=0;
    io.simulateArc=false; io.firmwareMatches=false;
}

static void ExerciseUnexpectedFirmware(PioneerSessionIO& io)
{
    auto* context=io.qwen35DataflowContext;
    assert(context && io.qwen35BorrowedRoleScope);
    context->qwen35SharedRoleBinding=PioneerQ35CreateSharedRoleBinding(*context);
    assert(context->qwen35SharedRoleBinding);
    io.runtimeCore=7; io.simulateArc=true; io.firmwareMatches=false;
    io.selectCalls=io.tlbWrites=0;
    assert(!io.SharedCaptureAndStageHeld());
    assert(!io.tlbWrites && !io.selectCalls && !gMapCalls && io.runtimeCore==7);
    assert(PioneerQ35DestroySharedRoleBinding(context->qwen35SharedRoleBinding));
    context->qwen35SharedRoleBinding=nullptr; io.simulateArc=false;
}

int main()
{
    DataflowRoleProgramBounds exported={
        {sizeof(DataflowSharedReaderShimWords)/sizeof(uint32_t),sizeof(DataflowSharedWriterShimWords)/sizeof(uint32_t)},
        {sizeof(DataflowSharedReaderCodeWords)/sizeof(uint32_t),sizeof(DataflowSharedWriterCodeWords)/sizeof(uint32_t)},
        {DataflowSharedReaderParkAddress,DataflowSharedWriterParkAddress}};
    assert(exported.Valid());
    assert(exported.ContainsRunning(DataflowRoleReader,DataflowRoleReaderCodeAddress));
    assert(exported.ContainsRunning(DataflowRoleWriter,DataflowRoleWriterCodeAddress));
    assert(!exported.ContainsRunning(DataflowRoleReader,DataflowSharedReaderParkAddress));
    assert(!exported.ContainsRunning(DataflowRoleWriter,DataflowSharedWriterParkAddress));
    PioneerStoreContext context; FakeService service; FakeGraph graph;
    context.qwen35DataflowService=&service; context.qwen35GraphOwner=&graph;
    PioneerProbeCookie cookie; cookie.context=&context; sRuntimeOwner=&cookie;
    ResetMaps(); gBorrowedScopeHook=ExerciseBinding;
    assert(Qwen35DataflowControl(&cookie,5,nullptr,0)==B_OK);
    assert(Qwen35DataflowGenerate(&cookie,nullptr,0)==B_OK);
    assert(Qwen35DataflowControl(&cookie,6,nullptr,0)==B_OK);
    assert(Qwen35DataflowControl(&cookie,7,nullptr,0)==B_OK);
    service.capturePending=true; gBorrowedScopeHook=ExerciseUnexpectedFirmware;
    assert(Qwen35DataflowControl(&cookie,5,nullptr,0)==B_OK);
    gBorrowedScopeHook=ExerciseStatusSampling;
    assert(Qwen35DataflowControl(&cookie,5,nullptr,0)==B_OK);
#ifdef TEST_SHARED_POST_RESET
    gBorrowedScopeHook=ExerciseResetRetirement;
    assert(Qwen35DataflowControl(&cookie,5,nullptr,0)==B_OK);
#endif
    service.capturePending=false;
    assert(!gLocked && !context.qwen35SharedRoleBinding && !context.io.qwen35BorrowedRoleScope);
    gBorrowedScopeHook=nullptr;
    std::puts("PASS actual shared role binding denies mapping, reset and release without fleet proof; SIMULATED ONLY");
}
