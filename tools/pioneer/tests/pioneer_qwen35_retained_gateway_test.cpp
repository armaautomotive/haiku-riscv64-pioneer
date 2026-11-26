/* SPDX-License-Identifier: MIT */
// Actual driver gateway, actual pin service and actual controller lease on fake nodes.
#define main retained_scope_fixture_main
#include "pioneer_retained_recovery_scope_test.cpp"
#undef main
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPciProfile.h"

#define PIONEER_DEVICE_MANAGER_RETAINED_PIN_MODULE "test/pins"
#define PIONEER_DRIVER_RECOVERY_GUARD_V2_MODULE "test/guard"
#define PIONEER_DRIVER_BUILD_TAG UINT64_C(0x1234)
enum {PIONEER_RECOVERY_STAGE_GUARD_COMPLETE=11,PIONEER_RECOVERY_STAGE_COMPLETE=12};
using PioneerQ35RetainedPinAdapter=ModulePin;
using PioneerQ35RetainedControllerAdapter=ModuleController;
struct pioneer_driver_recovery_guard_v2_info {int version;status_t (*validate)(uint64);};
static int pinRefs=0,controllerRefs=0,guardRefs=0;
static bool pinAvailable=true,profileMismatch=false,allowProof=false;
static status_t Validate(uint64 ticket) {return ticket==99?B_OK:B_NOT_ALLOWED;}
static pioneer_driver_recovery_guard_v2_info fakeGuard{2,Validate};
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
    bool recoveryFinishCompleted=false;
    uint64 recoveryPrepareTicket=0,recoveryResetTicket=0,recoveryFirmwareTicket=0;
    uint64 recoveryMappingTicket=0,recoveryFinishTicket=0,recoveryFinishCompletedGeneration=0;
};
struct PioneerProbeCookie {PioneerStoreContext* context=nullptr;};
struct tt_pioneer_recovery_run {uint64 expected_tag=0,expected_generation=0,new_generation=0;unsigned stage=0;};
static PioneerProbeCookie* sRuntimeOwner=nullptr;
static bool sColdRecoveryRequired=true;
static uint64 sReloadGeneration=3;
static int fakeManager=0,*sDeviceManager=&fakeManager,sCard=0;
static device_node* fakeCard=nullptr;static device_node* fakeRoot=nullptr;
static ECAMPCIController* fakeController=nullptr;
using NodeCallback=status_t (*)(void*,device_node*,device_node*,void*);
static status_t WithPioneerPciControllerNodes(int*,int,tt_pioneer_profile& profile,NodeCallback callback,void* argument) {
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
    if (recoveryCalls==1) {context.recoveryResetPending=true;context.recoveryResetTicket=99;return B_DEV_NOT_READY;}
    context.recoveryResetPending=false;context.recoveryFinishPending=true;context.recoveryFinishTicket=99;
    context.recoveryFinishCompleted=true;context.recoveryFinishCompletedGeneration=4;
    request.stage=PIONEER_RECOVERY_STAGE_GUARD_COMPLETE;return B_OK;
}
static bool Qwen35RetainedHardwareRetirementProof(PioneerStoreContext*,uint64 controllerTicket,uint64 resetTicket) {
    return allowProof && controllerTicket!=0 && resetTicket==99;
}
static status_t Qwen35FinalizeRetainedRecoveryLocked(PioneerProbeCookie* owner,PioneerStoreContext* context,
    tt_pioneer_recovery_run& request,bool retired) {
    if (!retired || owner!=sRuntimeOwner || owner->context!=context || !context->recoveryFinishCompleted)
        return B_NOT_ALLOWED;
    sReloadGeneration=context->recoveryFinishCompletedGeneration;owner->context=nullptr;sRuntimeOwner=nullptr;
    sColdRecoveryRequired=false;request.stage=PIONEER_RECOVERY_STAGE_COMPLETE;
    request.new_generation=sReloadGeneration;return B_OK;
}
#include "pioneer_q35_retained_gateway.inc"

int main() {
    assert(retained_scope_fixture_main()==0);
    current=10;device_node root,card;card.parent=&root;assert(card.InitDriver()==B_OK);
    ECAMPCIController controller;fakeCard=&card;fakeRoot=&root;fakeController=&controller;
    PioneerStoreContext context;context.profile.ready=1;context.profile.card_bus=17;
    context.profile.root_bus=16;context.profile.controller=100;context.profile.config=200;
    context.profile.config_length=4096;context.profile.bar0_host_start=300;
    context.profile.bar0_host_end=400;context.profile.bar2_host_start=500;context.profile.bar2_host_end=600;
    PioneerProbeCookie owner{&context},inspector;sRuntimeOwner=&owner;
    tt_pioneer_recovery_run request{PIONEER_DRIVER_BUILD_TAG,3,0,0};
    assert(!Qwen35RetainedRetryable(&context));
    context.recoveryAttempted=true;context.recoveryResetPending=true;context.recoveryResetTicket=99;
    assert(Qwen35RetainedRetryable(&context)==99 && !guardRefs);
    context.recoveryResetTicket=100;assert(!Qwen35RetainedRetryable(&context) && !guardRefs);
    context.recoveryResetTicket=99;context.recoveryMappingPending=true;context.recoveryMappingTicket=99;
    assert(!Qwen35RetainedRetryable(&context) && !guardRefs);
    context.recoveryMappingPending=false;context.recoveryFinishCompleted=true;
    assert(!Qwen35RetainedRetryable(&context) && !guardRefs);
    context.recoveryFinishCompleted=false;context.recoveryResetPending=false;
    context.recoveryResetTicket=0;context.recoveryMappingTicket=0;context.recoveryAttempted=false;
    uint64 retainedTicket=0;
    assert(!Qwen35RetainedTicketContinuity(retainedTicket,0) && retainedTicket==0);
    assert(Qwen35RetainedTicketContinuity(retainedTicket,99) && retainedTicket==99);
    assert(Qwen35RetainedTicketContinuity(retainedTicket,99));
    assert(!Qwen35RetainedTicketContinuity(retainedTicket,100) && retainedTicket==99);
    admissionAllowed=false;
    assert(Qwen35RecoveryWithRetainedRootLease(&inspector,request)==B_BUSY);
    assert(!sQwen35RetainedGateway && !pinRefs && !controllerRefs);
    admissionAllowed=true;
    context.recoveryAttempted=true;
    assert(Qwen35RecoveryWithRetainedRootLease(&inspector,request)==B_NOT_ALLOWED);
    assert(!sQwen35RetainedGateway && !pinRefs && !controllerRefs);
    context.recoveryAttempted=false;context.recoveryResetPending=true;
    assert(Qwen35RecoveryWithRetainedRootLease(&inspector,request)==B_NOT_ALLOWED);
    assert(!sQwen35RetainedGateway && !pinRefs && !controllerRefs);
    context.recoveryResetPending=false;
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
    assert(Qwen35RecoveryWithRetainedRootLease(&inspector,request)==B_DEV_NOT_READY);
    assert(recoveryCalls==1 && controller.fPioneerRecoveryLease.Parked() && pinRefs==1 && controllerRefs==1);
    assert(card.fInitialized==2 && root.fInitialized==3 && !guardRefs);
    current=11;assert(Access(controller)==B_DEV_NOT_READY);
    request={PIONEER_DRIVER_BUILD_TAG,4,0,0};
    assert(Qwen35RecoveryWithRetainedRootLease(&inspector,request)==B_NOT_ALLOWED);
    assert(recoveryCalls==1 && controller.fPioneerRecoveryLease.Parked());
    request={PIONEER_DRIVER_BUILD_TAG,3,0,0};
    assert(Qwen35RecoveryWithRetainedRootLease(&inspector,request)==B_DEV_NOT_READY);
    assert(recoveryCalls==2 && request.stage==PIONEER_RECOVERY_STAGE_GUARD_COMPLETE
        && sRuntimeOwner==&owner && sColdRecoveryRequired && controller.fPioneerRecoveryLease.Parked());
    current=12;request={PIONEER_DRIVER_BUILD_TAG,3,0,0};
    assert(Qwen35RecoveryWithRetainedRootLease(&inspector,request)==B_DEV_NOT_READY && recoveryCalls==2);
    allowProof=true;request={PIONEER_DRIVER_BUILD_TAG,3,0,0};
    assert(Qwen35RecoveryWithRetainedRootLease(&inspector,request)==B_OK);
    assert(recoveryCalls==2 && request.stage==PIONEER_RECOVERY_STAGE_COMPLETE && request.new_generation==4);
    assert(!sQwen35RetainedGateway && !sRuntimeOwner && !sColdRecoveryRequired
        && !controller.fPioneerRecoveryLease.Active() && !pinRefs && !controllerRefs && !guardRefs);
    assert(card.fInitialized==1 && root.fInitialized==1 && card.UninitDriver() && modules==0);
    std::puts("PASS actual retained recovery gateway retries and proof-gated retirement; SIMULATED ONLY");
}
