/* SPDX-License-Identifier: MIT */
// Compiles the actual Haiku binding with simulated kernel/session facilities.
// Physical release remains denied; all mappings and card access are mocked.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS 1
#ifdef TEST_SHARED_FABRIC_VIEWS
#define PIONEER_QWEN35_FABRIC_GUARD_WITNESS 1
#ifndef TEST_SHARED_POST_RESET
#error Fabric view test requires the post-reset harness
#endif
#endif
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
#ifdef TEST_SHARED_FABRIC_VIEWS
struct pioneer_driver_recovery_guard_v2_info {};
struct PioneerQ35FabricHostScope {
    PioneerStoreContext* context=nullptr;pioneer_driver_recovery_guard_v2_info* guard=nullptr;
    uint64 ticket=0;bool allowed=true;
    bool OwnsAfterAuditRelease(const PioneerStoreContext& requested,
        const pioneer_driver_recovery_guard_v2_info* requestedGuard,uint64 requestedTicket) const {
        return allowed && context==&requested && guard==requestedGuard && ticket==requestedTicket;
    }
};
#define PIONEER_QWEN35_FABRIC_HOST_CONTROLLER 1
#endif
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35DataflowSharedRoleBinding.inc"

#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
// The real qualification scope is the only production friend of this method.
class PioneerQ35ProjectionQualificationScope {
public:
    static bool Inspect(PioneerQwen35RoleBorrowedScopeWitness& scope,PioneerSessionIO& io,bool& failed) {
        return scope.InspectQualificationHeldFleet(io,failed);
    }
};
#endif

#ifdef TEST_SHARED_POST_RESET
struct PioneerQ35SharedRoleBindingTestAccess {
    struct Guard { bool ProvePinnedBar(uint64_t,uint64_t) { return true; } };
#ifdef TEST_SHARED_FABRIC_VIEWS
    static const void* CodeSnapshot(const PioneerQwen35SharedRoleBinding& binding) { return binding.fCodeSnapshot; }
    static const void* RamSnapshot(const PioneerQwen35SharedRoleBinding& binding) { return binding.fRamSnapshot; }
#endif
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
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
    static DataflowRoleProgramImage Image(const PioneerQwen35SharedRoleBinding& binding) {
        return binding.RoleImage();
    }
    static bool CloseViews(PioneerQwen35SharedRoleBinding& binding) { return binding.fViews.Close(); }
#ifndef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
    static bool SelectCaptureRoot(PioneerQwen35SharedRoleBinding& binding) {
        auto* scope=binding.Scope();
        if (!scope) return false;
        PioneerQwen35RoleRouteGuard<PioneerSessionIO,PioneerQwen35RoleBorrowedScopeWitness>
            route(binding.fContext.io,*scope);
        return binding.SelectQualificationCaptureRoot(*scope,route);
    }
    static bool HeldImagesCaptured(const PioneerQwen35SharedRoleBinding& binding) {
        return binding.fOwner.StacklessHeldImagesCaptured();
    }
    static bool HeldPreservationPrepared(const PioneerQwen35SharedRoleBinding& binding) {
        return binding.fOwner.HeldPreservationRollbackReady();
    }
    static void MarkReleaseAttempt(PioneerQwen35SharedRoleBinding& binding,unsigned core,bool value) {
        binding.fReleaseAttempted[core]=value;
    }
#ifdef TEST_PRESERVATION_ROLLBACK
    static void CheckControlRollbackOnly(PioneerQwen35SharedRoleBinding& b) {
        auto* scope=b.Scope();auto& io=b.fContext.io;assert(scope && io.SelectRuntimeCore(0));
        const uint32_t current=gMemory[DataflowRolePhysicalControl][0x224/4];
        const uint32_t original=RoleWindowInitial(0,DataflowRolePhysicalControl);
        assert(!scope->ProveRoleControlWriteSafe(io,0,current,original));
        assert(b.BeginHeldImageMutation(3));
        // The owner is still Armed here. The actual restore changes it to
        // Faulted before accepting even the saved control value.
        assert(!scope->ProveRoleControlWriteSafe(io,0,current,original)
            && !scope->ProveRoleControlWriteSafe(io,0,current,original^2U));
        b.fHeldImageMutation=0;
        assert(!scope->ProveRoleControlWriteSafe(io,0,current,original));
    }
#endif
    static void ExerciseSnapshotHoldLedger(PioneerQwen35SharedRoleBinding& b) {
        constexpr unsigned core=7;
        assert(!PioneerQ35SharedRoleSnapshotHoldTransition(&b,core,false));
        b.fSnapshotFleetParked=true;b.fReleaseAttempted[core]=true;b.fReleaseConfirmed[core]=true;
        b.fStackReleaseConfirmed[core]=true;b.fStopAttempted[core]=true;b.fStopToken[core]=1;
        b.fAutoRestoreParked[core]=true;b.fComputeStopAttempted[core]=true;
        assert(PioneerQ35SharedRoleSnapshotHoldTransition(&b,core,false));
        assert(!PioneerQ35SharedRoleSnapshotHoldTransition(nullptr,core,false)
            && !PioneerQ35SharedRoleSnapshotHoldTransition(&b,DataflowBarrierWorkers,false));
        b.fAutoRestoreOnStop=false;
        assert(!PioneerQ35SharedRoleSnapshotHoldTransition(&b,core,false));b.fAutoRestoreOnStop=true;
        b.fContext.qwen35SharedRoleBinding=nullptr;
        assert(!PioneerQ35SharedRoleSnapshotHoldTransition(&b,core,false));b.fContext.qwen35SharedRoleBinding=&b;
        bool* required[]={&b.fSnapshotFleetParked,&b.fReleaseAttempted[core],&b.fReleaseConfirmed[core],
            &b.fStackReleaseConfirmed[core],&b.fStopAttempted[core],&b.fAutoRestoreParked[core],
            &b.fComputeStopAttempted[core]};
        for (auto* flag:required) {
            *flag=false;assert(!PioneerQ35SharedRoleSnapshotHoldTransition(&b,core,false));*flag=true;
        }
        b.fStopToken[core]=0;assert(!PioneerQ35SharedRoleSnapshotHoldTransition(&b,core,false));
        b.fStopToken[core]=1;b.fReleaseHeld[core]=true;
        assert(!PioneerQ35SharedRoleSnapshotHoldTransition(&b,core,false));b.fReleaseHeld[core]=false;
        b.fHoldAttempted[core]=true;
        assert(!PioneerQ35SharedRoleSnapshotHoldTransition(&b,core,false)
            && PioneerQ35SharedRoleSnapshotHoldTransition(&b,core,true));
        auto* scope=b.Scope();assert(scope && !scope->ProveRoleHeldAfterAttempt(b.fContext.io,core));
        b.fSnapshotFleetParked=false;b.fReleaseAttempted[core]=b.fReleaseConfirmed[core]=false;
        b.fStackReleaseConfirmed[core]=b.fStopAttempted[core]=b.fAutoRestoreParked[core]=false;
        b.fComputeStopAttempted[core]=b.fHoldAttempted[core]=false;b.fStopToken[core]=0;
    }
    static void ExerciseSnapshotRuntimeObservation(PioneerQwen35SharedRoleBinding& b) {
        constexpr unsigned core=7;
        auto* scope=b.Scope();assert(scope);
        const bool firmware=b.fContext.io.firmwareMatches;
        b.fContext.io.firmwareMatches=true;
        assert(scope->CanInspectSnapshotRuntime(b.fContext.io,core)
            && !scope->ProveRoleRuntimeSafe(b.fContext.io,core));
        assert(!b.RuntimeObservationActive(core));
        b.fRuntimeObservationCore=core;
        assert(b.RuntimeObservationActive(core) && !b.RuntimeObservationActive(core+1));
        b.fRuntimeObservationCore=DataflowBarrierWorkers;
        auto* service=b.fContext.qwen35DataflowService;
        const bool ready=service->ready,capture=service->capturePending,running=service->graphRunning;
        service->ready=service->capturePending=service->graphRunning=false;
        assert(!scope->CanInspectSnapshotRuntime(b.fContext.io,core));
        service->ready=ready;service->capturePending=capture;service->graphRunning=running;
        auto* graph=b.fContext.qwen35GraphOwner;graph->retained=true;
        assert(!scope->CanInspectSnapshotRuntime(b.fContext.io,core));graph->retained=false;
        b.fContext.io.firmwareMatches=false;
        assert(!scope->CanInspectSnapshotRuntime(b.fContext.io,core));
        b.fContext.io.firmwareMatches=firmware;
    }
    static void ExerciseProjectionInFlight(PioneerQwen35SharedRoleBinding& b) {
        auto* scope=b.Scope();assert(scope);
        auto& io=b.fContext.io;
        const unsigned oldCore=io.runtimeCore,oldReset=io.reset;
        const bool oldFirmware=io.firmwareMatches;
        auto* service=b.fContext.qwen35DataflowService;
        const bool oldReady=service->ready;
        assert(!PioneerQ35SharedRoleProjectionInFlightLedger(&b));
        b.fCaptureAttempted=true;b.fReleaseAuthorized=true;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            b.fReleaseAttempted[core]=b.fReleaseConfirmed[core]=true;
            b.fStackReleaseSubmitted[core]=b.fStackReleaseAttempted[core]=true;
            b.fStackReleaseConfirmed[core]=b.fRunningImagesObserved[core]=true;
        }
        assert(PioneerQ35SharedRoleProjectionInFlightLedger(&b));
        io.runtimeCore=0;io.reset=0x6000;io.firmwareMatches=true;service->ready=true;
        int scratch=1,program=2;
        assert(scope->ProveRoleProjectionInFlightSafe(io,&scratch,&program));
        // The owned workload can have NoC requests in flight. Admission does
        // not mistake its nonempty queues for a failed idle-fleet audit.
        io.nocDoorbell[0][0]=1;io.nocOutstanding[1][3]=1;
        assert(scope->ProveRoleProjectionInFlightSafe(io,&scratch,&program));
        io.nocDoorbell[0][0]=io.nocOutstanding[1][3]=0;
        assert(!scope->ProveRoleProjectionInFlightSafe(io,nullptr,&program)
            && !scope->ProveRoleProjectionInFlightSafe(io,&scratch,nullptr));
        io.reset=0x47800;assert(!scope->ProveRoleProjectionInFlightSafe(io,&scratch,&program));io.reset=0x6000;
        const unsigned disabledReads=io.resetReadsClockDisabled;
        io.nocConfig[1]=1U<<12;
        assert(!scope->ProveRoleProjectionInFlightSafe(io,&scratch,&program)
            && io.resetReadsClockDisabled==disabledReads);
        io.nocConfig[1]=0;
        io.slot0[0]=io.DramTlbRead(0,0)^1U;io.tlb0Override=true;
        assert(!scope->ProveRoleProjectionInFlightSafe(io,&scratch,&program));io.tlb0Override=false;
        io.firmwareMatches=false;assert(!scope->ProveRoleProjectionInFlightSafe(io,&scratch,&program));io.firmwareMatches=true;
        service->ready=false;assert(!scope->ProveRoleProjectionInFlightSafe(io,&scratch,&program));service->ready=true;
        service->graphRunning=true;assert(!scope->ProveRoleProjectionInFlightSafe(io,&scratch,&program));service->graphRunning=false;
        io.runtimeCore=1;assert(!scope->ProveRoleProjectionInFlightSafe(io,&scratch,&program));io.runtimeCore=0;
        io.mapping=false;assert(!scope->ProveRoleProjectionInFlightSafe(io,&scratch,&program));io.mapping=true;
        b.fCaptureAttempted=false;assert(!PioneerQ35SharedRoleProjectionInFlightLedger(&b));b.fCaptureAttempted=true;
        b.fSnapshotFleetParked=true;assert(!PioneerQ35SharedRoleProjectionInFlightLedger(&b));b.fSnapshotFleetParked=false;
        b.fAutoRestoreOnStop=false;assert(!PioneerQ35SharedRoleProjectionInFlightLedger(&b));b.fAutoRestoreOnStop=true;
        for (unsigned core: {0U,37U,119U}) {
            bool* required[]={&b.fReleaseAttempted[core],&b.fReleaseConfirmed[core],
                &b.fStackReleaseSubmitted[core],&b.fStackReleaseAttempted[core],
                &b.fStackReleaseConfirmed[core],&b.fRunningImagesObserved[core]};
            for (auto* flag:required) {
                *flag=false;assert(!PioneerQ35SharedRoleProjectionInFlightLedger(&b));*flag=true;
            }
            bool* prohibited[]={&b.fReleaseHeld[core],&b.fStackReleaseWriting[core],
                &b.fStackCancelAttempted[core],&b.fStopAttempted[core],&b.fComputeStopAttempted[core],
                &b.fHoldAttempted[core],&b.fAutoRestoreParked[core],&b.fAutoCancelParked[core],
                &b.fAutoRestoreRecorded[core],&b.fCoreRestoreReleaseWrites[core],&b.fCoreRestoreHoldWrites[core]};
            for (auto* flag:prohibited) {
                *flag=true;assert(!PioneerQ35SharedRoleProjectionInFlightLedger(&b));*flag=false;
            }
        }
        b.fContext.qwen35SharedRoleBinding=nullptr;
        assert(!PioneerQ35SharedRoleProjectionInFlightLedger(&b));b.fContext.qwen35SharedRoleBinding=&b;
        b.fReleaseAuthorized=false;assert(!PioneerQ35SharedRoleProjectionInFlightLedger(&b));
        b.fCaptureAttempted=false;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            b.fReleaseAttempted[core]=b.fReleaseConfirmed[core]=false;
            b.fStackReleaseSubmitted[core]=b.fStackReleaseAttempted[core]=false;
            b.fStackReleaseConfirmed[core]=b.fRunningImagesObserved[core]=false;
        }
        io.runtimeCore=oldCore;io.reset=oldReset;io.firmwareMatches=oldFirmware;service->ready=oldReady;
    }
    static void ExerciseUnstartedRamSources(PioneerQwen35SharedRoleBinding& b) {
        const uint32_t* reader=reinterpret_cast<const uint32_t*>(1);
        const uint32_t* writer=reinterpret_cast<const uint32_t*>(1);bool captured=true;
        assert(!PioneerQ35SharedRoleSavedRamSources(&b,0,reader,writer,captured)
            && !reader && !writer && !captured);
        auto* scope=b.Scope();assert(scope);
        reader=writer=reinterpret_cast<const uint32_t*>(1);captured=true;
        assert(!scope->ProveRoleHeldCopySources(b.fContext.io,0,reader,writer,captured)
            && !reader && !writer && !captured);
        reader=writer=reinterpret_cast<const uint32_t*>(1);captured=true;
        assert(!scope->ProveRoleRetirementRamSources(b.fContext.io,0,reader,writer,captured)
            && !reader && !writer && !captured);
    }
#endif
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
#ifdef TEST_SHARED_FABRIC_VIEWS
static void ExerciseFabricViewRetirement(PioneerSessionIO& io)
{
    auto* context=io.qwen35DataflowContext;
    context->qwen35SharedRoleBinding=PioneerQ35CreateSharedRoleBinding(*context);
    auto*& binding=context->qwen35SharedRoleBinding;assert(binding && gLocked);
    ResetMaps();gAllowWrite=true;assert(PioneerQ35SharedRoleBindingTestAccess::OpenViews(*binding));
    const void* code=PioneerQ35SharedRoleBindingTestAccess::CodeSnapshot(*binding);
    const void* ram=PioneerQ35SharedRoleBindingTestAccess::RamSnapshot(*binding);
    assert(code && ram && binding->FabricViewsReferenceArea(0));
    sColdRecoveryRequired=true;
    pioneer_driver_recovery_guard_v2_info guard,otherGuard;
    PioneerQ35FabricHostScope scope{context,&guard,9,true};
    assert(PioneerQ35FabricRoleViewsLayoutValid(*context)
        && PioneerQ35FabricRoleViewsReferenceArea(*context,0));
    assert(!PioneerQ35FabricDisposeRoleViews(*context,scope,&guard,9) && !gUnmapCalls);
    auto& proof=context->recoveryFabricEvidence;
    proof.flags=PIONEER_FABRIC_PROOF_REQUIRED & ~(PIONEER_FABRIC_PROOF_OLD_MAPPINGS_DISPOSED
        | PIONEER_FABRIC_PROOF_OWNERS_RELEASED | PIONEER_FABRIC_PROOF_PCI_RESERVATION_RELEASED);
    proof.core_count=DataflowBarrierWorkers;proof.retained_owners=1;
    proof.fabric_generation=proof.firmware_generation=9;
    context->recoveryFabricEvidenceRecorded=context->recoveryFabricPcieAudited=true;
    context->recoveryFabricPciIdle=context->recoveryFabricAuditReleased=true;
    context->recoveryFabricStartTicket=9;
    io.fabricOldMmioDisposeComplete=true;io.PioneerHostPageIO::area=0;
    assert(!PioneerQ35FabricDisposeRoleViews(*context,scope,&otherGuard,9) && !gUnmapCalls);
    assert(!PioneerQ35FabricDisposeRoleViews(*context,scope,&guard,8) && !gUnmapCalls);
    assert(!PioneerQ35FabricDisposeRoleViews(*context,scope,&guard,9) && !gUnmapCalls); // host-page alias
    io.PioneerHostPageIO::area=-1;
    assert(PioneerQ35FabricDisposeRoleViews(*context,scope,&guard,9) && binding->FabricViewsDisposed() && gUnmapCalls);
    context->recoveryFabricRoleViewsDisposed=true; // controller publishes this after its final guard check
    assert(!binding->CanDestroy() && context->qwen35SharedRoleBinding==binding
        && PioneerQ35SharedRoleBindingTestAccess::CodeSnapshot(*binding)==code
        && PioneerQ35SharedRoleBindingTestAccess::RamSnapshot(*binding)==ram);
    assert(!PioneerQ35FabricDisposeRoleViews(*context,scope,&guard,9));
    PioneerQ35SharedRoleBindingTestAccess::Guard bar;
    auto permit=[]() { return true; };auto noAlias=[](int32_t) { return false; };
    assert(!binding->OpenFabricRecoveryViews(bar,permit,noAlias)); // no fresh session
    gMapCalls=0; // the fake mapper numbers a fresh pass from its first window
    PioneerStoreContext freshContext;MockRecoverySession freshSession{&freshContext};
    context->recoveryFreshSession=&freshSession;
    assert(!PioneerQ35FabricRoleRestorationMatches(*context,9)); // no role or parent receipt
    assert(binding->OpenFabricRecoveryViews(bar,permit,noAlias) && binding->FabricRecoveryViewsOpen()
        && !binding->FabricViewsDisposed() && !binding->CanDestroy());
    assert(!binding->BeginFabricCoreRecovery(io,*io.qwen35BorrowedRoleScope,permit)
        && !binding->BeginFabricCoreRecovery(freshContext.io,*io.qwen35BorrowedRoleScope,permit)
        && !binding->RestoreFabricCoreFleet(io,*io.qwen35BorrowedRoleScope,permit,1)
        && !binding->RestoreFabricCoreFleet(freshContext.io,*io.qwen35BorrowedRoleScope,permit,1)
        && !binding->VerifyFabricRolesAfterParent(freshContext.io,*io.qwen35BorrowedRoleScope,permit)
        && !binding->FabricCoreRecoveryUsable() && !binding->FabricCoreAlreadyRestored(0));
    assert(binding->CloseFabricRecoveryViews(permit,noAlias) && binding->FabricRecoveryViewsClosed()
        && !binding->CanDestroy() && PioneerQ35SharedRoleBindingTestAccess::CodeSnapshot(*binding)==code
        && PioneerQ35SharedRoleBindingTestAccess::RamSnapshot(*binding)==ram);
    assert(!PioneerQ35FabricRoleRestorationMatches(*context,9)); // close alone is not restoration
    assert(!PioneerQ35RecordFabricRoleRetirement(*context,9,permit)); // no retirement receipt
    assert(!PioneerQ35FabricRoleRetirementMatches(*context,9)); // no detached-session authority
    // Test teardown only: these mappings and snapshots are simulated.
    delete binding;binding=nullptr;context->recoveryFabricEvidenceRecorded=false;
    context->recoveryFabricPcieAudited=false;context->recoveryFabricPciIdle=false;
    context->recoveryFabricAuditReleased=false;context->recoveryFabricStartTicket=0;
    context->recoveryFabricRoleViewsDisposed=false;context->recoveryFreshSession=nullptr;io.fabricOldMmioDisposeComplete=false;
    sColdRecoveryRequired=false;gAllowWrite=false;
}
#endif
#endif

#if defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) && defined(TEST_SHARED_POST_RESET)
static void ExerciseAutoRestoreImageBinding(PioneerSessionIO& io)
{
    auto* context=io.qwen35DataflowContext;
    context->qwen35SharedRoleBinding=PioneerQ35CreateSharedRoleBinding(*context);
    auto*& binding=context->qwen35SharedRoleBinding;
    assert(binding && gLocked);
    assert(!PioneerQ35SharedRoleProjectionRestored(nullptr)
        && !PioneerQ35SharedRoleProjectionRestored(binding));
    const auto ordinary=PioneerQ35SharedRoleBindingTestAccess::Image(*binding);
    assert(ordinary.Valid() && ordinary.code[DataflowRoleReader]==DataflowSharedReaderCodeWords);
    assert(!PioneerQ35SelectSharedRoleAutoRestore(nullptr));
    assert(PioneerQ35SelectSharedRoleAutoRestore(binding) && !PioneerQ35SelectSharedRoleAutoRestore(binding)
        && !binding->SelectCoreLocalRestore());
    assert(!PioneerQ35SharedRoleProjectionRestored(binding));
    const auto image=PioneerQ35SharedRoleBindingTestAccess::Image(*binding);
    assert(image.Valid() && image.code[DataflowRoleReader]==DataflowServiceProbeReaderCodeWords
        && image.code[DataflowRoleWriter]==DataflowServiceProbeWriterCodeWords
        && image.shim[DataflowRoleReader]==DataflowServiceProbeReaderShimWords
        && image.shim[DataflowRoleWriter]==DataflowServiceProbeWriterShimWords);
    assert(image.bounds.ContainsRunning(DataflowRoleReader,DataflowRoleReaderCodeAddress
        +(DataflowServiceProbeReaderRuntimeCodeWords-1)*4));
    assert(image.bounds.ContainsRunning(DataflowRoleReader,DataflowServiceProbeReaderLoopAddress)
        && image.bounds.ContainsRunning(DataflowRoleReader,0x700cc)
        && image.bounds.ContainsRunning(DataflowRoleWriter,DataflowServiceProbeWriterLoopAddress));
    assert(!image.bounds.ContainsRunning(DataflowRoleReader,DataflowServiceProbeReaderRestoreAddress)
        && !image.bounds.ContainsRunning(DataflowRoleWriter,DataflowServiceProbeWriterRestoreAddress)
        && !image.bounds.ContainsRunning(DataflowRoleReader,DataflowServiceProbeReaderShimParkAddress)
        && !image.bounds.ContainsRunning(DataflowRoleReader,DataflowRoleReaderShimAddress)
        && !image.bounds.ContainsRunning(DataflowRoleReader,DataflowServiceProbeReaderLoopEnd));
    ResetMaps();gAllowWrite=true;
    assert(PioneerQ35SharedRoleBindingTestAccess::OpenViews(*binding));
#ifndef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
    PioneerQ35SharedRoleBindingTestAccess::ExerciseSnapshotHoldLedger(*binding);
    PioneerQ35SharedRoleBindingTestAccess::ExerciseSnapshotRuntimeObservation(*binding);
    PioneerQ35SharedRoleBindingTestAccess::ExerciseProjectionInFlight(*binding);
    PioneerQ35SharedRoleBindingTestAccess::ExerciseUnstartedRamSources(*binding);
#endif
    const unsigned resetWrites=io.sharedResetWrites;
    assert(!binding->ArmAndBeginRelease() && !binding->StartCore(0) && io.sharedResetWrites==resetWrites);
    assert(PioneerQ35SharedRoleBindingTestAccess::CloseViews(*binding));
    assert(PioneerQ35DestroySharedRoleBinding(binding));binding=nullptr;gAllowWrite=false;
}
static void ExerciseCoreLocalBinding(PioneerSessionIO& io)
{
    auto* context=io.qwen35DataflowContext;
    context->qwen35SharedRoleBinding=PioneerQ35CreateSharedRoleBinding(*context);
    auto*& binding=context->qwen35SharedRoleBinding;
    assert(binding && gLocked && binding->SelectCoreLocalRestore());
    assert(!binding->SelectCoreLocalRestore());
    ResetMaps();gAllowWrite=true;
    assert(PioneerQ35SharedRoleBindingTestAccess::OpenViews(*binding));
    const unsigned resetWrites=io.sharedResetWrites;
    assert(!binding->StageCoreLocalRestore(0) && !binding->BeginCoreLocalRestoreRelease(0)
        && !binding->MarkCoreLocalRestoreComplete(0) && !binding->PrepareCoreLocalRestoreHandoff(0)
        && !binding->ConfirmCoreLocalRestoreHandoff(0) && !binding->RestoreCoreLocalHandoff(0)
        && !binding->HoldCoreLocalRestore(0));
    assert(io.sharedResetWrites==resetWrites);
    assert(binding->FinalizeBeforeResume() && PioneerQ35DestroySharedRoleBinding(binding));
    binding=nullptr;gAllowWrite=false;
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

#if defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) && defined(TEST_SHARED_POST_RESET) \
    && !defined(PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS)
static void ExerciseQualificationCaptureRoot(PioneerSessionIO& io)
{
    auto* context=io.qwen35DataflowContext;
    assert(context && io.qwen35BorrowedRoleScope && context->qwen35DataflowService);
    ResetMaps();gAllowWrite=true;
    context->qwen35SharedRoleBinding=PioneerQ35CreateSharedRoleBinding(*context);
    auto* binding=context->qwen35SharedRoleBinding;assert(binding);
    assert(PioneerQ35SelectSharedRoleAutoRestore(binding));
    io.simulateArc=true;io.simulateHdma=true;io.firmwareMatches=true;
    context->qwen35DataflowService->capturePending=true;
    assert(io.SelectRuntimeCore(119));
    gQualificationHeldOwnerAllowed=false;
    const unsigned before=io.selectCalls;
    assert(!PioneerQ35SharedRoleBindingTestAccess::SelectCaptureRoot(*binding)
        && io.runtimeCore==119 && io.selectCalls==before);
    gQualificationHeldOwnerAllowed=true;
    assert(PioneerQ35SharedRoleBindingTestAccess::SelectCaptureRoot(*binding) && io.runtimeCore==0);
    const unsigned selectedRoot=io.selectCalls;
    assert(PioneerQ35SharedRoleBindingTestAccess::SelectCaptureRoot(*binding)
        && io.selectCalls==selectedRoot);
    assert(io.SelectRuntimeCore(119));io.tlb0Override=true;
    const unsigned invalidRoute=io.selectCalls;
    assert(!PioneerQ35SharedRoleBindingTestAccess::SelectCaptureRoot(*binding)
        && io.selectCalls==invalidRoute && !sColdRecoveryRequired);
    assert(io.SelectRuntimeCore(119));io.failNextSelection=true;
    assert(!PioneerQ35SharedRoleBindingTestAccess::SelectCaptureRoot(*binding)
        && sColdRecoveryRequired && io.tlb0Override && io.runtimeCore==119);
    sColdRecoveryRequired=false; // simulated external repair of the partial TLB write
    assert(io.SelectRuntimeCore(119));
    // Model each tile's BAR windows independently, as on the real device.
    gPerCoreRoleWindows=true;gRoleWindowCore=gRoleSelectedCore=io.runtimeCore;
    // The inspector restores worker 119; capture selects root, stages all
    // held tiles, and still cannot release a worker without the later proof.
    const unsigned resetWrites=io.sharedResetWrites;
    assert(io.SharedCaptureAndStageHeld() && !sColdRecoveryRequired
        && gMapCalls==DataflowRolePhysicalControl+1 && io.sharedResetWrites==resetWrites);
    assert(!io.SharedArmAndBeginRelease() && !io.SharedStartCore(0) && io.sharedResetWrites==resetWrites);
    assert(PioneerQ35SharedRoleBindingTestAccess::HeldPreservationPrepared(*binding));
    context->qwen35DataflowService->capturePending=false;
    context->qwen35DataflowService->faulted=true;
    gQualificationHeldOwnerAllowed=false;
    gQualificationHeldRollbackOwnerAllowed=true;
    assert(PioneerQ35CanAbortUnstagedQualification(binding));
#ifdef TEST_PRESERVATION_ROLLBACK
    PioneerQ35SharedRoleBindingTestAccess::CheckControlRollbackOnly(*binding);
#endif
    PioneerQ35SharedRoleBindingTestAccess::MarkReleaseAttempt(*binding,7,true);
    assert(!PioneerQ35CanAbortUnstagedQualification(binding));
    PioneerQ35SharedRoleBindingTestAccess::MarkReleaseAttempt(*binding,7,false);
    io.reset=0x46800;assert(!PioneerQ35CanAbortUnstagedQualification(binding));io.reset=0x47800;
    sColdRecoveryRequired=true;assert(!PioneerQ35CanAbortUnstagedQualification(binding));sColdRecoveryRequired=false;
    gQualificationHeldRollbackOwnerAllowed=false;
    assert(!io.SharedRestoreRolesHeld());
    gQualificationHeldRollbackOwnerAllowed=true;
    assert(io.SharedRestoreRolesHeld() && io.SharedVerifyRolesAfterResidentRestore());
    for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
        assert(io.SelectRuntimeCore(core));
        for (unsigned window=0;window<=DataflowRolePhysicalExtraCode;window++)
            assert(gMemory[window][0]==RoleWindowInitial(core,window));
#ifdef TEST_PRESERVATION_ROLLBACK
        assert(gMemory[DataflowRolePhysicalControl][0x224/4]==RoleWindowInitial(core,DataflowRolePhysicalControl));
#endif
    }
    assert(io.SharedFinalizeBeforeResume() && PioneerQ35DestroySharedRoleBinding(binding));
    context->qwen35SharedRoleBinding=nullptr;gQualificationHeldOwnerAllowed=false;
    gQualificationHeldRollbackOwnerAllowed=false;context->qwen35DataflowService->faulted=false;
    context->qwen35DataflowService->capturePending=true;
    gPerCoreRoleWindows=false;for (auto& windows:gRoleTileWindows) windows.clear();
    io.simulateArc=false;io.simulateHdma=false;ResetMaps();
}
#endif

static void ExerciseStatusSampling(PioneerSessionIO& io)
{
    auto* context=io.qwen35DataflowContext;
    assert(context && io.qwen35BorrowedRoleScope);
    context->qwen35SharedRoleBinding=PioneerQ35CreateSharedRoleBinding(*context);
    assert(context->qwen35SharedRoleBinding);
    io.runtimeCore=7; io.simulateArc=true; io.simulateHdma=true; io.firmwareMatches=true;
    io.selectCalls=io.tlbWrites=io.hdmaReads=0;
    auto* scope=io.qwen35BorrowedRoleScope;
#if defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) && !defined(PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS)
    assert(!scope->ProveRoleFleetHeldAndQuiet(io,7));
    gQualificationHeldOwnerAllowed=true;
    assert(scope->CanSelectRoleHeldCore(io,8) && !scope->ProveRoleFleetHeldAndQuiet(io,8));
    {
        PioneerQwen35RoleHaikuMap platform;
        DataflowRoleWritableViews<PioneerQwen35RoleHaikuMap> views(platform);
        PioneerQwen35RoleFleetPciIO<PioneerSessionIO,PioneerQwen35RoleBorrowedScopeWitness,decltype(views)>
            fleet(io,*scope,views);
        // Use the real scope and adapter: each check must select its own
        // tile before the selected-tile held/NoC observation.
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) assert(fleet.AllRolesHeld(core));
        gQualificationHeldOwnerAllowed=false;
        const unsigned selections=io.selectCalls;
        assert(!fleet.AllRolesHeld(0) && io.selectCalls==selections);
        gQualificationHeldOwnerAllowed=true;
    }
    for (unsigned core=0;core<DataflowBarrierWorkers;core++)
        assert(io.SelectRuntimeCore(core) && scope->ProveRoleFleetHeldAndQuiet(io,core));
    assert(io.SelectRuntimeCore(7) && scope->ProveRoleFleetHeldAndQuiet(io,7));
    context->qwen35DataflowService->capturePending=false;
    assert(!scope->ProveRoleMemoryReadSafe(io,7,DataflowRolePhysicalExtraCode));
    context->qwen35DataflowService->capturePending=true;
    assert(scope->ProveRoleMemoryReadSafe(io,7,DataflowRolePhysicalExtraCode)
        && !scope->ProveRoleMemoryReadSafe(io,7,DataflowRolePhysicalBriscRam)
        && !scope->ProveRoleMemoryReadSafe(io,8,DataflowRolePhysicalExtraCode)
        && !scope->ProveRoleMemoryWriteSafe(io,7,DataflowRolePhysicalExtraCode,0,4));
    for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
        assert(io.SelectRuntimeCore(core)
            && scope->ProveRoleMemoryReadSafe(io,core,DataflowRolePhysicalExtraCode)
            && !scope->ProveRoleMemoryWriteSafe(io,core,DataflowRolePhysicalExtraCode,0,4)
            && !scope->ProveRoleControlWriteSafe(io,core,0,DataflowRolePreserveResetMask));
    }
    {
        ResetMaps();gAllowWrite=true;
        PioneerQwen35RoleHaikuMap platform;
        DataflowRoleWritableViews<PioneerQwen35RoleHaikuMap> views(platform);
        PioneerQwen35RoleRouteGuard<PioneerSessionIO,PioneerQwen35RoleBorrowedScopeWitness> route(io,*scope);
        assert(views.Open(io.profile.bar0_host_start,io.profile.bar0_host_end,route));
        PioneerQwen35RoleFleetPciIO<PioneerSessionIO,PioneerQwen35RoleBorrowedScopeWitness,decltype(views)>
            fleet(io,*scope,views);
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            uint32_t saved=0,shim=0,boot=0,replacement=0x11223344;
            assert(fleet.Read(core,DataflowRoleExtraL1Address,&saved,sizeof(saved))
                && saved==0x76540000U+DataflowRolePhysicalExtraCode
                && fleet.ReadShim(core,DataflowRoleReaderShimAddress,&shim,sizeof(shim))
                && shim==0x76540000U+DataflowRolePhysicalShim
                && fleet.ReadBoot(core,DataflowRoleBriscBootAddress,boot)
                && boot==0x76540000U+DataflowRolePhysicalBriscBoot
                && !fleet.Write(core,DataflowRoleExtraL1Address,&replacement,sizeof(replacement))
                && gMemory[DataflowRolePhysicalExtraCode][0]==saved);
        }
        assert(views.Close());ResetMaps();
    }
    assert(io.SelectRuntimeCore(7));
    auto* binding=context->qwen35SharedRoleBinding;context->qwen35SharedRoleBinding=nullptr;
    assert(!scope->ProveRoleMemoryReadSafe(io,7,DataflowRolePhysicalExtraCode));
    context->qwen35SharedRoleBinding=binding;
    assert(!scope->ProveRoleFleetHeldAndQuiet(io,8));
    io.nocOutstanding[1][3]=1;assert(!scope->ProveRoleFleetHeldAndQuiet(io,7));io.nocOutstanding[1][3]=0;
    io.reset=0x6000;assert(!scope->ProveRoleFleetHeldAndQuiet(io,7));io.reset=0x47800;
    const unsigned disabledReads=io.resetReadsClockDisabled;
    io.nocConfig[0]=1U<<12;
    assert(!scope->ProveRoleFleetHeldAndQuiet(io,7) && io.resetReadsClockDisabled==disabledReads);
    io.nocConfig[0]=0;
    io.firmwareMatches=false;assert(!scope->ProveRoleFleetHeldAndQuiet(io,7));io.firmwareMatches=true;
    gQualificationHeldOwnerAllowed=false;assert(!scope->ProveRoleFleetHeldAndQuiet(io,7));
#endif
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
    // Exercise the actual combined inspection with the real route guard and
    // sampled transports. Every failed stage reports where it stopped, and
    // a failed route restoration is distinguishable from an ordinary refusal.
    PioneerQwen35FleetInspectionResult inspection;
    io.firmwareMatches=false;
    assert(!PioneerQwen35InspectHeldFleet(io,*scope,route,inspection)
        && inspection.stage==PioneerQwen35FleetInspectionResult::Admission
        && !inspection.restoreFailed);
    io.firmwareMatches=true;io.simulateArc=false;
    assert(!PioneerQwen35InspectHeldFleet(io,*scope,route,inspection)
        && inspection.stage==PioneerQwen35FleetInspectionResult::ArcReset
        && !inspection.restoreFailed && !io.tlbWrites);
    io.simulateArc=true;io.resetReads=0;io.resetChangeAt=0;
    assert(!PioneerQwen35InspectHeldFleet(io,*scope,route,inspection)
        && inspection.stage==PioneerQwen35FleetInspectionResult::HeldFleet
        && inspection.failedCore==0 && !inspection.restoreFailed
        && io.runtimeCore==7 && route.ProveSelectedRoute(7));
    io.resetChangeAt=-1;io.hdmaWords[0]=1;
    assert(!PioneerQwen35InspectHeldFleet(io,*scope,route,inspection)
        && inspection.stage==PioneerQwen35FleetInspectionResult::HostDma
        && !inspection.restoreFailed && io.runtimeCore==7 && route.ProveSelectedRoute(7));
    io.hdmaWords[0]=0;io.hdmaReads=0;io.dropHdmaRestore=true;
    assert(!PioneerQwen35InspectHeldFleet(io,*scope,route,inspection)
        && inspection.stage==PioneerQwen35FleetInspectionResult::HostDma
        && inspection.restoreFailed);
    io.dropHdmaRestore=false;
    assert(io.SelectRuntimeCore(7) && route.ProveSelectedRoute(7));
    io.hdmaReads=0;
    assert(PioneerQwen35InspectHeldFleet(io,*scope,route,inspection)
        && inspection.stage==PioneerQwen35FleetInspectionResult::Complete
        && inspection.l2cpuReset==0x0f && inspection.failedCore==DataflowBarrierWorkers
        && !inspection.restoreFailed && io.runtimeCore==7 && route.ProveSelectedRoute(7));
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
    bool qualificationRestoreFailed=false;
    assert(!PioneerQ35ProjectionQualificationScope::Inspect(*scope,io,qualificationRestoreFailed));
    auto* savedService=context->qwen35DataflowService;
    auto* savedBinding=context->qwen35SharedRoleBinding;
    context->qwen35DataflowService=nullptr;context->qwen35SharedRoleBinding=nullptr;
    assert(!scope->ProveRoleStatusInspectionAllowed(io));
    assert(PioneerQ35ProjectionQualificationScope::Inspect(*scope,io,qualificationRestoreFailed)
        && !qualificationRestoreFailed && io.runtimeCore==7 && route.ProveSelectedRoute(7));
    assert(!scope->ProveRoleStatusInspectionAllowed(io));
    io.hdmaReads=0;io.dropHdmaRestore=true;
    assert(!PioneerQ35ProjectionQualificationScope::Inspect(*scope,io,qualificationRestoreFailed)
        && qualificationRestoreFailed && !scope->ProveRoleStatusInspectionAllowed(io));
    io.dropHdmaRestore=false;
    assert(io.SelectRuntimeCore(7) && route.ProveSelectedRoute(7));
    context->qwen35DataflowService=savedService;context->qwen35SharedRoleBinding=savedBinding;
#endif
    io.selectCalls=io.tlbWrites=io.hdmaReads=0;
    bool restoreFailed=false;
    io.simulateHdma=false;
    assert(!PioneerQwen35SampleHostDma(io,*scope,route,restoreFailed) && !restoreFailed);
    assert(!io.selectCalls && !io.tlbWrites);
    io.simulateHdma=true;io.failNextSelection=true;
    assert(!PioneerQwen35SampleHostDma(io,*scope,route,restoreFailed) && restoreFailed);
    assert(io.selectCalls==1 && io.tlb0Override && !io.hdmaReads && !io.tlbWrites);
    assert(io.SelectRuntimeCore(7) && route.ProveSelectedRoute(7));
    io.selectCalls=0;
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
    io.firmwareMatches=false;assert(!io.SharedArmAndBeginRelease());
    io.firmwareMatches=true;assert(io.SharedArmAndBeginRelease() && !io.SharedStartCore(1));
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

static void ExerciseInspectionRestoreFailure(PioneerSessionIO& io)
{
    auto* context=io.qwen35DataflowContext;
    context->qwen35SharedRoleBinding=PioneerQ35CreateSharedRoleBinding(*context);
    assert(context->qwen35SharedRoleBinding && !sColdRecoveryRequired);
    io.runtimeCore=7;io.simulateArc=io.simulateHdma=io.firmwareMatches=true;
    io.hdmaSourceStable=true;io.hdmaReads=0;io.dropHdmaRestore=true;
    assert(!io.SharedCaptureAndStageHeld() && sColdRecoveryRequired);
    assert(!gMapCalls && !io.sharedResetWrites && !io.holdWrites);
    assert(PioneerQ35DestroySharedRoleBinding(context->qwen35SharedRoleBinding));
    context->qwen35SharedRoleBinding=nullptr;
    // Restore the fake route for subsequent tests; this does not clear the
    // actual binding's cold latch, which is checked after the locked call.
    io.dropHdmaRestore=false;assert(io.SelectRuntimeCore(7));
    io.simulateArc=io.simulateHdma=io.firmwareMatches=false;
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

// Exercise the actual compile-only admission, binding, image staging,
// release ledger and PCI adapter against per-tile simulated MMIO.
static void ExerciseReleaseBinding(PioneerSessionIO& io,unsigned failCore)
{
    auto* context=io.qwen35DataflowContext;
    context->qwen35SharedRoleBinding=PioneerQ35CreateSharedRoleBinding(*context);
    auto* binding=context->qwen35SharedRoleBinding;
    assert(binding && io.qwen35BorrowedRoleScope);
    io.runtimeCore=7;io.simulateArc=io.simulateHdma=io.firmwareMatches=true;
    io.hdmaSourceStable=true;io.reset=0x47800;io.resetReads=0;io.sharedResetWrites=0;
    io.roleStopWrites=io.computeStopWrites=io.holdWrites=0;
    std::fill(std::begin(io.tileReset),std::end(io.tileReset),0x47800);
    io.perCoreReset=true;io.dropReleaseCore=failCore;
    gAllowWrite=true;gPerCoreRoleWindows=true;gRoleWindowCore=gRoleSelectedCore=io.runtimeCore;
    assert(io.SharedCaptureAndStageHeld());
    io.firmwareMatches=false;assert(!io.SharedArmAndBeginRelease());
    io.firmwareMatches=true;
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
    if (failCore==DataflowBarrierWorkers) {
        // Model the worker-written startup receipts and program counters. The
        // actual binding must inspect each tile's staged image and readiness.
        io.computePc=DataflowServiceEntryAddress+4;
        io.rolePc[DataflowRoleReader]=DataflowRoleReaderCodeAddress+4;
        io.rolePc[DataflowRoleWriter]=DataflowRoleWriterCodeAddress+4;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            auto& role=io.roleBoxes[core];role={};
            role.magic=DataflowRoleMailboxMagic;role.version=DataflowRoleMailboxVersion;
            assert(DataflowRoleMailboxPublishStarted(role,DataflowRoleReader)
                && DataflowRoleMailboxPublishStarted(role,DataflowRoleWriter));
            auto& worker=io.workerBoxes[core];worker={};
            worker.magic=DataflowWorkerMailboxMagic;worker.version=DataflowWorkerMailboxVersion;
            worker.logicalCore=core;worker.state=DataflowWorkerReady;
        }
        DataflowWorkerSnapshot snapshot{};
        assert(io.ReadWorkerSnapshot(7,snapshot));
        io.roleBoxes[7].started[DataflowRoleWriter]=0;
        assert(!io.SharedCoreReady(7,snapshot));
        assert(DataflowRoleMailboxPublishStarted(io.roleBoxes[7],DataflowRoleWriter));
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            assert(io.ReadWorkerSnapshot(core,snapshot) && io.SharedCoreReady(core,snapshot));
        }
        // A warm graph may have advanced independent worker/role sequences.
        // Startup image inspection is cached, but readiness still validates
        // each fresh mailbox, route, reset, PC and service/firmware state.
        auto& role=io.roleBoxes[0];
        assert(DataflowRoleMailboxIssue(role,1));
        for (unsigned r=0;r<DataflowRoleCount;r++)
            assert(DataflowRoleMailboxBegin(role,r,1) && DataflowRoleMailboxComplete(role,r,1,true));
        assert(DataflowRoleMailboxAcknowledge(role,1,true,true));
        auto& worker=io.workerBoxes[0];
        worker.commandSequence=worker.acknowledgedSequence=worker.completedSequence=3;
        assert(io.ReadWorkerSnapshot(0,snapshot));
        context->qwen35DataflowService->capturePending=false;
        assert(io.SharedCoreReady(0,snapshot));
        io.firmwareMatches=false;assert(!io.SharedCoreReady(0,snapshot));io.firmwareMatches=true;
        io.queueBoxes[0][0].produced=1;assert(!io.SharedCoreReady(0,snapshot));
        io.queueBoxes[0][0].produced=0;
        io.rolePc[DataflowRoleWriter]=DataflowExclusiveWriterParkAddress;
        assert(!io.SharedCoreReady(0,snapshot));
        io.rolePc[DataflowRoleWriter]=DataflowRoleWriterCodeAddress+4;
        context->qwen35DataflowService->ready=false;
        assert(!io.SharedCoreReady(0,snapshot));
        context->qwen35DataflowService->graphRunning=true;
        assert(io.SharedCoreReady(0,snapshot));
        context->qwen35DataflowService->graphRunning=false;
        context->qwen35DataflowService->ready=true;
        context->qwen35DataflowService->capturePending=true;
    }
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
    if (failCore==DataflowBarrierWorkers) {
        context->qwen35DataflowService->cleanStopPending=true;io.simulateCleanStop=true;
        assert(io.SharedStopAndHoldAll(stage,core) && !stage && core==DataflowBarrierWorkers);
        assert(io.roleStopWrites==DataflowBarrierWorkers && io.computeStopWrites==DataflowBarrierWorkers
            && io.holdWrites==DataflowBarrierWorkers);
        assert(io.SharedStopAndHoldAll(stage,core)); // observations only, no duplicate writes
        assert(io.roleStopWrites==DataflowBarrierWorkers && io.computeStopWrites==DataflowBarrierWorkers
            && io.holdWrites==DataflowBarrierWorkers);
        assert(io.SelectRuntimeCore((io.runtimeCore+1)%DataflowBarrierWorkers));
        for (uint32_t reset:io.tileReset) assert(reset==0x47800);
        context->qwen35DataflowService->cleanStopPending=false;io.simulateCleanStop=false;
    } else {
        assert(!io.SharedStopAndHoldAll(stage,core) && stage==15);
    }
    const unsigned writesBeforeRetry=io.sharedResetWrites;
    assert(!io.SharedStartCore(failCore<DataflowBarrierWorkers?failCore+1:0)
        && io.sharedResetWrites==writesBeforeRetry);
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

#ifdef TEST_REAL_SERVICE_INTEGRATION
#define main existing_shared_binding_test_main
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
#if defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) && defined(TEST_SHARED_POST_RESET) \
    && !defined(PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS)
    gBorrowedScopeHook=ExerciseQualificationCaptureRoot;
    assert(Qwen35DataflowControl(&cookie,5,nullptr,0)==B_OK);
#endif
    ResetMaps();gBorrowedScopeHook=ExerciseInspectionRestoreFailure;
    assert(Qwen35DataflowControl(&cookie,5,nullptr,0)==B_OK && sColdRecoveryRequired);
    sColdRecoveryRequired=false; // simulated external recovery, not a driver operation
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
#ifdef TEST_SHARED_FABRIC_VIEWS
    gBorrowedScopeHook=ExerciseFabricViewRetirement;
    assert(Qwen35DataflowControl(&cookie,5,nullptr,0)==B_OK);
#endif
#endif
#if defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) && defined(TEST_SHARED_POST_RESET)
    gBorrowedScopeHook=ExerciseCoreLocalBinding;
    assert(Qwen35DataflowControl(&cookie,5,nullptr,0)==B_OK);
    gBorrowedScopeHook=ExerciseAutoRestoreImageBinding;
    assert(Qwen35DataflowControl(&cookie,5,nullptr,0)==B_OK);
#endif
    service.capturePending=false;
    assert(!gLocked && !context.qwen35SharedRoleBinding && !context.io.qwen35BorrowedRoleScope);
    gBorrowedScopeHook=nullptr;
#if defined(PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS) && defined(TEST_SHARED_POST_RESET)
    std::puts("PASS observed dedicated release and partial-stop retention with actual binding; SIMULATED ONLY");
#else
    std::puts("PASS actual shared role binding denies mapping, reset and release without fleet proof; SIMULATED ONLY");
#endif
    return 0;
}
#ifdef TEST_REAL_SERVICE_INTEGRATION
#undef main
#endif
