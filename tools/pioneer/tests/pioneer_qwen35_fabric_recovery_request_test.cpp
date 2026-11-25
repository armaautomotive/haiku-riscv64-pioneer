/* SPDX-License-Identifier: MIT */
// Exercise the actual ioctl request wrapper around the retained-start owner.
#define main fabric_start_existing_test_main
#include "pioneer_qwen35_fabric_recovery_start_test.cpp"
#undef main
#include <cstring>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35FleetInspectionProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35RoleStageProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35ProjectionQualificationProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35RiscResetProtocol.h"
static_assert(TT_PCI_PROBE_RECOVERY_FABRIC!=TT_PCI_PROBE_Q35_FLEET_INSPECT
    && TT_PCI_PROBE_RECOVERY_FABRIC!=TT_PCI_PROBE_Q35_ROLE_STAGE_QUALIFY
    && TT_PCI_PROBE_RECOVERY_FABRIC!=TT_PCI_PROBE_Q35_PROJECTION_QUALIFY
    && TT_PCI_PROBE_RECOVERY_FABRIC!=TT_PCI_PROBE_Q35_RISC_RESET_SAMPLE,"fabric ioctl must be distinct");
struct thread_info { team_id team; };
static status_t get_thread_info(thread_id id,thread_info* info) { if (id!=10) return B_NOT_ALLOWED;info->team=20;return B_OK; }
static bool locked=false;static status_t releaseResult=B_OK;
static status_t acquire_sem(int) { if (locked) return B_NOT_ALLOWED;locked=true;return B_OK; }
static status_t release_sem(int) { assert(locked);locked=false;return releaseResult; }
static status_t user_memcpy(void* to,const void* from,size_t length) { std::memcpy(to,from,length);return B_OK; }
using module_info=unsigned;
static int sLock=1;static bool sReloadGuard=true;static uint64 sReloadGeneration=7;
static constexpr uint64 PIONEER_DRIVER_BUILD_TAG=0x1234;
static constexpr const char* PIONEER_DRIVER_RECOVERY_GUARD_V2_MODULE="guard";
static pioneer_driver_recovery_guard_v2_info activeGuard;
static unsigned begins=0,aborts=0,modulePuts=0;static bool invalidateOnBegin=false;
static status_t abortResult=B_OK;
static status_t BeginTracked(uint64 generation,uint64* ticket) {
    ++begins;if (generation!=7) return B_NOT_ALLOWED;
    *ticket=123;if (invalidateOnBegin) ticketValid=false;return B_OK;
}
static status_t AbortTracked(uint64 ticket) { assert(ticket==123);++aborts;return abortResult; }
static status_t get_module(const char*,module_info** out) {
    activeGuard.begin=BeginTracked;activeGuard.abort=AbortTracked;
    *out=reinterpret_cast<module_info*>(&activeGuard);return B_OK;
}
static status_t put_module(const char*) { ++modulePuts;return B_OK; }
enum { B_BAD_VALUE=-4 };
#include "pioneer_q35_fabric_request.inc"
static tt_pioneer_recovery_run Request() {
    tt_pioneer_recovery_run r{};r.version=1;r.expected_tag=PIONEER_DRIVER_BUILD_TAG;r.expected_generation=7;return r;
}
static void ResetCounters() { begins=aborts=modulePuts=0;invalidateOnBegin=false;abortResult=releaseResult=B_OK;locked=false;sReloadGuard=true;sReloadGeneration=7; }
int main() {
    {
        Fixture f;ResetCounters();auto r=Request();
        assert(Qwen35FabricRecoveryRequest(&f.cookie,&r,sizeof(r))==B_OK
            && r.operation_status==B_DEV_NOT_READY && r.detail==PIONEER_RECOVERY_DETAIL_FABRIC_AUDITED
            && r.build_tag==PIONEER_DRIVER_BUILD_TAG && !r.new_generation
            && begins==1 && !aborts && modulePuts==1 && touches==3 && !locked);
        auto again=Request();assert(Qwen35FabricRecoveryRequest(&f.cookie,&again,sizeof(again))==B_OK
            && again.operation_status==B_NOT_ALLOWED && begins==1 && touches==3 && !locked);
    }
    {
        Fixture f;ResetCounters();auto r=Request();r.expected_tag++;
        assert(Qwen35FabricRecoveryRequest(&f.cookie,&r,sizeof(r))==B_OK
            && r.operation_status==B_BAD_VALUE && !begins && !touches && !locked);
    }
    {
        Fixture f;ResetCounters();auto r=Request();invalidateOnBegin=true;
        assert(Qwen35FabricRecoveryRequest(&f.cookie,&r,sizeof(r))==B_OK
            && r.operation_status==B_NOT_ALLOWED && begins==1 && aborts==1 && modulePuts==1
            && !touches && !f.context.recoveryFabricStartAttempted && !locked);
    }
    {
        Fixture f;ResetCounters();auto r=Request();failAt=2;
        assert(Qwen35FabricRecoveryRequest(&f.cookie,&r,sizeof(r))==B_OK
            && r.operation_status==B_DEV_NOT_READY && begins==1 && !aborts && modulePuts==1
            && touches==2 && f.context.recoveryFabricStartTicket==123 && sColdRecoveryRequired && !locked);
    }
    {
        Fixture f;ResetCounters();auto r=Request();invalidateOnBegin=true;abortResult=-9;
        assert(Qwen35FabricRecoveryRequest(&f.cookie,&r,sizeof(r))==B_OK
            && r.operation_status==-9 && begins==1 && aborts==1 && modulePuts==1 && !touches && !locked);
    }
    {
        Fixture f;ResetCounters();auto r=Request();r.expected_tag++;sColdRecoveryRequired=false;releaseResult=-8;
        assert(Qwen35FabricRecoveryRequest(&f.cookie,&r,sizeof(r))==-8
            && sColdRecoveryRequired && !begins && !touches && !locked);
    }
    std::puts("PASS actual fabric recovery ioctl admission, ticket retention and incomplete result; SIMULATED ONLY");
}
