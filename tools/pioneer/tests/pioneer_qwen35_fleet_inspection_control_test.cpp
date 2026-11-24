/* SPDX-License-Identifier: MIT */
// Exercises the actual restricted control and sampling code with simulated
// Haiku session, fleet and MMIO facilities; no hardware is accessed.
#define TEST_FLEET_INSPECTION_CONTROL 1
#define PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS 1
#define PIONEER_QWEN35_RECOVERY_QUALIFY 1
#define PIONEER_QWEN35_DEDICATED_FLEET_WITNESS 1
#define PIONEER_QWEN35_FLEET_INSPECTION_QUALIFY 1
#ifdef PIONEER_QWEN35_RISC_RESET_SAMPLE_QUALIFY
#define PIONEER_DRIVER_BUILD_TAG UINT64_C(0x2026112400030023)
#elif defined(TEST_ROLE_STAGE_QUALIFY)
#define PIONEER_DRIVER_BUILD_TAG UINT64_C(0x2026112300030005)
#else
#define PIONEER_DRIVER_BUILD_TAG UINT64_C(0x2026112300030004)
#endif
#define main qwen35_scope_test_unused_main
#include "pioneer_qwen35_dataflow_role_haiku_scope_test.cpp"
#undef main

#define B_DEVICE_OP_CODES_END 10000
constexpr status_t B_BAD_VALUE=-2,B_NOT_ALLOWED=-3,B_DEV_NOT_READY=-4;
struct tt_resident_session_result {};
static bool gPrepareFail=false,gLostOutput=false;
static unsigned gPrepareCalls=0,gRetains=0,gCopies=0;
static bool PioneerQ35SharedRoleCopiesCaptured(const PioneerQwen35SharedRoleBinding*,unsigned) { return false; }
static bool PioneerQ38DispatchUnsafe(const FakeInspectionDispatch& dispatch) { return dispatch.unsafe; }
static bool PrepareQ38Dispatch(PioneerStoreContext& c) {
    ++gPrepareCalls;c.qwen35DedicatedFleet=true;c.q38Dispatch=&c.dispatch;
    if (gPrepareFail) { c.dispatch.unsafe=true;return false; }
    c.q38DispatchRoutes=true;c.dispatch.prepared=120;return true;
}
static void RetainQ38DispatchOwner(PioneerStoreContext& c,tt_resident_session_result&) {
    ++gRetains;c.dispatchOwnerRetained=true;sColdRecoveryRequired=true;
}
static status_t acquire_sem(int32_t lock) { assert(lock==sLock && !gLocked);gLocked=true;return B_OK; }
static status_t user_memcpy(void* target,const void* source,size_t bytes) {
    ++gCopies;if (gLostOutput && gCopies==2) return B_BAD_VALUE;
    std::memcpy(target,source,bytes);return B_OK;
}
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35FleetInspectionControl.inc"

struct Fixture {
    PioneerStoreContext context;
    PioneerProbeCookie cookie;
    tt_q35_fleet_inspection request{};
    Fixture() {
        sColdRecoveryRequired=sReloadQuiesced=gLocked=gPrepareFail=gLostOutput=false;
        sOpenCookies=1;sReloadGeneration=42;gPrepareCalls=gRetains=gCopies=0;
        cookie.context=&context;sRuntimeOwner=sQwen35ExclusiveCookie=&cookie;
        context.io.runtimeCore=7;context.io.simulateArc=context.io.simulateHdma=true;
        context.io.firmwareMatches=true;
        request.version=1;request.expected_tag=PIONEER_DRIVER_BUILD_TAG;request.expected_generation=42;
    }
    status_t Run() { return Qwen35FleetInspectionControl(&cookie,&request,sizeof(request)); }
};

#ifndef TEST_ROLE_STAGE_QUALIFY
int main()
{
    (void)&Qwen35DataflowGenerate;
#ifdef PIONEER_QWEN35_RISC_RESET_SAMPLE_QUALIFY
    {
        Fixture f;tt_q35_risc_reset_sample r{};r.version=1;r.expected_tag=PIONEER_DRIVER_BUILD_TAG;r.expected_generation=42;
        assert(Qwen35RiscResetSampleControl(&f.cookie,&r,sizeof(r))==B_OK && r.operation_status==B_NOT_ALLOWED);
        assert(f.Run()==B_OK && f.request.operation_status==B_OK);gCopies=0;
        r={};r.version=1;r.expected_tag=PIONEER_DRIVER_BUILD_TAG;r.expected_generation=42;
        assert(Qwen35RiscResetSampleControl(&f.cookie,&r,sizeof(r))==B_OK && r.operation_status==B_OK
            && r.sampled && !r.restore_failed && r.build_tag==PIONEER_DRIVER_BUILD_TAG && r.generation==42);
        for (unsigned i=0;i<8;i++) assert(r.words[i]==UINT32_MAX-i);
        assert(!gRetains && !sColdRecoveryRequired);
        r={};r.version=1;r.expected_tag=PIONEER_DRIVER_BUILD_TAG;r.expected_generation=42;gCopies=0;
        assert(Qwen35RiscResetSampleControl(&f.cookie,&r,sizeof(r))==B_OK && r.operation_status==B_NOT_ALLOWED);
    }
    {
        Fixture f;assert(f.Run()==B_OK && f.request.operation_status==B_OK);gCopies=0;
        f.context.io.riscDropWriteAt=f.context.io.tlbWrites+1;
        tt_q35_risc_reset_sample r{};r.version=1;r.expected_tag=PIONEER_DRIVER_BUILD_TAG;r.expected_generation=42;
        assert(Qwen35RiscResetSampleControl(&f.cookie,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
            && !r.sampled && r.restore_failed);
        assert(f.context.dispatchOwnerRetained && sColdRecoveryRequired && gRetains==1);
        const unsigned writes=f.context.io.tlbWrites;
        r={};r.version=1;r.expected_tag=PIONEER_DRIVER_BUILD_TAG;r.expected_generation=42;gCopies=0;
        assert(Qwen35RiscResetSampleControl(&f.cookie,&r,sizeof(r))==B_OK && r.operation_status==B_NOT_ALLOWED);
        assert(f.context.io.tlbWrites==writes && gRetains==1);
    }
#endif
    {
        Fixture f;assert(f.Run()==B_OK && f.request.operation_status==B_OK);
        assert(f.request.stage==TT_Q35_FLEET_INSPECTION_COMPLETE && f.request.failed_core==120
            && f.request.prepared==120 && f.request.l2cpu_reset==0x0f && !f.request.restore_failed);
        assert(f.context.io.runtimeCore==7 && !sColdRecoveryRequired && gPrepareCalls==1 && !gRetains);
        f.request={};f.request.version=1;f.request.expected_tag=PIONEER_DRIVER_BUILD_TAG;f.request.expected_generation=42;
        assert(f.Run()==B_OK && f.request.operation_status==B_NOT_ALLOWED && gPrepareCalls==1);
    }
    {
        Fixture f;f.request.expected_generation=41;
        assert(f.Run()==B_OK && f.request.operation_status==B_BAD_VALUE && !gPrepareCalls);
    }
    {
        Fixture f;f.context.uploadedPages=1;
        assert(f.Run()==B_OK && f.request.operation_status==B_NOT_ALLOWED && !gPrepareCalls);
    }
    {
        Fixture f;sOpenCookies=2;
        assert(f.Run()==B_OK && f.request.operation_status==B_NOT_ALLOWED && !gPrepareCalls);
    }
    {
        Fixture f;gPrepareFail=true;
        assert(f.Run()==B_OK && f.request.operation_status==B_DEV_NOT_READY);
        assert(f.context.dispatchOwnerRetained && sColdRecoveryRequired && gRetains==1);
    }
    {
        Fixture f;f.context.io.resetChangeAt=0;
        assert(f.Run()==B_OK && f.request.operation_status==B_DEV_NOT_READY);
        assert(f.request.stage==PioneerQwen35FleetInspectionResult::HeldFleet
            && !f.request.restore_failed && !sColdRecoveryRequired && !gRetains);
    }
    {
        Fixture f;f.context.io.dropHdmaRestore=true;
        assert(f.Run()==B_OK && f.request.operation_status==B_DEV_NOT_READY);
        assert(f.request.restore_failed && f.context.dispatchOwnerRetained && sColdRecoveryRequired && gRetains==1);
    }
    {
        Fixture f;gLostOutput=true;
        assert(f.Run()==B_BAD_VALUE && f.context.qwen35FleetInspectionAttempted
            && f.context.q38Dispatch && !sColdRecoveryRequired);
    }
    std::puts("PASS restricted held-fleet control, admission, one-shot, failed restore and lost reply; SIMULATED ONLY");
}
#endif
