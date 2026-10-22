/* SPDX-License-Identifier: MIT */
// ACTUAL extracted native scope/retention/END gate + actual FFN owner,
// frozen zero/ROOT STOP/ALL119 boot instructions. Native context/mappings/
// clock/cookie are SIMULATED. This is NOT native MMIO or arithmetic proof.
#define PIONEER_Q38_PERSISTENT_GRAPH_FIXTURE_ONLY
#include "pioneer_q38_ffn_persistent_graph_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeArcClockGuard.h"
#define PIONEER_Q38_FFN_MODEL_INIT 1
#define PIONEER_Q38_NORMAL_CLOCK
using uint8=uint8_t;using uint32=uint32_t;
struct PioneerStoreContext;
struct PioneerHostPageIO{bool quarantined=false;};
struct PioneerSessionIO:PersistentGraphIO,PioneerHostPageIO {
 PioneerStoreContext* q38MutableContext=nullptr;
 unsigned runtimeCore=0;bool fleetEnabled=true,runtimeMapped=true,q38SpanAllowed=false,wideQuarantined=false,mapStable=true;
 struct DramAlias {
  const tt_dram_samples_result* target=nullptr;
  const tt_dram_samples_result* operator&()const{return target;}
  operator const tt_dram_samples_result&()const{assert(target);return *target;}
 } dram; // exact SAME proof reference,not a synthetic equal copy
 bool SelectRuntimeCore(unsigned w){bool okay=PersistentGraphIO::SelectRuntimeCore(w);if(okay)runtimeCore=w;return okay;}
 bool FfnGraphCodeMappingStable(unsigned w)const{return mapStable && w==runtimeCore && w<120;}// CPU-only like the real native mapping predicate
 bool FfnGraphScope(const PioneerQ38FfnPersistentGraphOwner&,const PioneerQ38GraphMutableBanks&,
  const PioneerQ38CanonicalModelPlan&,const PioneerQ38FfnIdentity&,const uint8_t*,unsigned);
 void FfnGraphRetainHostOwners();
 void QuarantineHostPage(){PioneerHostPageIO::quarantined=true;PersistentGraphIO::QuarantineHostPage();}
};
struct NativeGraphClock {
 tt_arc_clock_guard_result value={};bool required=true,quarantined=false;
 NativeGraphClock(){value.state=TT_ARC_CLOCK_BUSY_VERIFIED;value.busy_requested=value.busy_acknowledged=1;}
 const auto& Snapshot()const{return value;}
 bool RequiresExplicitEnd()const{return required;}
 void Quarantine(){quarantined=true;value.recovery_required=1;}
};
struct NativeGraphFlags{bool saved=false,dirty=false,ready=false,running=false,unsafe=false,awaiting=false,closed=false;};
using NativeGraphFixture=PersistentFixtureT<PioneerSessionIO>;
struct PioneerStoreContext {
 NativeGraphFixture& f;PioneerSessionIO& io;PioneerResidentBanks& banks;PioneerResidentLease& lease;
 PioneerBulkUploadLease& bulk;PioneerOutputBufferLease& bulkHost;PioneerWideOutputLease& wideHost;tt_host_page_result& host;
 PioneerQ38FfnPersistentGraphOwner* q38FfnGraph;PioneerQ38FfnGraphCodeOwner* q38FfnCode;PioneerQ38DispatchLease* q38Dispatch;
 PioneerQ38GraphMutableBanks* q38GraphMutable;PioneerQ38CanonicalModelPlan* q38FfnModelPlan;PioneerQ38FfnZeroLease* q38FfnZero;
 PioneerQ38FfnIdentity q38FfnModelIdentity;const uint8* q38FfnModelLedger;uint8* initialized;uint32 hash=0,*hashes=&hash;
 PioneerQ38GdnIdentity q38GdnIdentity={};PioneerQ38GdnBanks* q38GdnBanks=nullptr;PioneerQ38StateZeroLease* q38StateZero=nullptr;
 NativeGraphFlags span,q38Delta,panelProbe,streamProbe;NativeGraphFlags* q38Span=&span;
 void* q38Cohort=nullptr;void* fleet=nullptr;NativeGraphClock q38Clock;
 int q38GdnCallThread=42,runtimeCoreMode=1;bool dispatchOwnerRetained=false,bankStorageOnly=true,q38DispatchRoutes=true,otherUnsafe=false;
 uint64_t bytes=PioneerResidentBanks::Total;
 explicit PioneerStoreContext(NativeGraphFixture& v):f(v),io(v.io),banks(v.banks),lease(v.cohort->worker[0].Store()),
  bulk(v.zeroBulk),bulkHost(v.zeroTransfer),wideHost(v.zeroWide),host(v.zeroHost),q38FfnGraph(v.graphOwner.get()),
  q38FfnCode(v.code.get()),q38Dispatch(v.cohort.get()),q38GraphMutable(v.mutableOwner.get()),q38FfnModelPlan(v.plan.get()),
  q38FfnZero(&v.zeroLease),q38FfnModelIdentity(v.id),q38FfnModelLedger(v.learned.data()),initialized(v.learned.data()){
  span.closed=true;io.q38MutableContext=this;io.dram.target=&v.proof;
 }
 // Context outside the extracted scope is deliberately simulated. The
 // production full-context Unsafe is separately compiled in the driver.
 bool Unsafe(bool zero=false,bool graph=false)const{
  return otherUnsafe || banks.Quarantined() || q38Clock.quarantined || q38GraphMutable->Unsafe()
   || q38FfnZero->unsafe || (!zero && q38FfnZero->running)
   || (q38FfnGraph && !q38FfnGraph->Closed() && (!graph || !f.NativeMatches()));
 }
};
struct PioneerProbeCookie{PioneerStoreContext* context;uint32 magic=0x50313030;};
static PioneerProbeCookie* sRuntimeOwner=nullptr;
static bool sColdRecoveryRequired=false,sReloadQuiesced=false,sBankStorageOnly=true;
static uint64_t sReloadGeneration=4;
static const void* sQ38LocalDeltaOwner=nullptr;
static int currentThread=42;
static int find_thread(void*){return currentThread;}
#include "native_graph_retain.inc"
#include "native_graph_scope.inc"
static bool GraphEndGate(PioneerStoreContext& c){bool clean=true;tt_resident_session_result result={};
#include "native_graph_end.inc"
 return clean;
}
static void ResetNativeGlobals(PioneerProbeCookie& cookie){sRuntimeOwner=&cookie;sColdRecoveryRequired=sReloadQuiesced=false;
 sBankStorageOnly=true;sReloadGeneration=4;sQ38LocalDeltaOwner=nullptr;currentThread=42;}
int main(int argc,char** argv){
 assert(argc==4);GraphImage=argv[1];PeerImage=argv[2];setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;(void)&FreshInputs;
 unsigned refused=0;
 {NativeGraphFixture f(argv[3]);PioneerStoreContext c(f);PioneerProbeCookie cookie{&c};ResetNativeGlobals(cookie);assert(f.Prepare() && f.NativeMatches());
  auto scope=[&](){return f.io.FfnGraphScope(*f.graphOwner,*f.mutableOwner,*f.plan,f.id,f.learned.data(),unsigned(f.learned.size()));};
  assert(scope());const unsigned touches=f.io.touches;
  for(unsigned which=0;which<45;which++){
   auto test=[&](auto& field,auto bad){auto before=field;field=bad;assert(!scope() && f.io.touches==touches);field=before;};
   switch(which){
    case 0:test(sColdRecoveryRequired,true);break;case 1:test(sReloadQuiesced,true);break;case 2:test(sReloadGeneration,uint64_t(5));break;
    case 3:test(cookie.magic,0U);break;case 4:test(cookie.context,static_cast<PioneerStoreContext*>(nullptr));break;
    case 5:test(sRuntimeOwner,static_cast<PioneerProbeCookie*>(nullptr));break;case 6:test(sBankStorageOnly,false);break;
    case 7:test(c.bankStorageOnly,false);break;case 8:test(c.bytes,uint64_t(0));break;case 9:test(c.runtimeCoreMode,0);break;
    case 10:test(f.io.fleetEnabled,false);break;case 11:test(f.io.runtimeMapped,false);break;case 12:test(f.io.q38SpanAllowed,true);break;
    case 13:test(c.q38GdnCallThread,-1);break;case 14:test(currentThread,41);break;case 15:test(c.dispatchOwnerRetained,true);break;
    case 16:test(c.q38FfnGraph,static_cast<PioneerQ38FfnPersistentGraphOwner*>(nullptr));break;
    case 17:test(c.q38FfnModelLedger,c.q38FfnModelLedger+1);break;case 18:test(c.initialized,static_cast<uint8*>(nullptr));break;
    case 19:test(c.hashes,static_cast<uint32*>(nullptr));break;case 20:test(c.q38DispatchRoutes,false);break;
    case 21:test(c.q38Cohort,static_cast<void*>(&c));break;case 22:test(c.fleet,static_cast<void*>(&c));break;
    case 23:test(sQ38LocalDeltaOwner,static_cast<const void*>(&c));break;case 24:test(c.q38Delta.ready,true);break;
    case 25:test(c.panelProbe.saved,true);break;case 26:test(c.streamProbe.dirty,true);break;case 27:test(c.span.closed,false);break;
    case 28:test(c.lease.ready,false);break;case 29:test(c.bulk.ready,false);break;case 30:test(c.host.released,1U);break;
    case 31:test(c.bulkHost.pending,true);break;case 32:test(c.wideHost.memory_type_set,false);break;
    case 33:test(f.io.PioneerHostPageIO::quarantined,true);break;case 34:test(f.io.wideQuarantined,true);break;
    case 35:test(f.io.mapStable,false);break;case 36:test(c.q38FfnZero->closed,false);break;case 37:test(c.q38FfnZero->nonce,2U);break;
    case 38:test(c.q38FfnZero->dirty,true);break;case 39:test(c.q38Clock.required,false);break;case 40:test(c.q38Clock.value.busy_acknowledged,0U);break;
    case 41:test(c.q38Clock.value.restored,1U);break;case 42:test(c.otherUnsafe,true);break;
    case 43:test(f.cohort->worker[60].image.running,false);break;case 44:test(f.cohort->booted,true);break;
   }
   assert(scope() && f.io.touches==touches);refused++;
  }
  assert(f.Close() && GraphEndGate(c));PersistentOuterEnd(f);
 }
 for(unsigned reason=0;reason<2;reason++){
  NativeGraphFixture f(argv[3]);PioneerStoreContext c(f);PioneerProbeCookie cookie{&c};ResetNativeGlobals(cookie);assert(f.Prepare());
  unsigned before=f.io.touches;if(reason)c.q38GdnCallThread=-1;
  if(reason)assert(!f.Run());else assert(!GraphEndGate(c));
  assert(f.io.touches==before && sColdRecoveryRequired && c.dispatchOwnerRetained && c.q38Clock.quarantined
   && f.banks.Quarantined() && f.mutableOwner->Unsafe() && f.cohort->unsafe && c.bulkHost.quarantined && c.wideHost.quarantined
   && f.io.PioneerHostPageIO::quarantined && !f.io.rootCount && !f.result.rootHolds);
  assert(!f.Run() && !f.Close() && !GraphEndGate(c) && f.io.touches==before);
 }
 printf("PASS ACTUAL native persistent-FFN scope/retention/END gate:%u metadata refusals ZERO IO,actual private owner+zero publication/119boots/ROOT STOP/outer restoration,unclosed END or lost real call seal retains clock+ALL hosts/cohort/banks with ZERO further IO;SIMULATED_NATIVE_CONTEXT_NOT_FULL_NATIVE_FACTORY_MMIO_OR_SPEED\n",refused);
}
