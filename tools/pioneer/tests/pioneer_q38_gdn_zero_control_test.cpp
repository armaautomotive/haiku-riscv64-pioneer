/* SPDX-License-Identifier: MIT */
// ACTUAL handler/lock/call seal/real CPU bank binding. The initializer is
// an EXPLICIT MOCK:NOT zero data/RV32/native initialization qualification.
#define PIONEER_Q38_GDN_CALL_SCOPE_MODEL_ONLY
#include "pioneer_q38_gdn_call_scope_test.cpp"
#define B_DEVICE_OP_CODES_END 1000
using uint32=uint32_t;
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38InitializeOwnedState.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnZeroProtocol.h"
static unsigned copyCalls=0,failCopy=0,initializeCalls=0,initializeFault=0;
static status_t user_memcpy(void* to,const void* from,size_t bytes) {
 if (++copyCalls==failCopy) return -99; memcpy(to,from,bytes); return B_OK;
}
static bool InitializeQ38MutableState(PioneerStoreContext& c,PioneerQ38InitializeOwnedResult& r) {
 initializeCalls++; assert(c.q38GdnCallThread==42 && c.q38GdnBanks && c.q38StateZero && c.io.StateZeroBeforeCohort());
 r={1,672,672,73728,1,1,0};
 if (initializeFault==1) return false;
 if (initializeFault==2) r.pages--; if (initializeFault==3) r.jobs--; if (initializeFault==4) r.regions--;
 if (initializeFault==5) r.closed=0; if (initializeFault==6) r.published=0;
 if (initializeFault==7) r.recovery_required=1; if (initializeFault==8) c.otherUnsafe=true;
 return true;
}
static bool QualifyQ38MutableState(PioneerStoreContext& c,PioneerQ38InitializeOwnedResult& r) {
 initializeCalls++; assert(c.q38GdnCallThread==42 && c.q38GdnBanks && c.q38StateZero && c.io.StateZeroBeforeCohort());
 r={1,1,1,1,1,0,0}; if (initializeFault) r.published=1; return true; // EXPLICIT MOCK,NO data zeroing
}
#include "zero_control.inc"
static void Ready(Fixture& f,PioneerQ38SpanLease& span) {
 Reset(); copyCalls=failCopy=initializeCalls=initializeFault=0; sQ38GdnDiagnosticEpoch=0;
 span.closed=true; f.c.q38Span=&span;
 f.c.runtimeCoreMode=-1;
 f.c.lease.saved=&f.c.io.dummy; f.c.lease.ready=f.c.bulk.saved=f.c.bulk.ready=true;
 f.c.bulkHost.allocated=f.c.bulkHost.ready=f.c.wideHost.ready=f.c.wideHost.memory_type_set=true;
}
#ifndef PIONEER_Q38_GDN_ZERO_CONTROL_MODEL_ONLY
int main() {
 (void)&Input; (void)&Prepare; (void)&Upload; (void)&Close;
 {
  Fixture f(false); PioneerQ38SpanLease span; Ready(f,span); tt_q38_gdn_zero_support support={};
  assert(Q38GdnZeroControl(&f.cookie,TT_PCI_PROBE_Q38_GDN_ZERO_SUPPORT,&support,sizeof(support))==B_OK
   && support.version==1 && support.driver==55 && support.zero_only==1 && support.pages==73728 && support.max_regions==672
   && support.requires_closed_span==1 && !support.model_inference && support.pilot_pages==1 && !initializeCalls && !f.c.io.touches && acquired==released);
  tt_q38_gdn_zero_request r={}; r.version=1; r.mode=2; auto before=f.initialized;
  assert(Q38GdnZeroControl(&f.cookie,TT_PCI_PROBE_Q38_GDN_ZERO_INITIALIZE,&r,sizeof(r))==B_OK && initializeCalls==1
   && r.state==TT_RUNTIME_PASSED && r.regions==672 && r.jobs==672 && r.pages==73728 && r.closed && r.initialization_published && !r.recovery_required);
  assert(f.c.q38GdnIdentity.session==1 && f.c.q38GdnIdentity.sequence==1 && f.c.q38GdnIdentity.modelEpoch==UINT64_C(0x5147385a45524f31)
   && f.c.q38GdnIdentity.driverGeneration==7 && f.c.q38GdnCallThread==-1 && !f.c.retained && !f.c.io.touches && before==f.initialized);
  // Mock success is NOT a real initialization seal or device-read grant.
  assert(!f.c.q38GdnBanks->InitializationPublished(f.c.q38GdnIdentity));
  r={}; r.version=1; r.mode=2;
  assert(Q38GdnZeroControl(&f.cookie,TT_PCI_PROBE_Q38_GDN_ZERO_INITIALIZE,&r,sizeof(r))==B_NOT_ALLOWED
   && initializeCalls==1 && !f.c.io.touches && acquired==released);
  assert(AcquireProbeLock(&f.cookie,true)==B_NOT_ALLOWED && acquired==released);
 }
 for (unsigned fault=0;fault<33;fault++) {
  Fixture f(false); PioneerQ38SpanLease span; Ready(f,span); tt_q38_gdn_zero_request r={}; r.version=1; r.mode=2;
  unsigned op=TT_PCI_PROBE_Q38_GDN_ZERO_INITIALIZE; size_t bytes=sizeof(r); PioneerProbeCookie* cookie=&f.cookie; void* data=&r;
  if (!fault) cookie=nullptr; if (fault==1) f.cookie.magic=0; if (fault==2) data=nullptr;
  if (fault==3) currentTeam=18; if (fault==4) threadStatus=-7; if (fault==5) op++; if (fault==6) bytes--;
  if (fault==7) r.version=2; if (fault>=8 && fault<=9) r.reserved[fault-8]=1; if (fault==10) r.mode=0; if (fault==11) failCopy=1;
  if (fault==12) f.c.q38Span=nullptr; if (fault==13) span.closed=false; if (fault==14) span.dirty=true;
  if (fault==15) span.ready=true; if (fault==16) span.running=true; if (fault==17) span.unsafe=true;
  if (fault==18) f.c.io.q38SpanAllowed=false; if (fault==19) f.c.lease.saved=nullptr;
  if (fault==20) f.c.lease.ready=false; if (fault==21) f.c.lease.pending=true; if (fault==22) f.c.lease.recovery_required=true;
  if (fault==23) f.c.bulk.saved=false; if (fault==24) f.c.bulk.ready=false; if (fault==25) f.c.bulk.dirty=true;
  if (fault==26) f.c.bulkHost.ready=false; if (fault==27) f.c.bulkHost.pending=true;
  if (fault==28) f.c.wideHost.memory_type_set=false; if (fault==29) f.c.wideHost.pending=true;
  if (fault==30) sQ38GdnDiagnosticEpoch=UINT64_MAX;
  if (fault==31) f.c.runtimeCoreMode=1;
  if (fault==32) r.mode=3;
  assert(Q38GdnZeroControl(cookie,op,data,bytes)!=B_OK && !initializeCalls && !f.c.q38GdnBanks && !f.c.q38StateZero
   && !f.c.io.touches && !f.c.retained && acquired==released);
 }
 for (unsigned fault=0;fault<=8;fault++) {
  Fixture f(false); PioneerQ38SpanLease span; Ready(f,span); tt_q38_gdn_zero_request r={}; r.version=1; r.mode=2;
  if (!fault) failCopy=2; else initializeFault=fault;
  int status=Q38GdnZeroControl(&f.cookie,TT_PCI_PROBE_Q38_GDN_ZERO_INITIALIZE,&r,sizeof(r));
  assert(status==(!fault?-99:B_OK) && initializeCalls==1 && f.c.retained && sColdRecoveryRequired && f.c.q38GdnBanks
   && f.c.q38StateZero && f.c.q38StateZero->unsafe && f.c.q38GdnBanks->Unsafe() && !f.c.io.touches && acquired==released);
  if (fault) assert(r.state==TT_RUNTIME_COMPLETION_FAILED && !r.initialization_published && r.recovery_required);
  unsigned copies=copyCalls;
  assert(Q38GdnZeroControl(&f.cookie,TT_PCI_PROBE_Q38_GDN_ZERO_INITIALIZE,&r,sizeof(r))==B_DEV_NOT_READY
   && initializeCalls==1 && copyCalls==copies+1 && !f.c.io.touches && acquired==released);
 }
 for (unsigned fault=0;fault<2;fault++) {
  Fixture f(false); PioneerQ38SpanLease span; Ready(f,span); tt_q38_gdn_zero_request r={}; r.version=1; r.mode=1;
  initializeFault=fault;
  assert(Q38GdnZeroControl(&f.cookie,TT_PCI_PROBE_Q38_GDN_ZERO_INITIALIZE,&r,sizeof(r))==B_OK
   && r.mode==1 && initializeCalls==1 && r.regions==1 && r.jobs==1 && r.pages==1 && !r.initialization_published
   && r.recovery_required==fault && f.c.retained==bool(fault) && !f.c.io.touches && acquired==released);
 }
 puts("PASS OFFLINE ACTUAL fixed zero entry+lock+call seal+CPU bank claims:private epochs,closed-span/ready-owner one-way mode handoff,33 preflight refusals,duplicate inert,fixed1-page pilot NEVER publishes,8 initializer failure/bad completion fields+lost reply retain ALL owners;ZERO IO;INITIALIZERS EXPLICITLY MOCKED,NOT native/RV32/zero-data or readiness qualification");
}
#endif
