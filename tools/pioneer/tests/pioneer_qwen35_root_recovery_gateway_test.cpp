/* Distributed under the MIT License. */
#include <cassert>
#include <cstdio>
using status_t=int;
constexpr status_t B_OK=0,B_ERROR=-1,B_NOT_ALLOWED=-2;
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPciProfile.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPciRootLeaseScope.h"
struct PioneerProbeCookie {};
struct tt_pioneer_recovery_run {};
struct Context { tt_pioneer_profile profile{}; };
struct Owner { Context* context; };
static Owner* sRuntimeOwner=nullptr;
static void* sDeviceManager=(void*)1;
static int sCard=0;
static bool sColdRecoveryRequired=false,nodeHeld=false;
static int calls=0,moduleRefs=0,mode=0;
using Operation=status_t (*)(void*);
struct pioneer_sg2042_recovery_module_info { status_t (*run)(void*,Operation,void*,status_t*); };
using module_info=pioneer_sg2042_recovery_module_info;
#define PIONEER_SG2042_RECOVERY_MODULE "test"
struct PioneerRootLeaseCompileQuarantine { void Quarantine() { sColdRecoveryRequired=true; } };
static status_t Run(void* cookie,Operation operation,void* argument,status_t* status) {
    assert(cookie==(void*)2 && nodeHeld && moduleRefs==1);
    if (mode==1) return B_NOT_ALLOWED;
    *status=operation(argument);
    if (mode==2) return B_ERROR;
    if (mode==3) (void)operation(argument);
    return B_OK;
}
static pioneer_sg2042_recovery_module_info service{Run};
static status_t get_module(const char*,module_info** result) {
    if (mode==4) return B_ERROR;
    *result=&service;++moduleRefs;return B_OK;
}
static void put_module(const char*) { assert(moduleRefs==1);--moduleRefs; }
static status_t Qwen35RecoveryRunLocked(PioneerProbeCookie*,tt_pioneer_recovery_run&) {
    assert(nodeHeld && moduleRefs==1);++calls;return B_ERROR;
}
static status_t WithPioneerPciController(void*,int,tt_pioneer_profile& profile,
        status_t (*operation)(void*,void*),void* argument) {
    profile=sRuntimeOwner->context->profile;
    if (mode==5) ++profile.root_bus;
    nodeHeld=true;
    status_t result=operation((void*)2,argument);
    nodeHeld=false;
    return result;
}
#include "pioneer_q35_root_recovery_gateway.inc"
int main() {
    Context context;context.profile.ready=1;context.profile.card_bus=1;
    context.profile.root_bus=0;context.profile.controller=0x7062000000ULL;
    context.profile.config=0x4800000000ULL;context.profile.config_length=0x1000;
    context.profile.bar0_host_start=0x4900000000ULL;context.profile.bar0_host_end=0x491fffffffULL;
    context.profile.bar2_host_start=0x4920000000ULL;context.profile.bar2_host_end=0x49200fffffULL;
    Owner owner{&context};sRuntimeOwner=&owner;
    PioneerProbeCookie inspector;tt_pioneer_recovery_run request{};
    assert(Qwen35RecoveryWithRootLease(&inspector,request)==B_ERROR && calls==1 && !sColdRecoveryRequired);
    mode=1;assert(Qwen35RecoveryWithRootLease(&inspector,request)==B_NOT_ALLOWED && calls==1 && !sColdRecoveryRequired);
    mode=2;assert(Qwen35RecoveryWithRootLease(&inspector,request)==B_ERROR && calls==2 && sColdRecoveryRequired);
    sColdRecoveryRequired=false;mode=3;assert(Qwen35RecoveryWithRootLease(&inspector,request)==B_ERROR && calls==3 && sColdRecoveryRequired);
    sColdRecoveryRequired=false;mode=4;assert(Qwen35RecoveryWithRootLease(&inspector,request)==B_ERROR && calls==3 && !sColdRecoveryRequired);
    mode=5;assert(Qwen35RecoveryWithRootLease(&inspector,request)==B_NOT_ALLOWED && calls==3 && !sColdRecoveryRequired);
    sRuntimeOwner=nullptr;assert(Qwen35RecoveryWithRootLease(&inspector,request)==B_NOT_ALLOWED && calls==3);
    assert(!nodeHeld && moduleRefs==0);
    std::puts("PASS root recovery gateway: controller identity, callback status and ambiguous lease quarantine; simulated");
}
