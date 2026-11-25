/* SPDX-License-Identifier: MIT */
// Real retained-start controller with simulated mapping and fabric operations.
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS 1
#define PIONEER_QWEN35_RECOVERY_RUN_ENABLE 1
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <new>
using uint64=uint64_t;using thread_id=int;using team_id=int;using status_t=int;
enum { B_OK=0,B_NOT_ALLOWED=-1,B_NO_MEMORY=-2,B_DEV_NOT_READY=-3,B_DEVICE_OP_CODES_END=1000 };
#include "../../../src/add-ons/kernel/drivers/misc/PioneerRecoveryProtocol.h"
namespace PioneerResidentBanks { constexpr uint64 Total=4096; }
struct PioneerStoreContext;
struct PioneerProbeCookie { uint32_t magic=0x50313030;team_id team=20;PioneerStoreContext* context=nullptr;bool finalFreeRetained=false; };
static PioneerProbeCookie* sRuntimeOwner=nullptr;static PioneerProbeCookie* sQwen35ExclusiveCookie=nullptr;
static unsigned sOpenCookies=1;static bool sReloadQuiesced=false,sBankStorageOnly=true,sColdRecoveryRequired=true;
static int currentThread=10;static thread_id find_thread(const char*) { return currentThread; }
static bool ticketValid=true;static unsigned touches=0;static int failAt=0;
static status_t Validate(uint64 ticket) { return ticket==123 && ticketValid?B_OK:B_NOT_ALLOWED; }
static status_t Begin(uint64,uint64* ticket) { *ticket=123;return B_OK; }
static status_t Abort(uint64) { return B_OK; }
struct pioneer_driver_recovery_guard_v2_info {
    unsigned version=2;status_t (*validate)(uint64)=Validate;
    status_t (*begin)(uint64,uint64*)=Begin;status_t (*abort)(uint64)=Abort;
};
struct tt_arc_scratch_result { unsigned version=1; };
struct tt_arc_queue_result {};struct PioneerPostResetAuditResult {};struct PioneerQ35PostResetL2Result {};
struct PioneerPostResetMapping {
    bool q35CloseComplete=false;
    explicit PioneerPostResetMapping(int) {}
    ~PioneerPostResetMapping() { if (failAt==8) ticketValid=false; }
    template<class Check> bool OpenQ35Guarded(Check& check) {
        if (!check()) return false;
        ++touches;return failAt!=1 && check();
    }
    template<class Check> bool ProbeQ35FabricFirmwareGuarded(unsigned limit,tt_arc_scratch_result&,Check& check) {
        assert(limit==100);if (!check()) return false;
        ++touches;return failAt!=2 && check();
    }
};
struct PioneerQ35FabricArcCompileOwner {
    enum Phase { Retired };
    Phase CurrentPhase() const { return Retired; }
    ~PioneerQ35FabricArcCompileOwner() { if (failAt==7) ticketValid=false; }
};
struct PioneerStoreContext {
    int profile=0;struct {
        void* qwen35BorrowedRoleScope=nullptr;bool fabricOldMmioDisposeAttempted=false,fabricOldMmioDisposeComplete=false;
        template<class Check> status_t DisposeFabricOldMmio(Check& check) {
            if (!check()) return B_DEV_NOT_READY;
            fabricOldMmioDisposeAttempted=true;++touches;
            if (failAt==6 || !check()) return B_DEV_NOT_READY;
            fabricOldMmioDisposeComplete=true;return B_OK;
        }
    } io;
    bool recoveryFabricStartAttempted=false,recoveryAttempted=false,dispatchOwnerRetained=true;
    bool recoveryFabricPciIdleAttempted=false,recoveryFabricPciIdle=false;
    bool recoveryFabricAuditReleaseAttempted=false,recoveryFabricAuditReleased=false;
    bool q35PciReservationHeld=true,q35PciReleaseAttempted=false,bankStorageOnly=true;
    uint64 bytes=PioneerResidentBanks::Total,recoveryFabricStartTicket=0;
    PioneerPostResetMapping* recoveryMapping=nullptr;PioneerQ35FabricArcCompileOwner* recoveryFabricArcOwner=nullptr;void* recoveryFreshSession=nullptr;
    int Pci() const { return 0; }
    template<class Scope> bool RunQ35FabricRecovery(pioneer_driver_recovery_guard_v2_info*,uint64,
        const tt_arc_scratch_result&,tt_arc_queue_result&,PioneerPostResetAuditResult&,
        PioneerQ35PostResetL2Result&,Scope& scope,unsigned& stage) {
        if (!scope.HostOwnershipValid()) return false;
        ++touches;stage=failAt==3?8:17;
        recoveryFabricArcOwner=new PioneerQ35FabricArcCompileOwner;
        recoveryMapping->q35CloseComplete=failAt!=3;return failAt!=3;
    }
    ~PioneerStoreContext() { delete recoveryFabricArcOwner;delete recoveryMapping; }
};
struct PioneerQ35FabricHostScope {
    PioneerProbeCookie* inspector;PioneerProbeCookie* owner;PioneerStoreContext* context;
    pioneer_driver_recovery_guard_v2_info* guard;uint64 ticket;
    PioneerQ35FabricHostScope(PioneerProbeCookie* i,PioneerProbeCookie* o,PioneerStoreContext* c,
        pioneer_driver_recovery_guard_v2_info* g,uint64 t,thread_id,team_id)
        :inspector(i),owner(o),context(c),guard(g),ticket(t) {}
    bool HostOwnershipValid() const {
        return context && context->recoveryMapping && sRuntimeOwner==owner
            && sQwen35ExclusiveCookie==inspector && guard->validate(ticket)==B_OK;
    }
    bool HostOwnershipValidAfterAuditRelease() const {
        return context && !context->recoveryMapping && !context->recoveryFabricArcOwner
            && sRuntimeOwner==owner && sQwen35ExclusiveCookie==inspector && guard->validate(ticket)==B_OK;
    }
};
template<class IO,class Check> static bool PioneerPreResetDisableMasterGuarded(IO&,Check& check) {
    return check() && failAt!=4;
}
template<class IO,class Saved,class Check> static bool PioneerPostResetPciFinishGuarded(IO&,const Saved&,Check& check,unsigned& stage) {
    if (!check()) return false;
    ++touches;stage=failAt==5?2:3;return failAt!=5 && check();
}
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35FabricRecoveryStart.inc"
struct Fixture {
    PioneerStoreContext context;PioneerProbeCookie cookie;pioneer_driver_recovery_guard_v2_info guard;
    tt_pioneer_recovery_run result{};
    Fixture() {
        touches=0;failAt=0;ticketValid=true;currentThread=10;sOpenCookies=1;
        sReloadQuiesced=false;sBankStorageOnly=sColdRecoveryRequired=true;
        cookie.context=&context;sRuntimeOwner=sQwen35ExclusiveCookie=&cookie;
    }
    status_t Run() { return Qwen35FabricRecoveryStartLocked(&cookie,&cookie,&context,&guard,123,10,20,result); }
};
int main() {
    {
        Fixture f;assert(f.Run()==B_DEV_NOT_READY && f.result.stage==PIONEER_RECOVERY_STAGE_FABRIC_AUDIT_RELEASE
            && f.result.detail==PIONEER_RECOVERY_DETAIL_FABRIC_AUDIT_RELEASED && touches==5
            && f.context.recoveryAttempted && f.context.recoveryFabricStartTicket==123
            && !f.context.recoveryMapping && !f.context.recoveryFabricArcOwner && sColdRecoveryRequired
            && f.context.recoveryFabricPciIdleAttempted && f.context.recoveryFabricPciIdle
            && f.context.io.fabricOldMmioDisposeComplete && f.context.recoveryFabricAuditReleased);
        assert(f.Run()==B_NOT_ALLOWED && touches==5);
    }
    for (int failure=1;failure<=3;failure++) {
        Fixture f;failAt=failure;assert(f.Run()==B_DEV_NOT_READY && touches==unsigned(failure)
            && f.context.recoveryFabricStartAttempted && f.context.recoveryFabricStartTicket==123
            && f.context.recoveryMapping && sColdRecoveryRequired);
        const unsigned before=touches;assert(f.Run()==B_NOT_ALLOWED && touches==before);
        if (failure==3) assert(f.result.detail==8 && f.context.recoveryFabricArcOwner);
    }
    {
        Fixture f;failAt=4;
        assert(f.Run()==B_DEV_NOT_READY && f.result.stage==PIONEER_RECOVERY_STAGE_DISABLE_MASTER
            && !touches && f.context.recoveryFabricStartAttempted && f.context.recoveryMapping
            && f.context.recoveryFabricStartTicket==123 && sColdRecoveryRequired);
        assert(f.Run()==B_NOT_ALLOWED && !touches);
    }
    {
        Fixture f;failAt=5;
        assert(f.Run()==B_DEV_NOT_READY && f.result.stage==PIONEER_RECOVERY_STAGE_FABRIC_PCI_IDLE
            && f.result.detail==2 && touches==4 && f.context.recoveryFabricPciIdleAttempted
            && !f.context.recoveryFabricPciIdle && f.context.recoveryFabricStartTicket==123
            && f.context.recoveryMapping && f.context.recoveryFabricArcOwner && sColdRecoveryRequired);
        assert(f.Run()==B_NOT_ALLOWED && touches==4);
    }
    {
        Fixture f;failAt=6;
        assert(f.Run()==B_DEV_NOT_READY && f.result.stage==PIONEER_RECOVERY_STAGE_FABRIC_OLD_MMIO
            && touches==5 && f.context.recoveryFabricPciIdle && f.context.io.fabricOldMmioDisposeAttempted
            && !f.context.io.fabricOldMmioDisposeComplete && f.context.recoveryMapping
            && f.context.recoveryFabricArcOwner && sColdRecoveryRequired);
        assert(f.Run()==B_NOT_ALLOWED && touches==5);
    }
    for (int failure=7;failure<=8;failure++) {
        Fixture f;failAt=failure;
        assert(f.Run()==B_DEV_NOT_READY && f.result.stage==PIONEER_RECOVERY_STAGE_FABRIC_AUDIT_RELEASE
            && f.context.recoveryFabricAuditReleaseAttempted && !f.context.recoveryFabricAuditReleased
            && !f.context.recoveryFabricArcOwner && (f.context.recoveryMapping!=nullptr)==(failure==7)
            && !ticketValid && sColdRecoveryRequired);
        ticketValid=true;assert(f.Run()==B_NOT_ALLOWED);
        failAt=0;
    }
    for (unsigned denied=0;denied<7;denied++) {
        Fixture f;
        if (denied==0) ticketValid=false;
        if (denied==1) f.context.q35PciReservationHeld=false;
        if (denied==2) f.context.io.qwen35BorrowedRoleScope=&f;
        if (denied==3) currentThread=11;
        if (denied==4) sQwen35ExclusiveCookie=nullptr;
        if (denied==5) f.context.recoveryFreshSession=&f;
        if (denied==6) f.context.recoveryFabricStartTicket=99;
        assert(f.Run()==B_NOT_ALLOWED && !touches && !f.context.recoveryFabricStartAttempted
            && !f.context.recoveryMapping && f.context.recoveryFabricStartTicket==(denied==6?99U:0U));
    }
    std::puts("PASS retained fabric controller admission, one-shot preparation and incomplete recovery; SIMULATED ONLY");
    return 0;
}
