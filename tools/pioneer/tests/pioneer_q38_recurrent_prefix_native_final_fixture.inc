/* SPDX-License-Identifier: MIT */
// TEST composition of actual native END/source bodies with raw OS/PCI endpoints.
#ifdef PIONEER_Q38_RECURRENT_PREFIX_FINAL_RELEASE
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DispatchMmioEnd.h"
static unsigned finalUnreserved=0;
static struct{unsigned bus=1,device=2,function=0;}sCard;
static int sCookie=0;
#define TT_PCI_PROBE_DEVICE "misc/tenstorrent_pci_probe"
struct FinalPciService{
 void unreserve_device(unsigned b,unsigned d,unsigned fn,const char* name,int* cookie){
  assert(b==1 && d==2 && !fn && !strcmp(name,TT_PCI_PROBE_DEVICE) && cookie==&sCookie && finalRemoved.size()==33 && !finalDeleteFailed && !finalPciFailed);finalUnreserved++;
  auto* c=sRuntimeOwner->context;assert(c && c->q38Clock.Snapshot().after_mhz==800 && c->q38Clock.Notifications()==5 && c->q38Clock.Unmaps()==1);
#ifdef PIONEER_Q38_RECURRENT_PREFIX_FRESH_FINAL_TEST
  assert(finalLockHeld && finalAuditCalls==1 && finalAuditReturned && finalQuietReads && c->idle.ready && c->idle.tlb_restored && !c->idle.recovery_required);
#endif
 }
};
static FinalPciService finalPciService;static FinalPciService* sPCI=&finalPciService;
static void NativeFinalTail(PioneerProbeCookie* cookie,tt_resident_session_result& result,bool recheck){
 auto* context=cookie->context;
#ifdef PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_CONTROL_COMPILE_ONLY
 context->Snapshot(result); // actual CPU metadata method,NOT manufactured closed progress
#endif
 bool clean=!sColdRecoveryRequired && !context->Unsafe();
#include "native_final_prefix.inc"
 assert(false && "the existing native mixed source must refuse any non-bank fallback");return;
#include "native_final_tail.inc"
// Actual source footer closes this function itself.
#ifdef PIONEER_Q38_RECURRENT_PREFIX_FRESH_FINAL_TEST
static void EndResidentSession(PioneerProbeCookie* cookie,tt_resident_session_result& result,bool recheck){
 assert(finalLockHeld && recheck);NativeFinalTail(cookie,result,recheck);
} // actual owned prefix/footer;the fixture omits only the initial metadata Snapshot
static status_t NativeSealedFinalEnd(PioneerProbeCookie* cookie,tt_resident_session_result& result){
 status_t status=AcquireQ38GdnSessionLock(cookie);if(status!=B_OK)return status;
 struct{tt_resident_session_result result;}request={};
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38RecurrentPrefixFinalEndCall.inc"
 result=request.result;release_sem(sLock);return status;
}
#endif
#endif
