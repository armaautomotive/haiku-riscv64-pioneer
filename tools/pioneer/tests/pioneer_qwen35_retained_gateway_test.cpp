/* SPDX-License-Identifier: MIT */
// Actual driver gateway, actual pin service and actual controller lease on fake nodes.
#define main retained_scope_fixture_main
#include "pioneer_retained_recovery_scope_test.cpp"
#undef main
#include <cstdlib>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPciProfile.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadState.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPcieLinkReset.h"

#define PIONEER_DEVICE_MANAGER_RETAINED_PIN_MODULE "test/pins"
#define PIONEER_DRIVER_RECOVERY_GUARD_V2_MODULE "test/guard"
#define PIONEER_DRIVER_BUILD_TAG UINT64_C(0x1234)
enum {PIONEER_RECOVERY_STAGE_GUARD_COMPLETE=11,PIONEER_RECOVERY_STAGE_COMPLETE=12,
    PIONEER_RECOVERY_STAGE_UNRESERVE=13};
using PioneerQ35RetainedPinAdapter=ModulePin;
using PioneerQ35RetainedControllerAdapter=ModuleController;
struct pioneer_driver_recovery_guard_v2_info {
    int version;status_t (*validate)(uint64);
    status_t (*complete)(uint64,const PioneerDriverRecoveryProof*,uint64*);
};
static int pinRefs=0,controllerRefs=0,guardRefs=0;
static bool pinAvailable=true,profileMismatch=false,allowProof=false,fakeGuardCold=true;
static int completeCalls=0,unreserveCalls=0;
static status_t Validate(uint64 ticket) {return ticket==99 && fakeGuardCold?B_OK:B_NOT_ALLOWED;}
static status_t Complete(uint64 ticket,const PioneerDriverRecoveryProof* proof,uint64* generation) {
    assert(ticket==99 && fakeGuardCold && proof->Complete());++completeCalls;
    fakeGuardCold=false;*generation=4;return B_OK;
}
static pioneer_driver_recovery_guard_v2_info fakeGuard{2,Validate,Complete};
static status_t GatewayGetModule(const char* name,module_info** out) {
    if (!std::strcmp(name,PIONEER_DEVICE_MANAGER_RETAINED_PIN_MODULE)) {
        if (!pinAvailable) return B_ERROR;
        ++pinRefs;*out=reinterpret_cast<module_info*>(&pinService);return B_OK;
    }
    if (!std::strcmp(name,PIONEER_SG2042_RETAINED_LEASE_MODULE)) {
        ++controllerRefs;*out=reinterpret_cast<module_info*>(&gPioneerRetainedLease);return B_OK;
    }
    if (!std::strcmp(name,PIONEER_DRIVER_RECOVERY_GUARD_V2_MODULE)) {
        ++guardRefs;*out=reinterpret_cast<module_info*>(&fakeGuard);return B_OK;
    }
    return B_ERROR;
}
static void GatewayPutModule(const char* name) {
    if (!std::strcmp(name,PIONEER_DEVICE_MANAGER_RETAINED_PIN_MODULE)) {assert(pinRefs>0);--pinRefs;return;}
    if (!std::strcmp(name,PIONEER_SG2042_RETAINED_LEASE_MODULE)) {assert(controllerRefs>0);--controllerRefs;return;}
    if (!std::strcmp(name,PIONEER_DRIVER_RECOVERY_GUARD_V2_MODULE)) {assert(guardRefs>0);--guardRefs;return;}
    assert(false);
}
struct PioneerStoreContext {
    tt_pioneer_profile profile{};
    bool recoveryAttempted=false,recoveryPreparePending=false,recoveryResetPending=false;
    bool recoveryFirmwarePending=false,recoveryMappingPending=false,recoveryFinishPending=false;
    bool recoveryFinishReady=false,recoveryFinishKeepOwnerCookie=false,recoveryFinishUnreserveAttempted=false;
    bool q35PciReservationHeld=true,q35PciReleaseAttempted=false;
    uint64 recoveryPrepareTicket=0,recoveryResetTicket=0,recoveryFirmwareTicket=0;
    uint64 recoveryMappingTicket=0,recoveryFinishTicket=0;
    PioneerDriverRecoveryProof recoveryFinishProof{};
};
struct PioneerProbeCookie {PioneerStoreContext* context=nullptr;int magic=42;};
struct tt_pioneer_recovery_run {uint64 expected_tag=0,expected_generation=0,new_generation=0;unsigned stage=0;};
static PioneerProbeCookie* sRuntimeOwner=nullptr;
struct ColdLatch {
    bool value=true;operator bool() const {return value || fakeGuardCold;}
    void operator=(bool v){if(v){value=true;fakeGuardCold=true;}}
    void ClearAfterVerifiedRecovery(){value=false;}
};
static ColdLatch sColdRecoveryRequired;
static uint64 sReloadGeneration=3;
static bool sBankStorageOnly=true;
static int fakeManager=0,*sDeviceManager=&fakeManager,sCookie=0;
struct FakeCard {int bus=0,device=0,function=0;} sCard;
static device_node* fakeCard=nullptr;static device_node* fakeRoot=nullptr;
static ECAMPCIController* fakeController=nullptr;
static unsigned simulatedLinkWrites=0;
static bool failSimulatedLinkAssert=false;
struct RetainedLinkIO {
    uint32_t control=3;
    bool Allowed() const {
        return fakeController && fakeController->fPioneerRecoveryLease.Active()
            && !fakeController->fPioneerRecoveryLease.Parked()
            && fakeCard && fakeRoot && fakeCard->fInitialized>1 && fakeRoot->fInitialized>1;
    }
    uint32_t RootRead(uint16_t offset,uint8_t) {
        assert(Allowed());
        if (offset==0) return 0x20421e30;
        if (offset==8) return 0x06040000;
        if (offset==0x18) return 0x003f0100;
        if (offset==4) return 0x402;
        if (offset==0x3e) return control;
        assert(false);return 0;
    }
    uint32_t CardRead(uint16_t offset,uint8_t) {
        assert(Allowed());
        if (offset==0) return 0xb1401e52;
        if (offset==0x2c) return 0x00431e52;
        if (offset==4) return 0x402;
        assert(false);return 0;
    }
    void RootWrite(uint16_t offset,uint8_t size,uint32_t value) {
        assert(Allowed() && offset==0x3e && size==2);++simulatedLinkWrites;
        control=failSimulatedLinkAssert && (value&0x40U)?value&~0x40U:value;
    }
    bool DelayUs(int64_t) {return Allowed();}
    bool RootLinkReady() {return Allowed();}
};
#define TT_PCI_PROBE_DEVICE "test/pci"
struct FakePCI {status_t unreserve_device(int,int,int,const char*,void*) {
    assert(fakeGuardCold && !fakeController->fPioneerRecoveryLease.Active());
    assert(fakeCard->fInitialized==1 && fakeRoot->fInitialized==1);
    ++unreserveCalls;return B_OK;
}};
static FakePCI pci,*sPCI=&pci;
using NodeCallback=status_t (*)(void*,device_node*,device_node*,void*);
static status_t WithPioneerPciControllerNodes(int*,const FakeCard&,tt_pioneer_profile& profile,NodeCallback callback,void* argument) {
    profile=sRuntimeOwner->context->profile;
    if (profileMismatch) ++profile.controller;
    return callback(fakeController,fakeCard,fakeRoot,argument);
}
static int recoveryCalls=0;
static bool admissionAllowed=true;
static status_t Qwen35RecoverySelectOwner(PioneerProbeCookie*,const tt_pioneer_recovery_run&,
    PioneerProbeCookie*& owner,PioneerStoreContext*& context,bool& live) {
    if (!admissionAllowed) return B_BUSY;
    owner=sRuntimeOwner;context=owner->context;live=false;return B_OK;
}
static status_t Qwen35RecoveryRunLocked(PioneerProbeCookie*,tt_pioneer_recovery_run& request) {
    auto& context=*sRuntimeOwner->context;++recoveryCalls;context.recoveryAttempted=true;
    if (recoveryCalls==1) {
        RetainedLinkIO io;PioneerPcieLinkReset<RetainedLinkIO> link;
        const bool okay=link.Run(io,101);assert(simulatedLinkWrites==2);
        if (!okay) return B_DEV_NOT_READY;
        context.recoveryResetPending=true;context.recoveryResetTicket=99;return B_DEV_NOT_READY;
    }
    assert(simulatedLinkWrites==2);
    context.recoveryResetPending=false;context.recoveryFinishPending=true;context.recoveryFinishTicket=99;
    context.recoveryFinishReady=true;
    auto& proof=context.recoveryFinishProof;
    proof.flags=PIONEER_RECOVERY_PROOF_REQUIRED & ~PIONEER_RECOVERY_PROOF_RETAINED_OWNERS_RELEASED;
    proof.core_count=120;proof.retained_owners=1;proof.reset_generation=proof.firmware_generation=99;
    request.stage=PIONEER_RECOVERY_STAGE_GUARD_COMPLETE;return B_OK;
}
static bool Qwen35RetainedHardwareRetirementProof(PioneerStoreContext*,uint64 controllerTicket,uint64 resetTicket) {
    return allowProof && controllerTicket!=0 && resetTicket==99;
}
#include "pioneer_q35_retained_reservation.inc"
#include "pioneer_q35_retained_finish.inc"
#include "pioneer_q35_retained_gateway.inc"

int main(int argc,char** argv) {
    assert(retained_scope_fixture_main()==0);
    current=10;device_node root,card;card.parent=&root;assert(card.InitDriver()==B_OK);
    ECAMPCIController controller;fakeCard=&card;fakeRoot=&root;fakeController=&controller;
    auto* context=new PioneerStoreContext;context->profile.ready=1;context->profile.card_bus=17;
    context->profile.root_bus=16;context->profile.controller=100;context->profile.config=200;
    context->profile.config_length=4096;context->profile.bar0_host_start=300;
    context->profile.bar0_host_end=400;context->profile.bar2_host_start=500;context->profile.bar2_host_end=600;
    auto* owner=new PioneerProbeCookie{context};PioneerProbeCookie inspector;sRuntimeOwner=owner;
    tt_pioneer_recovery_run request{PIONEER_DRIVER_BUILD_TAG,3,0,0};
    assert(!Qwen35RetainedRetryable(context));
    context->recoveryAttempted=true;context->recoveryResetPending=true;context->recoveryResetTicket=99;
    assert(Qwen35RetainedRetryable(context)==99 && !guardRefs);
    context->recoveryResetTicket=100;assert(!Qwen35RetainedRetryable(context) && !guardRefs);
    context->recoveryResetTicket=99;context->recoveryMappingPending=true;context->recoveryMappingTicket=99;
    assert(!Qwen35RetainedRetryable(context) && !guardRefs);
    context->recoveryMappingPending=false;context->recoveryFinishReady=true;
    assert(!Qwen35RetainedRetryable(context) && !guardRefs);
    context->recoveryFinishReady=false;context->recoveryResetPending=false;
    context->recoveryResetTicket=0;context->recoveryMappingTicket=0;context->recoveryAttempted=false;
    uint64 retainedTicket=0;
    assert(!Qwen35RetainedTicketContinuity(retainedTicket,0) && retainedTicket==0);
    assert(Qwen35RetainedTicketContinuity(retainedTicket,99) && retainedTicket==99);
    assert(Qwen35RetainedTicketContinuity(retainedTicket,99));
    assert(!Qwen35RetainedTicketContinuity(retainedTicket,100) && retainedTicket==99);
    admissionAllowed=false;
    assert(Qwen35RecoveryWithRetainedRootLease(&inspector,request)==B_BUSY);
    assert(!sQwen35RetainedGateway && !pinRefs && !controllerRefs);
    admissionAllowed=true;
    context->q35PciReservationHeld=false;
    assert(Qwen35RecoveryWithRetainedRootLease(&inspector,request)==B_NOT_ALLOWED);
    assert(!sQwen35RetainedGateway && !pinRefs && !controllerRefs);
    context->q35PciReservationHeld=true;context->q35PciReleaseAttempted=true;
    assert(Qwen35RecoveryWithRetainedRootLease(&inspector,request)==B_NOT_ALLOWED);
    assert(!sQwen35RetainedGateway && !pinRefs && !controllerRefs);
    context->q35PciReleaseAttempted=false;
    context->recoveryAttempted=true;
    assert(Qwen35RecoveryWithRetainedRootLease(&inspector,request)==B_NOT_ALLOWED);
    assert(!sQwen35RetainedGateway && !pinRefs && !controllerRefs);
    context->recoveryAttempted=false;context->recoveryResetPending=true;
    assert(Qwen35RecoveryWithRetainedRootLease(&inspector,request)==B_NOT_ALLOWED);
    assert(!sQwen35RetainedGateway && !pinRefs && !controllerRefs);
    context->recoveryResetPending=false;
    pinAvailable=false;
    assert(Qwen35RecoveryWithRetainedRootLease(&inspector,request)==B_ERROR);
    assert(!sQwen35RetainedGateway && !pinRefs && !controllerRefs && card.fInitialized==1);
    pinAvailable=true;profileMismatch=true;
    assert(Qwen35RecoveryWithRetainedRootLease(&inspector,request)==B_NOT_ALLOWED);
    assert(!sQwen35RetainedGateway && !pinRefs && !controllerRefs && card.fInitialized==1);
    profileMismatch=false;
    const auto acquire=pinService.acquire;
    pinService.acquire=[](device_node* node,const void* identity,uint64 session,uint64* id) -> status_t {
        if (node==fakeRoot) return B_NO_MEMORY;
        return pioneer_acquire_retained_pin(node,identity,session,id);
    };
    assert(Qwen35RecoveryWithRetainedRootLease(&inspector,request)==B_NO_MEMORY);
    assert(!sQwen35RetainedGateway && !pinRefs && !controllerRefs
        && card.fInitialized==1 && root.fInitialized==1);
    pinService.acquire=acquire;
    failSimulatedLinkAssert=argc>1 && !std::strcmp(argv[1],"link-fault");
    assert(Qwen35RecoveryWithRetainedRootLease(&inspector,request)==B_DEV_NOT_READY);
    assert(recoveryCalls==1 && simulatedLinkWrites==2 && controller.fPioneerRecoveryLease.Parked() && pinRefs==1 && controllerRefs==1);
    assert(card.fInitialized==2 && root.fInitialized==3 && !guardRefs);
    current=11;assert(Access(controller)==B_DEV_NOT_READY);
    if (failSimulatedLinkAssert) {
        request={PIONEER_DRIVER_BUILD_TAG,3,0,0};
        assert(Qwen35RecoveryWithRetainedRootLease(&inspector,request)==B_NOT_ALLOWED);
        assert(recoveryCalls==1 && simulatedLinkWrites==2 && controller.fPioneerRecoveryLease.Parked()
            && pinRefs==1 && controllerRefs==1 && sColdRecoveryRequired);
        std::puts("PASS failed link pulse retains the parked gateway without replay; SIMULATED ONLY");
        std::fflush(stdout);std::_Exit(0);
    }
    if (argc>1) {
        // Intentionally leave simulated ownership retained after the fault;
        // this subprocess exits without pretending to release an unsafe lease.
        context->q35PciReservationHeld=false;
        request={PIONEER_DRIVER_BUILD_TAG,3,0,0};
        assert(Qwen35RecoveryWithRetainedRootLease(&inspector,request)==B_NOT_ALLOWED);
        context->q35PciReservationHeld=true;
        assert(Qwen35RecoveryWithRetainedRootLease(&inspector,request)==B_NOT_ALLOWED);
        assert(recoveryCalls==1 && controller.fPioneerRecoveryLease.Parked()
            && pinRefs==1 && controllerRefs==1 && sColdRecoveryRequired);
        std::puts("PASS lost reservation retains the parked gateway without retry; SIMULATED ONLY");
        std::fflush(stdout);std::_Exit(0);
    }
    request={PIONEER_DRIVER_BUILD_TAG,4,0,0};
    assert(Qwen35RecoveryWithRetainedRootLease(&inspector,request)==B_NOT_ALLOWED);
    assert(recoveryCalls==1 && controller.fPioneerRecoveryLease.Parked());
    request={PIONEER_DRIVER_BUILD_TAG,3,0,0};
    assert(Qwen35RecoveryWithRetainedRootLease(&inspector,request)==B_DEV_NOT_READY);
    assert(recoveryCalls==2 && request.stage==PIONEER_RECOVERY_STAGE_GUARD_COMPLETE
        && sRuntimeOwner==owner && sColdRecoveryRequired && controller.fPioneerRecoveryLease.Parked()
        && fakeGuardCold && !unreserveCalls && !completeCalls);
    current=12;request={PIONEER_DRIVER_BUILD_TAG,3,0,0};
    assert(Qwen35RecoveryWithRetainedRootLease(&inspector,request)==B_DEV_NOT_READY && recoveryCalls==2);
    allowProof=true;request={PIONEER_DRIVER_BUILD_TAG,3,0,0};
    assert(Qwen35RecoveryWithRetainedRootLease(&inspector,request)==B_OK);
    assert(recoveryCalls==2 && simulatedLinkWrites==2 && request.stage==PIONEER_RECOVERY_STAGE_COMPLETE && request.new_generation==4);
    assert(!sQwen35RetainedGateway && !sRuntimeOwner && !sColdRecoveryRequired
        && !fakeGuardCold && !sBankStorageOnly && unreserveCalls==1 && completeCalls==1
        && !controller.fPioneerRecoveryLease.Active() && !pinRefs && !controllerRefs && !guardRefs);
    assert(card.fInitialized==1 && root.fInitialized==1 && card.UninitDriver() && modules==0);
    std::puts("PASS actual retained recovery gateway retries and proof-gated retirement; SIMULATED ONLY");
}
