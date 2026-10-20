/* SPDX-License-Identifier: MIT */
// Actual extracted handler,lock,END ownership and final-cookie policy.
#define PIONEER_Q38_LOCAL_DELTA_PROBE
#define PIONEER_Q38_DELTA_PROBE_MOCK_ONLY
#include "pioneer_q38_delta_probe_test.cpp"
#define TT_PIONEER_EXPERIMENTAL_Q38_BANKS
#define TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
#define TT_PIONEER_Q38_WORK_MODE
using status_t=int; using uint32=uint32_t;
constexpr status_t B_OK=0,B_DEV_NOT_READY=-1,B_BUSY=-2,B_NOT_ALLOWED=-3,B_BAD_VALUE=-4,B_NO_MEMORY=-5;
struct thread_info { int team; };
static int team=17;
static int find_thread(void*) { return 1; }
static int get_thread_info(int,thread_info* t) { t->team=team; return B_OK; }
struct Ref1 { tt_dram_samples_result& dram; };
struct Ref2 { Ref1 baseline; tt_compute_noc_result noc; };
struct Ref3 { Ref2 baseline; };
struct Ref4 { Ref3 baseline; };
struct DispatchStub { bool closed=false; };
struct PioneerStoreContext:DeltaFixture {
 Ref4 baseline; PioneerResidentBanks& banks; PioneerResidentLease& lease; PioneerQ38DeltaProbeLease& q38Delta;
 DispatchStub* q38Dispatch=nullptr;
 void* q38Cohort=nullptr;
 PioneerQ38PanelLease panelProbe,streamProbe;
 bool bankStorageOnly=true,dispatchOwnerRetained=false;
 struct HostLease { bool allocated=true,released=false,ready=true,quarantined=false; } bulkHost,wideHost;
 explicit PioneerStoreContext(const char* image):DeltaFixture(image),baseline{{{{own.dram},own.noc}}},banks(own.banks),lease(own.store),q38Delta(DeltaFixture::lease) {}
 bool Unsafe() { return q38Delta.unsafe || q38Delta.running || lease.recovery_required || banks.Quarantined(); }
};
struct PioneerProbeCookie { uint32_t magic=0x50313030; int team=17; PioneerStoreContext* context=nullptr; };
static PioneerProbeCookie* sRuntimeOwner=nullptr;
static bool sBankStorageOnly=false,sColdRecoveryRequired=false;
static int sLock=1,acquired=0,released=0,copyCalls=0,failCopy=0,cleanEnds=0;
static status_t acquire_sem(int lock) { assert(lock==1); acquired++; return B_OK; }
static void release_sem(int lock) { assert(lock==1); released++; }
#include "pioneer_lock.inc"
static status_t user_memcpy(void* to,const void* from,size_t bytes) {
 if (++copyCalls==failCopy) return -99; memcpy(to,from,bytes); return B_OK;
}
// Unrelated dispatcher is excluded by the real handler; these sentinels
// prove the LOCAL path never calls its prepare/close/quarantine functions.
static void PioneerQ38DispatchQuarantine(DispatchStub&,PioneerResidentBanks&) { assert(false); }
static bool PioneerQ38DispatchClose(DeltaIO&,const tt_dram_samples_result&,PioneerResidentBanks&,DispatchStub&,bool) { assert(false); return false; }
#include "pioneer_q38_delta_control.inc"
#include "pioneer_q38_dispatch_retain.inc"
static void EndResidentSession(PioneerProbeCookie* cookie,tt_resident_session_result& r,bool)
{
 auto& c=*cookie->context; bool clean=!sColdRecoveryRequired && !c.Unsafe();
 if (!EndQ38DispatchOwnership(c,r,clean)) return;
 assert(clean && !c.q38Delta.dirty); cleanEnds++;
 Finish(c.io,c.own); delete cookie->context; cookie->context=nullptr; sRuntimeOwner=nullptr; sBankStorageOnly=false; sQ38LocalDeltaOwner=nullptr;
}
#include "pioneer_q38_dispatch_cookie_release.inc"
static PioneerProbeCookie* Begin(const char* image) {
 assert(!sRuntimeOwner); auto* c=new PioneerProbeCookie; c->context=new PioneerStoreContext(image);
 sRuntimeOwner=c; sBankStorageOnly=true; return c;
}
static void DiscardRetained(PioneerProbeCookie* c) {
 assert(c->context->dispatchOwnerRetained && c->context->bulkHost.quarantined && c->context->wideHost.quarantined);
 assert(sRuntimeOwner==c && c->magic==0x50313030 && sColdRecoveryRequired);
 delete c->context; delete c; sRuntimeOwner=nullptr; sBankStorageOnly=false; sColdRecoveryRequired=false; sQ38LocalDeltaOwner=nullptr;
}
#ifndef PIONEER_Q38_DELTA_CONTROL_MOCK_ONLY
int main(int argc,char** argv)
{
 assert(argc==2);
 (void)&Input; (void)&Prepare; (void)&Upload; (void)&Close; (void)&PreparePanel; (void)&Run; (void)&Job; (void)&ClosePanel; (void)&Finish;
 PioneerProbeCookie empty,foreign; tt_q38_delta_support support={};
 assert(Q38DeltaControl(&empty,TT_PCI_PROBE_Q38_DELTA_SUPPORT,&support,sizeof(support))==B_OK
  && support.version==TT_Q38_DELTA_PROBE_VERSION && support.driver==55 && support.local_only && support.keys==128 && support.values==64 && support.state_words==8192 && support.accept_required);
 auto request=std::make_unique<tt_q38_delta_probe>(); PioneerQ38DeltaDebugFixture(*request,1);
 assert(Q38DeltaControl(&empty,TT_PCI_PROBE_Q38_DELTA_RUN,request.get(),sizeof(*request))==B_NOT_ALLOWED);
 for (unsigned phase=0;phase<5;phase++) {
  auto* c=Begin(argv[1]); auto& f=*c->context;
  unsigned reads=f.io.reads,writes=f.io.writes;
  team=18; assert(Q38DeltaControl(c,TT_PCI_PROBE_Q38_DELTA_RUN,request.get(),sizeof(*request))==B_NOT_ALLOWED); team=17;
  assert(Q38DeltaControl(&foreign,TT_PCI_PROBE_Q38_DELTA_RUN,request.get(),sizeof(*request))==B_BUSY);
  assert(Q38DeltaControl(c,TT_PCI_PROBE_Q38_DELTA_RUN,request.get(),sizeof(*request)-1)==B_BAD_VALUE);
  request->reserved[1]=1;
  assert(Q38DeltaControl(c,TT_PCI_PROBE_Q38_DELTA_RUN,request.get(),sizeof(*request))==B_BAD_VALUE); request->reserved[1]=0;
  assert(reads==f.io.reads && writes==f.io.writes);
  if (phase==4) failCopy=copyCalls+2; // lose successful RUN's reply
  assert(Q38DeltaControl(c,TT_PCI_PROBE_Q38_DELTA_RUN,request.get(),sizeof(*request))==(phase==4?-99:B_OK));
  failCopy=0;
  if (phase!=4) assert(!PioneerQ38DeltaDebugErrors(*request) && request->result.stopped && f.q38Delta.awaiting);
  if (phase!=4) assert(AcquireProbeLock(c,true)==B_NOT_ALLOWED); // bank/dispatch mutation blocked under actual lock
  reads=f.io.reads; writes=f.io.writes; unsigned resets=f.io.resetWrites;
  tt_q38_delta_accept accept={TT_Q38_DELTA_PROBE_VERSION,request->result.nonce,{0,0}};
  if (!phase) {
   assert(Q38DeltaControl(c,TT_PCI_PROBE_Q38_DELTA_ACCEPT,&accept,sizeof(accept))==B_OK);
   tt_resident_session_result r={}; EndResidentSession(c,r,true); assert(!c->context); assert(ReleaseResidentCookie(c)==B_OK); continue;
  }
  if (phase==1) { // explicit END before ACCEPT:retain with zero reads/writes
   tt_resident_session_result r={}; EndResidentSession(c,r,true);
  } else if (phase==2) { // finite numeric mismatch:client CLOSE ONLY
   request->next[8191]^=1; assert(PioneerQ38DeltaDebugErrors(*request)==1);
   assert(ReleaseResidentCookie(c)==B_OK);
  } else if (phase==3) { // even ACCEPT without explicit END cannot implicit-restore
   assert(Q38DeltaControl(c,TT_PCI_PROBE_Q38_DELTA_ACCEPT,&accept,sizeof(accept))==B_OK);
   assert(ReleaseResidentCookie(c)==B_OK);
  } else assert(ReleaseResidentCookie(c)==B_OK);
  assert(reads==f.io.reads && writes==f.io.writes && resets==f.io.resetWrites);
  DiscardRetained(c);
 }
 for (unsigned fault:{1U,3U,5U,6U,15U,17U,18U,19U,21U}) {
  auto* c=Begin(argv[1]); auto& f=*c->context; f.io.fault=fault;
  assert(Q38DeltaControl(c,TT_PCI_PROBE_Q38_DELTA_RUN,request.get(),sizeof(*request))==B_OK && sColdRecoveryRequired && f.Unsafe());
  unsigned reads=f.io.reads,writes=f.io.writes,resets=f.io.resetWrites;
  assert(Q38DeltaControl(c,TT_PCI_PROBE_Q38_DELTA_RUN,request.get(),sizeof(*request))==B_DEV_NOT_READY);
  assert(ReleaseResidentCookie(c)==B_OK && reads==f.io.reads && writes==f.io.writes && resets==f.io.resetWrites);
  DiscardRetained(c);
 }
 assert(cleanEnds==1 && acquired==released);
 puts("PASS OFFLINE actual ioctl+lock+END+cookie release:clean explicit ACCEPT/END,unaccepted END,finite mismatch CLOSE,lost reply,ACCEPT without END,9 hardware faults retain context/cookie/PCI/ALL host owners with ZERO later I/O");
}
#endif
