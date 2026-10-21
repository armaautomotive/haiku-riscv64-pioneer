/* SPDX-License-Identifier: MIT */
// ACTUAL entry/lock/call guard/controller/RV32/SFPU;simulated NoC/GDDR.
// Initial whole zero ledger is EXPLICIT counter fabric;NOT native IO.
#define B_DEVICE_OP_CODES_END 1000
#define PIONEER_Q38_RGDN_ROOT_CONTROLLER_MODEL_ONLY
#include "pioneer_q38_rgdn_root_controller_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DispatchBootstrap.h"
using status_t=int; using uint32=uint32_t; using thread_id=int;
constexpr int B_OK=0,B_DEV_NOT_READY=-1,B_BUSY=-2,B_NOT_ALLOWED=-3,B_BAD_VALUE=-4,B_NO_MEMORY=-5;
struct thread_info { int team; };
static int currentTeam=17,currentThread=42,sLock=1,acquired=0,released=0;
static int find_thread(void*) { return currentThread; }
static int get_thread_info(int,thread_info* t) { t->team=currentTeam; return B_OK; }
static int acquire_sem(int) { acquired++; return B_OK; }
static void release_sem(int) { released++; }
struct ControlClock { bool unsafe=false; void Quarantine() { unsafe=true; } };
struct Ref1 { tt_dram_samples_result& dram; };
struct Ref2 { Ref1 baseline; tt_compute_noc_result& noc; };
struct Ref3 { Ref2 baseline; };
struct Ref4 { Ref3 baseline; };
struct PioneerStoreContext {
 RgdnFixture f;
 RgdnControlIO& io; PioneerResidentBanks& banks;
 PioneerQ38GdnBanks* q38GdnBanks; PioneerQ38StateZeroLease* q38StateZero;
 PioneerQ38ResidentGdnRootLease* q38ResidentGdnRoot=nullptr;
 PioneerQ38GdnIdentity q38GdnIdentity;
 Ref4 baseline;
 tt_host_page_result& host; PioneerResidentLease& lease; PioneerBulkUploadLease& bulk;
 PioneerOutputBufferLease& bulkHost; PioneerWideOutputLease& wideHost;
 PioneerQ38DispatchLease* q38Dispatch=nullptr;
 ControlClock q38Clock;
 uint64_t bytes=PioneerResidentBanks::Total;
 bool bankStorageOnly=true,dispatchOwnerRetained=false;
 int runtimeCoreMode=1,q38GdnCallThread=-1;
 PioneerStoreContext(const char* zero,const char* root):f(zero,root),io(f.io),banks(f.owner.banks),q38GdnBanks(f.state.get()),q38StateZero(&f.zero),
  q38GdnIdentity(f.identity),baseline{{{{f.owner.dram},f.owner.noc}}},host(f.owner.host),lease(f.owner.store),bulk(f.bulk),bulkHost(f.transfer),wideHost(f.wide) { io.q38MutableContext=this; }
 bool Unsafe() const { return q38Clock.unsafe || q38GdnBanks->Unsafe() || lease.recovery_required
  || (q38ResidentGdnRoot && (q38ResidentGdnRoot->unsafe || q38ResidentGdnRoot->running)); }
 ~PioneerStoreContext() { delete q38ResidentGdnRoot; }
};
struct PioneerProbeCookie { uint32_t magic=0x50313030; int team=17; PioneerStoreContext* context=nullptr; };
static PioneerProbeCookie* sRuntimeOwner=nullptr;
static bool sColdRecoveryRequired=false,sReloadQuiesced=false,sBankStorageOnly=true;
static uint64_t sReloadGeneration=4;
static unsigned copyCalls=0,failCopy=0;
static int user_memcpy(void* to,const void* from,size_t bytes) { if (++copyCalls==failCopy) return -99; memcpy(to,from,bytes); return B_OK; }
#include "native_lock.inc"
#include "native_retain.inc"
#include "call_scope.inc"
#include "root_control.inc"
struct ControlFixture {
 PioneerStoreContext c;
 PioneerProbeCookie cookie{0x50313030,17,&c};
 ControlFixture(const char* zero,const char* root):c(zero,root) {
  sRuntimeOwner=&cookie; sColdRecoveryRequired=sReloadQuiesced=false; sBankStorageOnly=true; sReloadGeneration=4;
  currentTeam=17; currentThread=42; acquired=released=0; copyCalls=failCopy=0;
 }
 bool Run(unsigned trial=4) { c.f.Input(trial); return Q38RgdnRootControl(&cookie,TT_PCI_PROBE_Q38_RGDN_ROOT_RUN,c.f.job.get(),sizeof(*c.f.job))==B_OK; }
 bool Accept() { tt_q38_rgdn_root_accept a={1,c.q38ResidentGdnRoot->nonce,{0,0}}; return Q38RgdnRootControl(&cookie,TT_PCI_PROBE_Q38_RGDN_ROOT_ACCEPT,&a,sizeof(a))==B_OK; }
 bool End() { tt_q38_rgdn_root_end r={1,0,0,0}; return Q38RgdnRootControl(&cookie,TT_PCI_PROBE_Q38_RGDN_ROOT_END,&r,sizeof(r))==B_OK && r.closed && !r.recovery_required; }
 void Retained() {
  assert(sColdRecoveryRequired && c.dispatchOwnerRetained && c.q38Clock.unsafe && c.q38ResidentGdnRoot->unsafe && c.q38GdnBanks->Unsafe()
   && c.bulkHost.quarantined && c.wideHost.quarantined && cookie.context==&c && sRuntimeOwner==&cookie && c.q38GdnCallThread==-1 && acquired==released);
  unsigned touches=c.io.touches; tt_q38_rgdn_root_end r={1,0,0,0};
  assert(Q38RgdnRootControl(&cookie,TT_PCI_PROBE_Q38_RGDN_ROOT_END,&r,sizeof(r))==B_DEV_NOT_READY && c.io.touches==touches);
 }
};
#ifndef PIONEER_Q38_RGDN_ROOT_CONTROL_MODEL_ONLY
int main(int argc,char** argv) {
 assert(argc==3); setvbuf(stdout,nullptr,_IOLBF,0); (void)&TestWorker;
 (void)&Input; (void)&Prepare; (void)&Upload; (void)&Close; (void)&PreparePanel; (void)&Run; (void)&Job; (void)&ClosePanel; (void)&Finish;
 {
  ControlFixture f(argv[1],argv[2]); tt_q38_rgdn_root_support s={}; unsigned touches=f.c.io.touches;
  assert(Q38RgdnRootControl(&f.cookie,TT_PCI_PROBE_Q38_RGDN_ROOT_SUPPORT,&s,sizeof(s))==B_OK && s.version==1 && s.driver==55
   && !s.fixed_root && !s.fixed_head && s.keys==128 && s.values==64 && s.requires_initialization==1 && s.slots==2
   && s.accept_required==1 && !s.model_inference && f.c.io.touches==touches);
  for (unsigned position=0;position<8;position++) {
   assert(f.Run(4+position) && f.c.f.job->result.state==TT_RUNTIME_PASSED && f.c.f.job->result.awaiting);
   // The controller suite independently checks ALL FP32 words;this entry
   // test verifies actual call-seal/nonce/position and no implicit commit.
   assert(f.c.q38ResidentGdnRoot->position==position && f.c.q38ResidentGdnRoot->awaiting && f.c.q38GdnCallThread==-1);
   touches=f.c.io.touches; auto job=std::make_unique<tt_q38_rgdn_root_request>(*f.c.f.job);
   assert(Q38RgdnRootControl(&f.cookie,TT_PCI_PROBE_Q38_RGDN_ROOT_RUN,job.get(),sizeof(*job))==B_NOT_ALLOWED && f.c.io.touches==touches);
   assert(f.Accept() && !f.Accept() && f.c.q38ResidentGdnRoot->position==position+1);
  }
  assert(f.End() && f.c.q38ResidentGdnRoot->closed && !f.c.Unsafe() && !sColdRecoveryRequired && acquired==released);
 }
 for (unsigned bad=0;bad<10;bad++) {
  ControlFixture f(argv[1],argv[2]); f.c.f.Input(); unsigned touches=f.c.io.touches;
  if (!bad) f.c.f.job->version=0; if (bad==1) f.c.f.job->reserved[1]=1;
  if (bad==2) f.c.f.job->query[127]=1; if (bad==3) f.c.f.job->beta=0x7fc00000;
  if (bad==4) currentTeam=18; if (bad==5) f.c.q38GdnIdentity.driverGeneration++;
  if (bad==6) f.c.q38StateZero->closed=false; if (bad==7) f.c.q38StateZero->ready=true;
  if (bad==8) f.c.runtimeCoreMode=-1; if (bad==9) f.c.io.q38MutableContext=nullptr;
  assert(Q38RgdnRootControl(&f.cookie,TT_PCI_PROBE_Q38_RGDN_ROOT_RUN,f.c.f.job.get(),sizeof(*f.c.f.job))!=B_OK
   && !f.c.q38ResidentGdnRoot && f.c.io.touches==touches && !sColdRecoveryRequired && acquired==released);
 }
 for (unsigned fault=0;fault<5;fault++) {
  ControlFixture f(argv[1],argv[2]); f.c.f.Input();
  if (!fault) failCopy=2; // actual successful RUN's reply is lost
  if (fault==1) f.c.io.vmFaultPacket=3;
  if (fault==2) f.c.io.corruptCounter=true;
  int status=Q38RgdnRootControl(&f.cookie,TT_PCI_PROBE_Q38_RGDN_ROOT_RUN,f.c.f.job.get(),sizeof(*f.c.f.job));
  assert(status==(!fault?-99:B_OK));
  if (fault<=2) { f.Retained(); continue; }
  if (fault==3) { // premature END must NOT imply debug ACCEPT/close/reset
   unsigned touches=f.c.io.touches; assert(!f.End() && f.c.io.touches==touches); f.Retained();
  } else {
   assert(f.Accept()); failCopy=copyCalls+2; assert(!f.End()); f.Retained(); // successful CLOSE reply lost
  }
 }
 puts("PASS OFFLINE ACTUAL native resident entry+lock+call guard+real controller+RV32/SFPU:8 state transitions/explicit ACCEPT/clean CLOSE,10 inert DATA/parent/team refusals,lost RUN/CLOSE replies and native failures/premature END retain ALL owners with ZERO later IO;native IO scope separately tested,initial zero EXPLICIT counter fabric,NO native module/selection/performance claim");
}
#endif
