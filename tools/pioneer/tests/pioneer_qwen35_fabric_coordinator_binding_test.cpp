/* SPDX-License-Identifier: MIT */
// Exercise the actual coordinator bindings with simulated owner operations.
#define PIONEER_QWEN35_FABRIC_GUARD_WITNESS 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_RESIDENT_SERVICE_WITNESS 1
#define PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS 1
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <new>
using uint64=uint64_t;using status_t=int;
enum { B_OK=0,B_NOT_ALLOWED=-1,B_NO_MEMORY=-2,B_DEV_NOT_READY=-3 };
static unsigned stage=0,failStage=0,checks=0,failCheck=0;
static bool sColdRecoveryRequired=false,finished=false;
static bool Step(unsigned next) { stage=next;return next!=failStage; }
static status_t Begin(uint64,uint64*) { return B_OK; }
static status_t Abort(uint64) { return B_OK; }
static status_t Validate(uint64 ticket) { return ticket==7 && ++checks!=failCheck?B_OK:B_DEV_NOT_READY; }
struct pioneer_driver_recovery_guard_v2_info { unsigned version=2;status_t (*begin)(uint64,uint64*)=Begin;
    status_t (*abort)(uint64)=Abort;status_t (*validate)(uint64)=Validate; };
struct pioneer_driver_fabric_recovery_guard_info { unsigned version=1;status_t (*begin)(uint64,uint64*)=Begin;
    status_t (*abort)(uint64)=Abort;status_t (*validate)(uint64)=Validate;void (*complete)()=nullptr; };
static void Complete() {}
struct PioneerStoreContext;
struct PioneerProbeCookie { PioneerStoreContext* context=nullptr; };
struct PioneerQ35FreshRecoverySession;
struct PioneerStoreContext { PioneerQ35FreshRecoverySession* recoveryFreshSession=nullptr;
    int* qwen35SharedRoleBinding=nullptr;int* qwen35GraphOwner=nullptr; };
struct PioneerQ35FabricHostScope {
    PioneerProbeCookie* inspector;PioneerProbeCookie* owner;
    bool MatchesParties(PioneerProbeCookie* a,PioneerProbeCookie* b) const { return a==inspector && b==owner; }
    bool OwnsAfterAuditRelease(PioneerStoreContext& c,pioneer_driver_recovery_guard_v2_info* g,uint64 ticket) const {
        return owner->context==&c && g->validate(ticket)==B_OK;
    }
};
struct Scope {
    bool allowed=true,retained=false;
    bool FabricCoordinatorAdmission(PioneerProbeCookie&,PioneerProbeCookie&,PioneerStoreContext&,uint64) { return allowed; }
    bool FabricCoordinatorAccessAllowed(PioneerProbeCookie&,PioneerProbeCookie&,PioneerStoreContext&,uint64) { return allowed; }
};
struct PioneerQ35FreshRecoverySession {
    PioneerStoreContext* old=nullptr;bool retained=false;
    static PioneerQ35FreshRecoverySession* Allocate() { return new(std::nothrow) PioneerQ35FreshRecoverySession; }
    bool PrepareFabricRole(PioneerProbeCookie*,PioneerStoreContext& c,pioneer_driver_recovery_guard_v2_info*,Scope&) {
        old=&c;c.recoveryFreshSession=this;return Step(1);
    }
    bool DiscardUnstarted() { if (old) return false;delete this;return true; }
    void RetainUncertain(Scope& scope) { retained=true;scope.retained=true;sColdRecoveryRequired=true; }
    uint64 Ticket() const { return 7; }
    bool RoleReady() const { return true; }
    bool RetireRoleSession(pioneer_driver_recovery_guard_v2_info*,Scope&) {
        if (!Step(3)) return false;
        old->recoveryFreshSession=nullptr;delete this;return true;
    }
};
struct RoleScope {};struct ParentScope {};struct Bar {};struct FinishScope {};struct tt_pioneer_recovery_run {};
template<class Session,class Context,class Guard,class A,class B,class C,class D>
static bool PioneerQ35RestoreFabricRoleSequence(Session&,Context&,Guard*,A&,B&,C&,D&,unsigned) { return Step(2); }
template<class Check> static bool PioneerQ35ReleaseFabricRoleOwner(PioneerStoreContext& c,uint64,Check& check) {
    if (!check() || !Step(4)) return false;
    c.qwen35SharedRoleBinding=nullptr;return check();
}
template<class Check> static bool PioneerQ35RecordFabricGraphRetirement(PioneerStoreContext&,uint64,Check& check) {
    return check() && Step(5) && check();
}
template<class Finish> static status_t PioneerQ35FabricFinishLocked(PioneerProbeCookie*,PioneerProbeCookie*,
    PioneerStoreContext*,PioneerQ35FabricHostScope&,pioneer_driver_recovery_guard_v2_info*,
    pioneer_driver_fabric_recovery_guard_info*,uint64,Finish&,tt_pioneer_recovery_run&) {
    if (!Step(6)) return B_DEV_NOT_READY;
    finished=true;return B_OK;
}
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35FabricRecoveryCoordinator.inc"
int main() {
    for (unsigned failure=0;failure<=6;failure++) {
        stage=failCheck=checks=0;failStage=failure;finished=sColdRecoveryRequired=false;
        int role=1,graph=2;PioneerStoreContext context{nullptr,&role,&graph};PioneerProbeCookie cookie{&context};
        PioneerQ35FabricHostScope host{&cookie,&cookie};pioneer_driver_recovery_guard_v2_info validation;
        pioneer_driver_fabric_recovery_guard_info fabric;fabric.complete=Complete;
        Scope scope;RoleScope rs;ParentScope ps;Bar bar;FinishScope fs;tt_pioneer_recovery_run result;
        const status_t status=PioneerQ35ResumeFabricRoleRecoveryLocked(&cookie,&cookie,&context,host,&validation,
            &fabric,7,5,scope,rs,ps,bar,fs,result);
        if (!failure) assert(status==B_OK && finished && !context.recoveryFreshSession && !context.qwen35SharedRoleBinding);
        else {
            assert(status==B_DEV_NOT_READY && !finished && stage==failure);
            if (failure<=3) assert(context.recoveryFreshSession && scope.retained && sColdRecoveryRequired);
            else assert(!context.recoveryFreshSession && (failure==6 || sColdRecoveryRequired));
        }
        delete context.recoveryFreshSession;
    }
    unsigned totalChecks=0;
    for (unsigned failure=0;;failure++) {
        if (failure && failure>totalChecks) break;
        stage=checks=failStage=0;failCheck=failure;finished=sColdRecoveryRequired=false;
        int role=1,graph=2;PioneerStoreContext context{nullptr,&role,&graph};PioneerProbeCookie cookie{&context};
        PioneerQ35FabricHostScope host{&cookie,&cookie};pioneer_driver_recovery_guard_v2_info validation;
        pioneer_driver_fabric_recovery_guard_info fabric;fabric.complete=Complete;
        Scope scope;RoleScope rs;ParentScope ps;Bar bar;FinishScope fs;tt_pioneer_recovery_run result;
        const status_t status=PioneerQ35ResumeFabricRoleRecoveryLocked(&cookie,&cookie,&context,host,&validation,
            &fabric,7,5,scope,rs,ps,bar,fs,result);
        if (!failure) { assert(status==B_OK && finished);totalChecks=checks;assert(totalChecks>10); }
        else {
            assert(status!=B_OK && !finished);
            if (context.recoveryFreshSession) assert(scope.retained && sColdRecoveryRequired);
            else if (stage>=3) assert(sColdRecoveryRequired);
        }
        delete context.recoveryFreshSession;
    }
    std::printf("PASS actual fabric coordinator bindings: stages and %u simulated ticket-loss checks; NO HARDWARE\n",totalChecks);
}
