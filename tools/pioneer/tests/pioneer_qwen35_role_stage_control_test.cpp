/* SPDX-License-Identifier: MIT */
// The actual stage control, image owner, PCI adapter and Haiku mapping run
// against isolated per-core fake BAR windows; no accelerator is accessed.
#define TEST_ROLE_STAGE_QUALIFY 1
#define TEST_SHARED_POST_RESET 1
#define PIONEER_QWEN35_ROLE_STAGE_QUALIFY 1
#define PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS 1
#include "PioneerQwen35ExclusiveRoleImages.h"
#include "PioneerQwen35DataflowServiceImage.h"
#include "PioneerQwen35SharedRoleImages.h"
#include "pioneer_qwen35_fleet_inspection_control_test.cpp"
#include <new>
constexpr status_t B_NO_MEMORY=-5;
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35RoleStageControl.inc"

static bool EndGuard(PioneerStoreContext& c,tt_resident_session_result& result,bool& clean) {
#include "pioneer_q35_role_stage_end_guard.inc"
    return true;
}
static tt_q35_role_stage_qualification Request() {
    tt_q35_role_stage_qualification r{};r.version=1;
    r.expected_tag=PIONEER_DRIVER_BUILD_TAG;r.expected_generation=42;return r;
}
#ifndef TEST_ROLE_PRESTACK_CONTROL
int main()
{
    (void)&Qwen35DataflowGenerate;
    {
        Fixture f;auto r=Request();
        assert(Qwen35RoleStageControl(&f.cookie,&r,sizeof(r))==B_OK
            && r.operation_status==B_NOT_ALLOWED && !f.context.qwen35RoleStageAttempted);
    }
    {
        Fixture f;assert(f.Run()==B_OK && f.request.operation_status==B_OK);
        assert(f.context.qwen35FleetInspectionPassed);
        ResetMaps();gAllowWrite=true;gFailMap=0;
        auto r=Request();
        assert(Qwen35RoleStageControl(&f.cookie,&r,sizeof(r))==B_OK
            && r.operation_status==B_DEV_NOT_READY && r.stage==3);
        assert(f.context.qwen35RoleStageQualification && f.context.dispatchOwnerRetained
            && sColdRecoveryRequired && gRetains==1);
        tt_resident_session_result result{};bool clean=true;
        assert(!EndGuard(f.context,result,clean) && !clean && gRetains==2);
        // Simulate a full external power cycle solely to free the test heap.
        delete f.context.qwen35RoleStageQualification;f.context.qwen35RoleStageQualification=nullptr;
        gAllowWrite=false;gFailMap=-1;sColdRecoveryRequired=false;
    }
    {
        Fixture f;assert(f.Run()==B_OK && f.request.operation_status==B_OK);
        ResetMaps();gAllowWrite=true;gPerCoreRoleWindows=true;
        gRoleWindowCore=gRoleSelectedCore=f.context.io.runtimeCore;
        for (unsigned noc=0;noc<2;noc++) f.context.io.nocAvailable[noc]=0x10101010U;
        auto r=Request();
        assert(Qwen35RoleStageControl(&f.cookie,&r,sizeof(r))==B_OK
            && r.operation_status==B_OK && r.stage==TT_Q35_ROLE_STAGE_COMPLETE);
        assert(r.captured && r.staged && r.restored && r.mappings_closed
            && !f.context.qwen35RoleStageQualification && !sColdRecoveryRequired && !gRetains);
        assert(!f.context.io.sharedResetWrites && !f.context.io.holdWrites);
        assert(f.context.io.SelectRuntimeCore((f.context.io.runtimeCore+1)%DataflowBarrierWorkers));
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            for (unsigned which=0;which<=DataflowRolePhysicalExtraCode;which++)
                for (uint32_t word:gRoleTileWindows[core][which])
                    assert(word==RoleWindowInitial(core,which));
        gPerCoreRoleWindows=false;gAllowWrite=false;
        for (auto& windows:gRoleTileWindows) windows.clear();
        r=Request();assert(Qwen35RoleStageControl(&f.cookie,&r,sizeof(r))==B_OK
            && r.operation_status==B_NOT_ALLOWED);
    }
    {
        Fixture f;assert(f.Run()==B_OK && f.request.operation_status==B_OK);
        ResetMaps();gAllowWrite=true;gPerCoreRoleWindows=true;
        gRoleWindowCore=gRoleSelectedCore=f.context.io.runtimeCore;
        for (unsigned noc=0;noc<2;noc++) f.context.io.nocAvailable[noc]=0x10101010U;
        gFailAfterStageWrite=true;gStageWriteFailureInjected=false;
        auto r=Request();
        assert(Qwen35RoleStageControl(&f.cookie,&r,sizeof(r))==B_OK
            && r.operation_status==B_DEV_NOT_READY && r.stage==5 && r.captured && !r.staged);
        assert(gStageWriteFailureInjected && f.context.qwen35RoleStageQualification
            && f.context.dispatchOwnerRetained && sColdRecoveryRequired && gRetains==1);
        // Release fake mappings/heap only to model external power-cycle cleanup.
        gFailAfterStageWrite=false;assert(acquire_sem(sLock)==B_OK);
        assert(f.context.qwen35RoleStageQualification->views.Close());
        assert(release_sem(sLock)==B_OK);
        delete f.context.qwen35RoleStageQualification;f.context.qwen35RoleStageQualification=nullptr;
        gPerCoreRoleWindows=false;gAllowWrite=false;sColdRecoveryRequired=false;
        for (auto& windows:gRoleTileWindows) windows.clear();
    }
    {
        Fixture f;assert(f.Run()==B_OK && f.request.operation_status==B_OK);
        ResetMaps();gAllowWrite=true;gPerCoreRoleWindows=true;
        gRoleWindowCore=gRoleSelectedCore=f.context.io.runtimeCore;
        for (unsigned noc=0;noc<2;noc++) f.context.io.nocAvailable[noc]=0x10101010U;
        gFailAfterStageWrite=true;gFailStageWriteOnce=true;gStageWriteFailureInjected=false;
        auto r=Request();
        assert(Qwen35RoleStageControl(&f.cookie,&r,sizeof(r))==B_OK
            && r.operation_status==B_DEV_NOT_READY && r.stage==5 && r.captured && !r.staged
            && r.restored && r.mappings_closed);
        assert(gStageWriteFailureInjected && !f.context.qwen35RoleStageQualification
            && !f.context.dispatchOwnerRetained && !sColdRecoveryRequired && !gRetains);
        assert(!f.context.io.sharedResetWrites && !f.context.io.holdWrites);
        assert(f.context.io.SelectRuntimeCore((f.context.io.runtimeCore+1)%DataflowBarrierWorkers));
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            for (unsigned which=0;which<=DataflowRolePhysicalExtraCode;which++)
                for (uint32_t word:gRoleTileWindows[core][which])
                    assert(word==RoleWindowInitial(core,which));
        gFailStageWriteOnce=false;gPerCoreRoleWindows=false;gAllowWrite=false;
        for (auto& windows:gRoleTileWindows) windows.clear();
    }
    {
        Fixture f;assert(f.Run()==B_OK && f.request.operation_status==B_OK);
        ResetMaps();gAllowWrite=true;gPerCoreRoleWindows=true;
        gRoleWindowCore=gRoleSelectedCore=f.context.io.runtimeCore;
        for (unsigned noc=0;noc<2;noc++) f.context.io.nocAvailable[noc]=0x10101010U;
        gFailGuardAfterStageWrite=true;gStageGuardFailureInjected=false;
        auto r=Request();
        assert(Qwen35RoleStageControl(&f.cookie,&r,sizeof(r))==B_OK
            && r.operation_status==B_DEV_NOT_READY && r.stage==5 && r.captured
            && !r.staged && !r.restored && !r.mappings_closed);
        assert(gStageGuardFailureInjected && f.context.qwen35RoleStageQualification
            && !f.context.qwen35RoleStageQualification->views.ImageWritesConfirmed()
            && f.context.dispatchOwnerRetained && sColdRecoveryRequired && gRetains==1);
        // An unconfirmed write must never be treated as a successful rollback.
        assert(acquire_sem(sLock)==B_OK);
        assert(f.context.qwen35RoleStageQualification->views.Close());
        assert(release_sem(sLock)==B_OK);
        delete f.context.qwen35RoleStageQualification;f.context.qwen35RoleStageQualification=nullptr;
        gPerCoreRoleWindows=false;gAllowWrite=false;sColdRecoveryRequired=false;
        for (auto& windows:gRoleTileWindows) windows.clear();
    }
    std::puts("PASS held-role stage, full image restoration and retained mapping failure; SIMULATED ONLY");
}
#endif
