/* SPDX-License-Identifier: MIT */
// Compiles the actual Haiku binding with simulated kernel/session facilities.
// Physical release remains denied; all mappings and card access are mocked.
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
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
#include "PioneerQwen35ExclusiveRoleImages.h"
#endif
#include "PioneerQwen35DataflowServiceImage.h"
#define main qwen35_haiku_scope_test_main
#include "pioneer_qwen35_dataflow_role_haiku_scope_test.cpp"
#undef main
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35DataflowSharedRoleBinding.inc"

#ifdef TEST_SHARED_POST_RESET
struct PioneerQ35SharedRoleBindingTestAccess {
    struct Guard { bool ProvePinnedBar(uint64_t,uint64_t) { return true; } };
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
    struct FailedFirstWrite {
        bool AllRolesHeld(unsigned) { return true; }
        bool DefaultBootRouting(unsigned) { return true; }
        bool Write(unsigned,unsigned,const uint32_t*,unsigned) { return false; }
        bool WriteShim(unsigned,unsigned,const uint32_t*,unsigned) { return false; }
        bool WriteBoot(unsigned,unsigned,uint32_t) { return false; }
        bool Read(unsigned,unsigned,uint32_t*,unsigned) { return false; }
        bool ReadShim(unsigned,unsigned,uint32_t*,unsigned) { return false; }
        bool ReadBoot(unsigned,unsigned,uint32_t&) { return false; }
    };
    static bool StageWithFailedWrite(PioneerQwen35SharedRoleBinding& binding) {
        FailedFirstWrite io;
        assert(!binding.fTakeover.Stage(binding.RoleImage(),io));
        return binding.fTakeover.ResetRequired();
    }
    static void AllowSimulatedRelease(PioneerQwen35RoleBorrowedScopeWitness& scope) {
        scope.fTestDedicatedReleaseAllowed=true;
    }
    static bool Attempted(const PioneerQwen35SharedRoleBinding& binding,unsigned core) {
        return binding.fReleaseAttempted[core];
    }
    static bool Confirmed(const PioneerQwen35SharedRoleBinding& binding,unsigned core) {
        return binding.fReleaseConfirmed[core];
    }
    static bool AuditHeldForStop(PioneerQwen35SharedRoleBinding& binding,unsigned core) {
        auto* scope=binding.Scope();
        if (!scope) return false;
        PioneerQwen35SharedRoleBinding::Adapter io(binding.fContext.io,*scope,binding.fViews);
        return io.AllRolesHeldForStop(core);
    }
#endif
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
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
    assert(PioneerQ35SharedRoleBindingTestAccess::StageWithFailedWrite(*binding));
    assert(!binding->RestoreRolesHeld() && !binding->VerifyAfterResidentRestore()
        && !binding->FinalizeBeforeResume());
#endif
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
    io.runtimeCore=7; io.simulateArc=true; io.simulateHdma=true; io.firmwareMatches=true;
    io.selectCalls=io.tlbWrites=io.hdmaReads=0;
    auto* scope=io.qwen35BorrowedRoleScope;
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
    assert(!scope->ProveRoleAuditedDescriptorContinuity(io,7));
#endif
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
    PioneerQwen35RoleRouteGuard<PioneerSessionIO,PioneerQwen35RoleBorrowedScopeWitness> route(io,*scope);
    bool restoreFailed=false;
    io.simulateHdma=false;
    assert(!PioneerQwen35SampleHostDma(io,*scope,route,restoreFailed) && !restoreFailed);
    assert(!io.selectCalls && !io.tlbWrites);
    io.simulateHdma=true; io.hdmaSourceStable=false;
    assert(!PioneerQwen35SampleHostDma(io,*scope,route,restoreFailed) && !restoreFailed);
    assert(io.runtimeCore==7 && !io.tlbWrites && !io.hdmaReads);
    io.hdmaSourceStable=true;
    for (unsigned word=0;word<4;word++) {
        io.hdmaWords[word]=1;
        assert(!PioneerQwen35SampleHostDma(io,*scope,route,restoreFailed) && !restoreFailed);
        assert(io.runtimeCore==7 && route.ProveSelectedRoute(7));
        io.hdmaWords[word]=0;
    }
    io.hdmaReads=0; io.hdmaChangeAt=4;
    assert(!PioneerQwen35SampleHostDma(io,*scope,route,restoreFailed) && !restoreFailed);
    assert(io.runtimeCore==7 && route.ProveSelectedRoute(7));
    io.hdmaChangeAt=UINT32_MAX; io.hdmaReads=0; io.dropHdmaRestore=true;
    assert(!PioneerQwen35SampleHostDma(io,*scope,route,restoreFailed) && restoreFailed);
    io.dropHdmaRestore=false;
    assert(io.SelectRuntimeCore(7) && route.ProveSelectedRoute(7));
    io.selectCalls=io.tlbWrites=io.hdmaReads=0;
    // The dedicated compile witness can stage under observed checks; the
    // hardware release and recovery contracts remain separately denied.
#if defined(PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS) && defined(TEST_SHARED_POST_RESET)
    gAllowWrite=true;gPerCoreRoleWindows=true;gRoleWindowCore=gRoleSelectedCore=io.runtimeCore;
    assert(io.SharedCaptureAndStageHeld());
    assert(io.tlbWrites==12 && io.hdmaReads==12 && gMapCalls>0);
    assert(context->qwen35SharedRoleBinding->TakeoverResetRequired());
    // Flush the last selected mock view and check every distinct tile, rather
    // than letting one shared backing buffer stand in for the whole fleet.
    assert(io.SelectRuntimeCore((io.runtimeCore+1)%DataflowBarrierWorkers));
    uint32_t readerBoot=0,writerBoot=0;
    assert(DataflowRoleBootWord(DataflowRoleReader,DataflowRoleReaderShimAddress,readerBoot)
        && DataflowRoleBootWord(DataflowRoleWriter,DataflowRoleWriterShimAddress,writerBoot));
    for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
        const auto& windows=gRoleTileWindows[core];
        assert(windows[DataflowRolePhysicalBriscBoot][0]==readerBoot
            && windows[DataflowRolePhysicalNcriscBoot][0]==writerBoot);
        const auto& shim=windows[DataflowRolePhysicalShim];
        const auto& code=windows[DataflowRolePhysicalExtraCode];
        assert(std::equal(std::begin(DataflowExclusiveReaderShimWords),std::end(DataflowExclusiveReaderShimWords),
            shim.begin()+(DataflowRoleReaderShimAddress-DataflowRoleReaderShimAddress)/4));
        assert(std::equal(std::begin(DataflowExclusiveWriterShimWords),std::end(DataflowExclusiveWriterShimWords),
            shim.begin()+(DataflowRoleWriterShimAddress-DataflowRoleReaderShimAddress)/4));
        assert(std::equal(std::begin(DataflowSharedReaderCodeWords),std::end(DataflowSharedReaderCodeWords),
            code.begin()+(DataflowRoleReaderCodeAddress-DataflowRoleExtraL1Address)/4));
        assert(std::equal(std::begin(DataflowSharedWriterCodeWords),std::end(DataflowSharedWriterCodeWords),
            code.begin()+(DataflowRoleWriterCodeAddress-DataflowRoleExtraL1Address)/4));
        assert(code.back()==RoleWindowInitial(core,DataflowRolePhysicalExtraCode)
            && shim.back()==RoleWindowInitial(core,DataflowRolePhysicalShim));
    }
    gPerCoreRoleWindows=false;
    for (auto& windows:gRoleTileWindows) windows.clear();
    assert(scope->ProveRoleDedicatedObservedStage(io));
    const unsigned selected=io.runtimeCore;
    assert(scope->ProveRoleMemoryReadSafe(io,selected,DataflowRolePhysicalExtraCode)
        && scope->ProveRoleMemoryWriteSafe(io,selected,DataflowRolePhysicalExtraCode,0,4));
    assert(!scope->ProveRoleMemoryReadSafe(io,selected,DataflowRolePhysicalBriscRam)
        && !scope->ProveRoleMemoryWriteSafe(io,selected,DataflowRolePhysicalBriscRam,0,4)
        && !scope->ProveRoleMemoryWriteSafe(io,selected,DataflowRolePhysicalExtraCode,3,4)
        && !scope->ProveRoleMemoryReadSafe(io,(selected+1)%DataflowBarrierWorkers,DataflowRolePhysicalExtraCode)
        && !scope->ProveRoleFleetHeldAndQuiet(io,selected));
    io.firmwareMatches=false;assert(!scope->ProveRoleDedicatedObservedStage(io)
        && !scope->ProveRoleMemoryWriteSafe(io,selected,DataflowRolePhysicalExtraCode,0,4));
    io.firmwareMatches=true;io.reset=0x6000;
    assert(!scope->ProveRoleMemoryReadSafe(io,selected,DataflowRolePhysicalExtraCode));
    io.reset=0x47800;
    io.nocConfig[0]=1U<<12;io.resetReads=0;
    assert(!scope->ProveRoleMemoryReadSafe(io,selected,DataflowRolePhysicalExtraCode)
        && !io.resetReads);
    io.nocConfig[0]=0;
    assert(!io.SharedArmAndBeginRelease() && !io.SharedStartCore(0));
    assert(!io.sharedResetWrites && !io.holdWrites);
    PioneerDriverRecoveryProof proof{};
    proof.flags=PIONEER_RECOVERY_PROOF_REQUIRED & ~PIONEER_RECOVERY_PROOF_RETAINED_OWNERS_RELEASED;
    proof.core_count=DataflowBarrierWorkers;proof.retained_owners=1;
    proof.reset_generation=proof.firmware_generation=9;
    io.postResetDisposeComplete=true;
    assert(PioneerQ35ReleaseSharedRoleAfterReset(context->qwen35SharedRoleBinding,proof,9));
    assert(!context->qwen35SharedRoleBinding);
    io.postResetDisposeComplete=false;gAllowWrite=false;
#else
    // The snapshot-based scope still denies the independent fleet proof.
    assert(!io.SharedCaptureAndStageHeld());
    assert(io.tlbWrites==12 && io.selectCalls==243 && io.hdmaReads==12 && io.runtimeCore==7);
    assert(!gMapCalls && !io.sharedResetWrites && !io.holdWrites);
    assert(!io.SharedArmAndBeginRelease() && !io.SharedStartCore(0));
    assert(PioneerQ35DestroySharedRoleBinding(context->qwen35SharedRoleBinding));
    context->qwen35SharedRoleBinding=nullptr;
#endif
    io.nocConfig[0]=io.nocConfig[1]=io.nocAvailable[0]=io.nocAvailable[1]=0;
    io.simulateArc=false; io.simulateHdma=false; io.firmwareMatches=false;
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

#if defined(PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS) && defined(TEST_SHARED_POST_RESET)
static void ExerciseObservedStageResetDrift(PioneerSessionIO& io)
{
    auto* context=io.qwen35DataflowContext;
    context->qwen35SharedRoleBinding=PioneerQ35CreateSharedRoleBinding(*context);
    assert(context->qwen35SharedRoleBinding && io.qwen35BorrowedRoleScope);
    io.runtimeCore=7;io.simulateArc=io.simulateHdma=io.firmwareMatches=true;
    io.hdmaSourceStable=true;io.reset=0x47800;io.resetReads=0;io.resetChangeAt=10000;
    gAllowWrite=true;
    assert(!io.SharedCaptureAndStageHeld() && gMapCalls>0
        && context->qwen35SharedRoleBinding->TakeoverResetRequired());
    assert(gMemory[DataflowRolePhysicalExtraCode][0]!=0x76540000U+DataflowRolePhysicalExtraCode);
    assert(!io.SharedCaptureAndStageHeld() && !io.SharedArmAndBeginRelease()
        && !io.sharedResetWrites && !io.holdWrites);
    io.resetChangeAt=-1;
    PioneerDriverRecoveryProof proof{};
    proof.flags=PIONEER_RECOVERY_PROOF_REQUIRED & ~PIONEER_RECOVERY_PROOF_RETAINED_OWNERS_RELEASED;
    proof.core_count=DataflowBarrierWorkers;proof.retained_owners=1;
    proof.reset_generation=proof.firmware_generation=9;io.postResetDisposeComplete=true;
    assert(PioneerQ35ReleaseSharedRoleAfterReset(context->qwen35SharedRoleBinding,proof,9));
    io.postResetDisposeComplete=false;io.simulateArc=io.simulateHdma=io.firmwareMatches=false;
    gAllowWrite=false;
}

static void ExerciseObservedStageMisroute(PioneerSessionIO& io)
{
    auto* context=io.qwen35DataflowContext;
    context->qwen35SharedRoleBinding=PioneerQ35CreateSharedRoleBinding(*context);
    assert(context->qwen35SharedRoleBinding && io.qwen35BorrowedRoleScope);
    io.runtimeCore=7;io.simulateArc=io.simulateHdma=io.firmwareMatches=true;
    io.hdmaSourceStable=true;io.reset=0x47800;io.resetReads=0;
    gAllowWrite=true;gPerCoreRoleWindows=true;gRoleWindowCore=gRoleSelectedCore=io.runtimeCore;
    // Two selects during the held-fleet check and two before the first code
    // chunk precede its write. Corrupt that write's BAR translation only.
    gRoleMisrouteCountdown=4;gRoleMisrouteInjected=false;
    assert(!io.SharedCaptureAndStageHeld() && gRoleMisrouteInjected
        && context->qwen35SharedRoleBinding->TakeoverResetRequired());
    assert(io.SelectRuntimeCore((io.runtimeCore+1)%DataflowBarrierWorkers));
    assert(gRoleTileWindows[0][DataflowRolePhysicalExtraCode][0]
        ==RoleWindowInitial(0,DataflowRolePhysicalExtraCode));
    for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
        assert(gRoleTileWindows[core][DataflowRolePhysicalBriscBoot][0]
            ==RoleWindowInitial(core,DataflowRolePhysicalBriscBoot));
        assert(gRoleTileWindows[core][DataflowRolePhysicalNcriscBoot][0]
            ==RoleWindowInitial(core,DataflowRolePhysicalNcriscBoot));
    }
    assert(!io.SharedArmAndBeginRelease() && !io.SharedStartCore(0)
        && !io.sharedResetWrites && !io.holdWrites);
    gPerCoreRoleWindows=false;gRoleMisrouteCountdown=-1;
    for (auto& windows:gRoleTileWindows) windows.clear();
    PioneerDriverRecoveryProof proof{};
    proof.flags=PIONEER_RECOVERY_PROOF_REQUIRED & ~PIONEER_RECOVERY_PROOF_RETAINED_OWNERS_RELEASED;
    proof.core_count=DataflowBarrierWorkers;proof.retained_owners=1;
    proof.reset_generation=proof.firmware_generation=9;io.postResetDisposeComplete=true;
    assert(PioneerQ35ReleaseSharedRoleAfterReset(context->qwen35SharedRoleBinding,proof,9));
    io.postResetDisposeComplete=false;io.simulateArc=io.simulateHdma=io.firmwareMatches=false;
    gAllowWrite=false;
}

// The injected authorization is only a mock proof, but the binding, role
// image staging, release ledger and PCI adapter are the real implementation.
static void ExerciseReleaseBinding(PioneerSessionIO& io,unsigned failCore)
{
    auto* context=io.qwen35DataflowContext;
    context->qwen35SharedRoleBinding=PioneerQ35CreateSharedRoleBinding(*context);
    auto* binding=context->qwen35SharedRoleBinding;
    assert(binding && io.qwen35BorrowedRoleScope);
    io.runtimeCore=7;io.simulateArc=io.simulateHdma=io.firmwareMatches=true;
    io.hdmaSourceStable=true;io.reset=0x47800;io.resetReads=0;io.sharedResetWrites=0;
    std::fill(std::begin(io.tileReset),std::end(io.tileReset),0x47800);
    io.perCoreReset=true;io.dropReleaseCore=failCore;
    gAllowWrite=true;gPerCoreRoleWindows=true;gRoleWindowCore=gRoleSelectedCore=io.runtimeCore;
    assert(io.SharedCaptureAndStageHeld());
    assert(!io.SharedArmAndBeginRelease()); // real proof remains unavailable
    PioneerQ35SharedRoleBindingTestAccess::AllowSimulatedRelease(*io.qwen35BorrowedRoleScope);
    assert(io.SharedArmAndBeginRelease());
    assert(!io.SharedStartCore(1) && !PioneerQ35SharedRoleBindingTestAccess::Attempted(*binding,1));
    const unsigned successful=failCore<DataflowBarrierWorkers?failCore:DataflowBarrierWorkers;
    for (unsigned core=0;core<successful;core++) {
        assert(io.SharedStartCore(core));
        assert(PioneerQ35SharedRoleBindingTestAccess::Attempted(*binding,core)
            && PioneerQ35SharedRoleBindingTestAccess::Confirmed(*binding,core));
    }
    if (failCore<DataflowBarrierWorkers) {
        assert(!io.SharedStartCore(failCore)
            && PioneerQ35SharedRoleBindingTestAccess::Attempted(*binding,failCore)
            && !PioneerQ35SharedRoleBindingTestAccess::Confirmed(*binding,failCore));
        assert(!io.SharedStartCore(failCore) && !io.SharedStartCore(failCore+1));
        assert(io.sharedResetWrites==failCore+1);
    } else {
        assert(io.sharedResetWrites==DataflowBarrierWorkers && !io.SharedStartCore(0));
    }
    assert(io.SelectRuntimeCore((io.runtimeCore+1)%DataflowBarrierWorkers));
    for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
        assert(io.tileReset[core]==(core<successful?0x6000U:0x47800U));
        if (core>failCore && failCore<DataflowBarrierWorkers)
            assert(!PioneerQ35SharedRoleBindingTestAccess::Attempted(*binding,core));
    }
    assert(!io.qwen35BorrowedRoleScope->ProveRoleDedicatedObservedStage(io));
    // Close is no longer in the capture phase. A held tile can still be
    // inspected through the actual scope; a running tile or changed firmware
    // cannot satisfy the held audit.
    context->qwen35DataflowService->capturePending=false;
    assert(!PioneerQ35SharedRoleBindingTestAccess::AuditHeldForStop(*binding,0));
    if (failCore<DataflowBarrierWorkers) {
        assert(PioneerQ35SharedRoleBindingTestAccess::AuditHeldForStop(*binding,failCore));
        io.firmwareMatches=false;
        assert(!PioneerQ35SharedRoleBindingTestAccess::AuditHeldForStop(*binding,failCore));
        io.firmwareMatches=true;
    }
    context->qwen35DataflowService->capturePending=true;
    unsigned stage=0,core=0;
    assert(!io.SharedStopAndHoldAll(stage,core));
    assert(stage==(failCore<DataflowBarrierWorkers?15U:8U));
    assert(binding->TakeoverResetRequired());
    gPerCoreRoleWindows=false;for (auto& windows:gRoleTileWindows) windows.clear();
    io.perCoreReset=false;io.dropReleaseCore=UINT32_MAX;io.reset=0x47800;
    PioneerDriverRecoveryProof proof{};
    proof.flags=PIONEER_RECOVERY_PROOF_REQUIRED & ~PIONEER_RECOVERY_PROOF_RETAINED_OWNERS_RELEASED;
    proof.core_count=DataflowBarrierWorkers;proof.retained_owners=1;
    proof.reset_generation=proof.firmware_generation=9;io.postResetDisposeComplete=true;
    assert(PioneerQ35ReleaseSharedRoleAfterReset(context->qwen35SharedRoleBinding,proof,9));
    io.postResetDisposeComplete=false;io.simulateArc=io.simulateHdma=io.firmwareMatches=false;
    io.sharedResetWrites=0;gAllowWrite=false;
}
static void ExerciseCompleteReleaseBinding(PioneerSessionIO& io) {
    ExerciseReleaseBinding(io,DataflowBarrierWorkers);
}
static void ExercisePartialReleaseBinding(PioneerSessionIO& io) {
    ExerciseReleaseBinding(io,17);
}
#endif

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
    PioneerProbeCookie cookie; cookie.context=&context; sRuntimeOwner=&cookie; sQwen35ExclusiveCookie=&cookie;
    ResetMaps(); gBorrowedScopeHook=ExerciseBinding;
    assert(Qwen35DataflowControl(&cookie,5,nullptr,0)==B_OK);
    assert(Qwen35DataflowGenerate(&cookie,nullptr,0)==B_OK);
    assert(Qwen35DataflowControl(&cookie,6,nullptr,0)==B_OK);
    assert(Qwen35DataflowControl(&cookie,7,nullptr,0)==B_OK);
    service.capturePending=true; gBorrowedScopeHook=ExerciseUnexpectedFirmware;
    assert(Qwen35DataflowControl(&cookie,5,nullptr,0)==B_OK);
    gBorrowedScopeHook=ExerciseStatusSampling;
    assert(Qwen35DataflowControl(&cookie,5,nullptr,0)==B_OK);
#if defined(PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS) && defined(TEST_SHARED_POST_RESET)
    ResetMaps();gBorrowedScopeHook=ExerciseObservedStageResetDrift;
    assert(Qwen35DataflowControl(&cookie,5,nullptr,0)==B_OK);
    ResetMaps();gBorrowedScopeHook=ExerciseObservedStageMisroute;
    assert(Qwen35DataflowControl(&cookie,5,nullptr,0)==B_OK);
    ResetMaps();gBorrowedScopeHook=ExerciseCompleteReleaseBinding;
    assert(Qwen35DataflowControl(&cookie,5,nullptr,0)==B_OK);
    ResetMaps();gBorrowedScopeHook=ExercisePartialReleaseBinding;
    assert(Qwen35DataflowControl(&cookie,5,nullptr,0)==B_OK);
#endif
#ifdef TEST_SHARED_POST_RESET
    gBorrowedScopeHook=ExerciseResetRetirement;
    assert(Qwen35DataflowControl(&cookie,5,nullptr,0)==B_OK);
#endif
    service.capturePending=false;
    assert(!gLocked && !context.qwen35SharedRoleBinding && !context.io.qwen35BorrowedRoleScope);
    gBorrowedScopeHook=nullptr;
#if defined(PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS) && defined(TEST_SHARED_POST_RESET)
    std::puts("PASS dedicated per-tile staging and real release ledger with mock proof; SIMULATED ONLY");
#else
    std::puts("PASS actual shared role binding denies mapping, reset and release without fleet proof; SIMULATED ONLY");
#endif
}
