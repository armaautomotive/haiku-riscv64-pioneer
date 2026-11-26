/* SPDX-License-Identifier: MIT */
// Real persistent guard and real deferred finalizer with simulated PCI nodes.
#include "../../../src/add-ons/kernel/drivers/misc/pioneer_p100a_reload_guard.cpp"
#include <cassert>
#include <cstdint>
#include <cstdio>
enum {PIONEER_RECOVERY_STAGE_GUARD_COMPLETE=11,PIONEER_RECOVERY_STAGE_COMPLETE=12,
 PIONEER_RECOVERY_STAGE_UNRESERVE=13};
static int contextsDestroyed=0,cookiesDestroyed=0,completeCalls=0,unreserveCalls=0;
static bool faultOnDestroy=false;
static void FaultOnDestroy();
struct PioneerStoreContext {
 uint64 recoveryFinishTicket=0;PioneerDriverRecoveryProof recoveryFinishProof{};
 bool recoveryFinishReady=false,recoveryFinishPending=false,recoveryFinishKeepOwnerCookie=false;
 bool recoveryFinishUnreserveAttempted=false;
 bool q35PciReservationHeld=true,q35PciReleaseAttempted=false;
 ~PioneerStoreContext() {++contextsDestroyed;if(faultOnDestroy)FaultOnDestroy();}
};
struct PioneerProbeCookie {
 PioneerStoreContext* context=nullptr;int magic=42;
 ~PioneerProbeCookie() {assert(magic==0);++cookiesDestroyed;}
};
struct tt_pioneer_recovery_run {unsigned stage=0;uint64 new_generation=0;};
static uint64 sReloadGeneration=0;
static void FaultOnDestroy() {sGuard.fault(sReloadGeneration);}
struct ColdLatch {
 bool value=true;operator bool() const {return value || sGuard.cold();}
 void operator=(bool v){if(v){value=true;sGuard.fault(sReloadGeneration);}}
 void ClearAfterVerifiedRecovery(){value=false;}
};
static ColdLatch sColdRecoveryRequired;
static PioneerProbeCookie* sRuntimeOwner=nullptr;
static bool sBankStorageOnly=true,failValidation=false,failComplete=false,failUnreserve=false,faultOnUnreserve=false;
static status_t Validate(uint64 ticket) {
 return failValidation?B_DEV_NOT_READY:sRecoveryGuardV2.validate(ticket);
}
static status_t Complete(uint64 ticket,const PioneerDriverRecoveryProof* proof,uint64* out) {
 ++completeCalls;assert(proof->Complete());
 return failComplete?B_DEV_NOT_READY:sRecoveryGuardV2.complete(ticket,proof,out);
}
static pioneer_driver_recovery_guard_v2_info guard;
struct FakeCard {int bus=0,device=0,function=0;} sCard;
static int sCookie;
#define TT_PCI_PROBE_DEVICE "test/pci"
struct FakePCI {status_t unreserve_device(int,int,int,const char*,void*) {
 ++unreserveCalls;if(faultOnUnreserve)sGuard.fault(sReloadGeneration);
 return failUnreserve?B_DEV_NOT_READY:B_OK;
}};
static FakePCI pci,*sPCI=&pci;
static status_t get_module(const char*,module_info** out){*out=&guard.info;return B_OK;}
static void put_module(const char*){}
static uint64 Start(PioneerStoreContext& c) {
 sState=PioneerDriverReloadState{};sFaultState=0;sReloadGeneration=0;
 assert(sGuard.attach(&sReloadGeneration)==B_OK && sReloadGeneration==1);
 sGuard.fault(sReloadGeneration);uint64 ticket=0;
 assert(sRecoveryGuardV2.begin(sReloadGeneration,&ticket)==B_OK && ticket);
 c.recoveryFinishReady=true;c.recoveryFinishPending=true;c.recoveryFinishTicket=ticket;
 auto& p=c.recoveryFinishProof;p.flags=PIONEER_RECOVERY_PROOF_REQUIRED
  & ~PIONEER_RECOVERY_PROOF_RETAINED_OWNERS_RELEASED;
 p.core_count=120;p.retained_owners=1;p.reset_generation=p.firmware_generation=ticket;
 sColdRecoveryRequired.value=true;sBankStorageOnly=true;return ticket;
}
#include "pioneer_q35_retained_reservation.inc"
#include "pioneer_q35_retained_finish.inc"
int main() {
 assert(sGuard.info.std_ops(B_MODULE_INIT)==B_OK);
 guard=sRecoveryGuardV2;guard.validate=Validate;guard.complete=Complete;
 auto* owner=new PioneerProbeCookie;auto* context=new PioneerStoreContext;
 owner->context=context;sRuntimeOwner=owner;uint64 ticket=Start(*context);
 tt_pioneer_recovery_run request{PIONEER_RECOVERY_STAGE_GUARD_COMPLETE,0};
 assert(Qwen35FinalizeRetainedRecoveryLocked(owner,context,request,false)==B_NOT_ALLOWED);
 assert(sRuntimeOwner==owner && Cold() && sRecoveryGuardV2.validate(ticket)==B_OK
  && !completeCalls && !unreserveCalls);
 context->q35PciReservationHeld=false;
 assert(Qwen35FinalizeRetainedRecoveryLocked(owner,context,request,true)==B_NOT_ALLOWED && !unreserveCalls);
 context->q35PciReservationHeld=true;context->q35PciReleaseAttempted=true;
 assert(Qwen35FinalizeRetainedRecoveryLocked(owner,context,request,true)==B_NOT_ALLOWED && !unreserveCalls);
 context->q35PciReleaseAttempted=false;
 request.stage=0;
 assert(Qwen35FinalizeRetainedRecoveryLocked(owner,context,request,true)==B_NOT_ALLOWED);
 request.stage=PIONEER_RECOVERY_STAGE_GUARD_COMPLETE;failValidation=true;
 assert(Qwen35FinalizeRetainedRecoveryLocked(owner,context,request,true)==B_DEV_NOT_READY);
 assert(sRuntimeOwner==owner && Cold() && !completeCalls && !unreserveCalls
  && sRecoveryGuardV2.validate(ticket)==B_DEV_NOT_READY);
 delete context;owner->magic=0;delete owner;failValidation=false;
 owner=new PioneerProbeCookie;context=new PioneerStoreContext;owner->context=context;
 sRuntimeOwner=owner;Start(*context);failUnreserve=true;
 request={PIONEER_RECOVERY_STAGE_GUARD_COMPLETE,0};
 assert(Qwen35FinalizeRetainedRecoveryLocked(owner,context,request,true)==B_DEV_NOT_READY);
 assert(request.stage==PIONEER_RECOVERY_STAGE_UNRESERVE && unreserveCalls==1
  && !completeCalls && sRuntimeOwner==owner && Cold());
 request.stage=PIONEER_RECOVERY_STAGE_GUARD_COMPLETE;failUnreserve=false;
 assert(Qwen35FinalizeRetainedRecoveryLocked(owner,context,request,true)==B_NOT_ALLOWED
  && unreserveCalls==1);
 delete context;owner->magic=0;delete owner;
 owner=new PioneerProbeCookie;context=new PioneerStoreContext;owner->context=context;
 sRuntimeOwner=owner;Start(*context);failComplete=true;
 request={PIONEER_RECOVERY_STAGE_GUARD_COMPLETE,0};
 assert(Qwen35FinalizeRetainedRecoveryLocked(owner,context,request,true)==B_DEV_NOT_READY);
 assert(unreserveCalls==2 && completeCalls==1 && sRuntimeOwner==owner && Cold());
 failComplete=false;
 assert(Qwen35FinalizeRetainedRecoveryLocked(owner,context,request,true)==B_NOT_ALLOWED
  && unreserveCalls==2);
 delete context;owner->magic=0;delete owner;
 owner=new PioneerProbeCookie;context=new PioneerStoreContext;owner->context=context;
 sRuntimeOwner=owner;ticket=Start(*context);context->recoveryFinishKeepOwnerCookie=true;
 request={PIONEER_RECOVERY_STAGE_GUARD_COMPLETE,0};
 assert(Cold() && sRecoveryGuardV2.validate(ticket)==B_OK);
 assert(Qwen35FinalizeRetainedRecoveryLocked(owner,context,request,true)==B_OK);
 assert(!sRuntimeOwner && !owner->context && !sColdRecoveryRequired && !Cold() && !sBankStorageOnly);
 assert(sReloadGeneration==2 && request.new_generation==2 && request.stage==PIONEER_RECOVERY_STAGE_COMPLETE);
 assert(contextsDestroyed==4 && cookiesDestroyed==3);owner->magic=0;delete owner;
 owner=new PioneerProbeCookie;context=new PioneerStoreContext;owner->context=context;
 sRuntimeOwner=owner;Start(*context);request={PIONEER_RECOVERY_STAGE_GUARD_COMPLETE,0};
 assert(Qwen35FinalizeRetainedRecoveryLocked(owner,context,request,true)==B_OK);
 assert(contextsDestroyed==5 && cookiesDestroyed==5 && !sRuntimeOwner && !Cold());
 owner=new PioneerProbeCookie;context=new PioneerStoreContext;owner->context=context;
 sRuntimeOwner=owner;Start(*context);faultOnDestroy=true;
 request={PIONEER_RECOVERY_STAGE_GUARD_COMPLETE,0};
 assert(Qwen35FinalizeRetainedRecoveryLocked(owner,context,request,true)==B_DEV_NOT_READY);
 assert(sRuntimeOwner==owner && !owner->context && Cold() && sColdRecoveryRequired
  && sBankStorageOnly && request.stage==PIONEER_RECOVERY_STAGE_GUARD_COMPLETE
  && request.new_generation==0 && contextsDestroyed==6 && cookiesDestroyed==5);
 faultOnDestroy=false;owner->magic=0;delete owner;sRuntimeOwner=nullptr;
 owner=new PioneerProbeCookie;context=new PioneerStoreContext;owner->context=context;
 sRuntimeOwner=owner;Start(*context);faultOnUnreserve=true;
 request={PIONEER_RECOVERY_STAGE_GUARD_COMPLETE,0};
 const int beforeUnreserve=unreserveCalls,beforeComplete=completeCalls;
 assert(Qwen35FinalizeRetainedRecoveryLocked(owner,context,request,true)==B_DEV_NOT_READY);
 assert(unreserveCalls==beforeUnreserve+1 && completeCalls==beforeComplete+1
  && sRuntimeOwner==owner && owner->context==context && Cold() && !request.new_generation);
 faultOnUnreserve=false;
 assert(Qwen35FinalizeRetainedRecoveryLocked(owner,context,request,true)==B_NOT_ALLOWED
  && unreserveCalls==beforeUnreserve+1);
 delete context;owner->magic=0;delete owner;sRuntimeOwner=nullptr;
 std::puts("PASS actual guard stays cold until reservation and retained lease retire; SIMULATED ONLY");
}
