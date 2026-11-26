/* SPDX-License-Identifier: MIT */
// Tests the actual deferred-completion helper with simulated driver objects.
#include <cassert>
#include <cstdint>
#include <cstdio>
using status_t=int;using uint64=uint64_t;
enum {B_OK=0,B_NOT_ALLOWED=-1,PIONEER_RECOVERY_STAGE_GUARD_COMPLETE=11,
 PIONEER_RECOVERY_STAGE_COMPLETE=12};
static int contextsDestroyed=0,cookiesDestroyed=0;
struct PioneerStoreContext {
 uint64 recoveryFinishCompletedGeneration=0;
 bool recoveryFinishCompleted=false,recoveryFinishKeepOwnerCookie=false;
 ~PioneerStoreContext() {++contextsDestroyed;}
};
struct PioneerProbeCookie {
 PioneerStoreContext* context=nullptr;int magic=42;
 ~PioneerProbeCookie() {assert(magic==0);++cookiesDestroyed;}
};
struct tt_pioneer_recovery_run {unsigned stage=0;uint64 new_generation=0;};
struct ColdLatch {bool cold=true;operator bool() const {return cold;}
 void ClearAfterVerifiedRecovery(){cold=false;}};
static ColdLatch sColdRecoveryRequired;
static uint64 sReloadGeneration=3;
static PioneerProbeCookie* sRuntimeOwner=nullptr;
static bool sBankStorageOnly=true;
#include "pioneer_q35_retained_finish.inc"
int main() {
 auto* owner=new PioneerProbeCookie;auto* context=new PioneerStoreContext;
 owner->context=context;sRuntimeOwner=owner;
 tt_pioneer_recovery_run request{PIONEER_RECOVERY_STAGE_GUARD_COMPLETE,0};
 assert(Qwen35FinalizeRetainedRecoveryLocked(owner,context,request,true)==B_NOT_ALLOWED);
 context->recoveryFinishCompleted=true;context->recoveryFinishCompletedGeneration=9;
 assert(Qwen35FinalizeRetainedRecoveryLocked(owner,context,request,false)==B_NOT_ALLOWED);
 assert(sRuntimeOwner==owner && owner->context==context && sColdRecoveryRequired && sReloadGeneration==3);
 request.stage=0;
 assert(Qwen35FinalizeRetainedRecoveryLocked(owner,context,request,true)==B_NOT_ALLOWED);
 request.stage=PIONEER_RECOVERY_STAGE_GUARD_COMPLETE;
 context->recoveryFinishKeepOwnerCookie=true;
 assert(Qwen35FinalizeRetainedRecoveryLocked(owner,context,request,true)==B_OK);
 assert(!sRuntimeOwner && !owner->context && !sColdRecoveryRequired && !sBankStorageOnly);
 assert(sReloadGeneration==9 && request.new_generation==9 && request.stage==PIONEER_RECOVERY_STAGE_COMPLETE);
 assert(contextsDestroyed==1 && cookiesDestroyed==0);owner->magic=0;delete owner;
 owner=new PioneerProbeCookie;context=new PioneerStoreContext;owner->context=context;
 context->recoveryFinishCompleted=true;context->recoveryFinishCompletedGeneration=10;
 sRuntimeOwner=owner;sBankStorageOnly=true;sColdRecoveryRequired.cold=true;
 request={PIONEER_RECOVERY_STAGE_GUARD_COMPLETE,0};
 assert(Qwen35FinalizeRetainedRecoveryLocked(owner,context,request,true)==B_OK);
 assert(contextsDestroyed==2 && cookiesDestroyed==2 && !sRuntimeOwner && !sColdRecoveryRequired);
 assert(sReloadGeneration==10 && request.new_generation==10);
 std::puts("PASS retained recovery completion keeps owner cold until lease and pins retire; SIMULATED ONLY");
}
