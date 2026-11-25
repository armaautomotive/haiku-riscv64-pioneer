/* SPDX-License-Identifier: MIT */
// Exercise the actual fabric finalizer with a real software guard state and
// simulated context, PCI reservation, scope and destructor; no card access.
#define PIONEER_QWEN35_FABRIC_GUARD_WITNESS 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_RESIDENT_SERVICE_WITNESS 1
#define PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS 1
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadState.h"
using uint64=uint64_t;using status_t=int;
constexpr int B_OK=0,B_NOT_ALLOWED=-1,B_DEV_NOT_READY=-2;
constexpr unsigned PIONEER_RECOVERY_STAGE_OWNER_RELEASE=10,PIONEER_RECOVERY_STAGE_UNRESERVE=11,
    PIONEER_RECOVERY_STAGE_GUARD_COMPLETE=12,PIONEER_RECOVERY_STAGE_COMPLETE=13;
struct tt_pioneer_recovery_run { unsigned stage=0;uint64 new_generation=0; };
static PioneerDriverReloadState state;
static unsigned checks=0,failAt=0,unreserves=0,destructions=0,completions=0;
static bool failUnreserve=false,failComplete=false,zeroGeneration=false,destructorFault=false;
static uint32_t epoch=3;
static status_t Begin(uint64,uint64*) { return B_OK; }
static status_t Abort(uint64) { return B_OK; }
static status_t Validate(uint64 ticket) {
    if (++checks==failAt) return B_DEV_NOT_READY;
    return state.CanResumeRecovery(ticket,epoch)?B_OK:B_DEV_NOT_READY;
}
static status_t OtherValidate(uint64 ticket) { return Validate(ticket); }
static status_t Complete(uint64 ticket,const PioneerDriverFabricRecoveryProof* proof,uint64* generation) {
    ++completions;if (failComplete) return B_DEV_NOT_READY;
    uint32_t begun=0;uint64 value=0;
    if (!state.CompleteFabricRecovery(ticket,epoch,*proof,begun,value)) return B_DEV_NOT_READY;
    *generation=zeroGeneration?0:value;return B_OK;
}
struct pioneer_driver_recovery_guard_v2_info {
    unsigned version=2;status_t (*begin)(uint64,uint64*)=Begin;
    status_t (*complete)(uint64,const PioneerDriverRecoveryProof*,uint64*)=nullptr;
    status_t (*abort)(uint64)=Abort;status_t (*validate)(uint64)=Validate;
};
struct pioneer_driver_fabric_recovery_guard_info {
    unsigned version=1;status_t (*begin)(uint64,uint64*)=Begin;
    status_t (*complete)(uint64,const PioneerDriverFabricRecoveryProof*,uint64*)=Complete;
    status_t (*abort)(uint64)=Abort;status_t (*validate)(uint64)=Validate;
};
struct ReloadGuard { bool cold() const { return state.cold; } };
static ReloadGuard reloadGuard;static ReloadGuard* sReloadGuard=&reloadGuard;
static uint64 sReloadGeneration=0;static bool sBankStorageOnly=true;
struct ColdLatch {
    bool value=true;
    void operator=(bool fault) { if (fault) { value=true;state.Fault(sReloadGeneration); } }
    void ClearAfterVerifiedRecovery() { value=false; }
} static sColdRecoveryRequired;
struct Graph { bool ready=true;bool FabricContextRetirementReady(uint64 ticket,bool=false) const { return ready && ticket==1; } };
struct Qualification { bool Active() const { return false; } };
struct PioneerStoreContext {
    Graph* qwen35GraphOwner=nullptr;void* qwen35SharedRoleBinding=nullptr;void* recoveryFreshSession=nullptr;
    void* recoveryMapping=nullptr;void* recoveryFabricArcOwner=nullptr;
    bool recoveryFabricFinishAttempted=false,recoveryFabricRoleOwnerReleaseAttempted=true,recoveryFabricRoleOwnerReleased=true;
    bool recoveryFabricGraphRetirementAttempted=true,recoveryFabricGraphRetirementRecorded=true,recoveryFabricEvidenceRecorded=true;
    bool q35PciReservationHeld=true,q35PciReleaseAttempted=false;
    PioneerDriverFabricRecoveryProof recoveryFabricEvidence{};Qualification qwen35QualificationOwner;
    ~PioneerStoreContext() { ++destructions;if (destructorFault) sColdRecoveryRequired=true; }
};
struct PioneerProbeCookie { uint32_t magic=0x50313030;PioneerStoreContext* context=nullptr; };
static PioneerProbeCookie* sRuntimeOwner=nullptr;
struct PioneerQ35FabricHostScope {
    PioneerProbeCookie* inspector=nullptr;PioneerProbeCookie* owner=nullptr;
    bool MatchesParties(const PioneerProbeCookie* requestedInspector,const PioneerProbeCookie* requestedOwner) const {
        return requestedInspector==inspector && requestedOwner==owner;
    }
    bool OwnsAfterAuditRelease(const PioneerStoreContext& c,const pioneer_driver_recovery_guard_v2_info* g,uint64 ticket) const {
        return c.q35PciReservationHeld && !c.q35PciReleaseAttempted && g->validate(ticket)==B_OK;
    }
    bool OwnsAfterReservationRelease(const PioneerStoreContext& c,const pioneer_driver_recovery_guard_v2_info* g,uint64 ticket) const {
        return !c.q35PciReservationHeld && c.q35PciReleaseAttempted && g->validate(ticket)==B_OK;
    }
};
static status_t Qwen35ReleasePciReservation(PioneerStoreContext& c) {
    ++unreserves;c.q35PciReleaseAttempted=true;c.q35PciReservationHeld=false;
    return failUnreserve?B_DEV_NOT_READY:B_OK;
}
struct Scope {
    bool admission=true,after=true,changeProof=false;
    bool FabricFinishAdmission(PioneerProbeCookie&,PioneerProbeCookie&,PioneerStoreContext&,uint64) { return admission; }
    bool FabricFinishAfterUnreserve(PioneerProbeCookie&,PioneerProbeCookie&,PioneerStoreContext& c,uint64) {
        if (changeProof) c.recoveryFabricEvidence.host_dma_active=1;
        return after;
    }
};
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35FabricFinish.inc"
struct Fixture {
    Graph graph;PioneerProbeCookie cookie;PioneerStoreContext* context;PioneerQ35FabricHostScope host;
    pioneer_driver_recovery_guard_v2_info validation;pioneer_driver_fabric_recovery_guard_info fabric;
    Scope scope;tt_pioneer_recovery_run result;uint64 ticket=0;
    Fixture() {
        state={};assert(state.Attach(sReloadGeneration));state.Fault(sReloadGeneration);
        epoch=3;assert(state.BeginRecovery(sReloadGeneration,epoch,ticket) && ticket==1);
        checks=failAt=unreserves=destructions=completions=0;
        failUnreserve=failComplete=zeroGeneration=destructorFault=false;sColdRecoveryRequired.value=true;sBankStorageOnly=true;
        context=new PioneerStoreContext;context->qwen35GraphOwner=&graph;cookie.context=context;sRuntimeOwner=&cookie;
        host.inspector=host.owner=&cookie;
        auto& p=context->recoveryFabricEvidence;
        p.flags=PIONEER_FABRIC_PROOF_REQUIRED & ~(PIONEER_FABRIC_PROOF_OWNERS_RELEASED|PIONEER_FABRIC_PROOF_PCI_RESERVATION_RELEASED);
        p.core_count=120;p.retained_owners=1;p.fabric_generation=p.firmware_generation=ticket;
    }
    status_t Run() { return PioneerQ35FabricFinishLocked(&cookie,&cookie,context,host,&validation,&fabric,ticket,scope,result); }
    ~Fixture() { if (cookie.context) { delete context;cookie.context=nullptr; } }
};
int main() {
    unsigned totalChecks=0;
    for (unsigned failure=0;;failure++) {
        if (failure && failure>totalChecks) break;
        Fixture f;failAt=failure;const status_t status=f.Run();
        if (!failure) {
            assert(status==B_OK && !f.cookie.context && !sRuntimeOwner && !sBankStorageOnly
                && !state.cold && !sColdRecoveryRequired.value && destructions==1 && unreserves==1
                && completions==1 && f.result.stage==PIONEER_RECOVERY_STAGE_COMPLETE && f.result.new_generation==2);
            totalChecks=checks;assert(totalChecks>=6);
        } else {
            assert(status!=B_OK && f.cookie.context==f.context && state.cold && sColdRecoveryRequired.value
                && !destructions && f.result.stage!=PIONEER_RECOVERY_STAGE_COMPLETE);
            if (unreserves) {
                assert(f.context->recoveryFabricFinishAttempted && f.context->q35PciReleaseAttempted);
                failAt=0;assert(f.Run()==B_NOT_ALLOWED && unreserves==1);
            }
        }
    }
    for (unsigned failure=0;failure<5;failure++) {
        Fixture f;
        if (failure==0) f.scope.admission=false;
        if (failure==1) failUnreserve=true;
        if (failure==2) f.scope.after=false;
        if (failure==3) f.scope.changeProof=true;
        if (failure==4) failComplete=true;
        assert(f.Run()!=B_OK && f.cookie.context==f.context && state.cold && sColdRecoveryRequired.value
            && !destructions && f.result.stage!=PIONEER_RECOVERY_STAGE_COMPLETE);
        assert(unreserves==(failure?1U:0U));
        if (failure) assert(f.Run()==B_NOT_ALLOWED && unreserves==1);
    }
    {
        Fixture f;PioneerProbeCookie wrong;f.host.inspector=&wrong;
        assert(f.Run()==B_NOT_ALLOWED && !unreserves && !completions && f.cookie.context==f.context);
    }
    {
        Fixture f;f.fabric.validate=OtherValidate;
        assert(f.Run()==B_NOT_ALLOWED && !unreserves && !completions && f.cookie.context==f.context);
    }
    {
        Fixture f;auto* oldOwner=new PioneerProbeCookie;oldOwner->context=f.context;
        f.cookie.context=nullptr;sRuntimeOwner=oldOwner;f.host.owner=oldOwner;
        assert(PioneerQ35FabricFinishLocked(&f.cookie,oldOwner,f.context,f.host,&f.validation,&f.fabric,
            f.ticket,f.scope,f.result)==B_OK && !sRuntimeOwner && !state.cold && destructions==1);
    }
    {
        Fixture f;zeroGeneration=true;
        assert(f.Run()==B_DEV_NOT_READY && state.cold && sColdRecoveryRequired.value
            && f.cookie.context==f.context && unreserves==1 && completions==1 && !destructions);
    }
    {
        Fixture f;destructorFault=true;
        assert(f.Run()==B_DEV_NOT_READY && state.cold && sColdRecoveryRequired.value
            && !f.cookie.context && !sRuntimeOwner && destructions==1 && unreserves==1 && completions==1
            && f.result.stage!=PIONEER_RECOVERY_STAGE_COMPLETE);
    }
    std::printf("PASS fabric finish: %u simulated ticket-loss boundaries, one-shot unreserve and destructor fault; NO HARDWARE\n",totalChecks);
}
