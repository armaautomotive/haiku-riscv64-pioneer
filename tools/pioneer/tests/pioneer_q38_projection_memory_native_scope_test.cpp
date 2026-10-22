/* SPDX-License-Identifier: MIT */
// ACTUAL extracted native scope/Unsafe/retention/END + full memory owner
// and ACTUAL frozen zero controller. Native contexts/endpoints simulated.
#define PIONEER_Q38_PERSISTENT_GRAPH_FIXTURE_ONLY
#include "pioneer_q38_ffn_persistent_graph_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGraphMemory.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeArcClockGuard.h"
#define PIONEER_Q38_PROJECTION_GRAPH_MEMORY 1
#define PIONEER_Q38_FFN_MODEL_INIT 1
#define PIONEER_Q38_NORMAL_CLOCK
using uint8=uint8_t;using uint32=uint32_t;
struct PioneerStoreContext;
struct PioneerHostPageIO{bool quarantined=false;};
struct PioneerSessionIO:PersistentGraphIO,PioneerHostPageIO {
 PioneerStoreContext* q38MutableContext=nullptr;
 unsigned runtimeCore=0;bool fleetEnabled=true,runtimeMapped=true,q38SpanAllowed=false,wideQuarantined=false,mapStable=true;
 bool q38ProjectionMemoryMapAttempted=true;
 struct DramAlias {const tt_dram_samples_result* target=nullptr;const tt_dram_samples_result* operator&()const{return target;}
  operator const tt_dram_samples_result&()const{assert(target);return *target;}} dram;
 std::vector<uint32_t> data=std::vector<uint32_t>(8134072,0xfedcba98);
 bool SelectRuntimeCore(unsigned w){bool okay=PersistentGraphIO::SelectRuntimeCore(w);if(okay)runtimeCore=w;return okay;}
 bool ProjectionGraphMemoryMappingStable(unsigned w)const{return mapStable && w==runtimeCore && w<120;}
 static unsigned Index(unsigned w,uint32_t a){
  unsigned at=0;for(unsigned i=0;i<PioneerQ38ProjectionGraphSpanCount(w);i++){
   auto s=PioneerQ38ProjectionGraphSnapshotSpan(w,i);if(a>=s.address && a-s.address<s.words*4)return PioneerQ38ProjectionGraphSnapshotBase(w)+at+(a-s.address)/4;at+=s.words;
  }assert(false);return 0;
 }
 bool ProjectionGraphMemoryReadChecked(unsigned w,uint32_t a,uint32_t& value){
  Touch();assert(w==current && PioneerQ38ProjectionGraphSnapshotContains(w,a));
  value=a==0x6000?BootstrapIO::RuntimeCodeRead(0):a>=0x70000 && a<0x74000?BootstrapIO::Q38PanelRead(a):data.at(Index(w,a));return true;
 }
 bool ProjectionGraphMemoryWrite(unsigned w,uint32_t a,uint32_t value){
  Touch();assert(w==current && PioneerQ38ProjectionGraphSnapshotContains(w,a));
  if(a==0x6000)BootstrapIO::RuntimeCodeWrite(0,value);else if(a>=0x70000 && a<0x74000)BootstrapIO::Q38PanelWrite(a,value);else data.at(Index(w,a))=value;return true;
 }
 bool ProjectionGraphMemoryScope(const PioneerQ38ProjectionGraphMemory&,const PioneerQ38CanonicalModelPlan&,const PioneerQ38GdnIdentity&,const uint8_t*,unsigned);
 void ProjectionGraphRetainHostOwners();
 void QuarantineHostPage(){PioneerHostPageIO::quarantined=true;PersistentGraphIO::QuarantineHostPage();}
};
struct NativeMemoryClock {
 tt_arc_clock_guard_result value={};bool required=true,quarantined=false;
 NativeMemoryClock(){value.state=TT_ARC_CLOCK_BUSY_VERIFIED;value.busy_requested=value.busy_acknowledged=1;}
 const auto& Snapshot()const{return value;}bool RequiresExplicitEnd()const{return required;}
 void Quarantine(){quarantined=true;value.recovery_required=1;}
};
struct NativeMemoryFlags{bool saved=false,dirty=false,ready=false,running=false,unsafe=false,awaiting=false,closed=false;};
using SourceFixture=PersistentFixtureT<PioneerSessionIO>;
struct PioneerStoreContext {
 SourceFixture& f;PioneerSessionIO& io;PioneerResidentBanks& banks;PioneerResidentLease& lease;
 PioneerBulkUploadLease& bulk;PioneerOutputBufferLease& bulkHost;PioneerWideOutputLease& wideHost;tt_host_page_result& host;
 PioneerQ38ProjectionGraphMemory* q38ProjectionMemory;
 struct D{tt_dram_samples_result& dram;};struct B1{D baseline;};struct B2{B1 baseline;};struct B3{B2 baseline;};B3 baseline;
 PioneerQ38FfnPersistentGraphOwner* q38FfnGraph=nullptr;PioneerQ38FfnGraphCodeOwner* q38FfnCode=nullptr;PioneerQ38DispatchLease* q38Dispatch;
 PioneerQ38GraphMutableBanks* q38GraphMutable;PioneerQ38CanonicalModelPlan* q38FfnModelPlan;PioneerQ38FfnZeroLease* q38FfnZero;
 PioneerQ38GdnIdentity q38FfnModelIdentity;const uint8* q38FfnModelLedger;uint8* initialized;uint32 hash=0,*hashes=&hash;
 PioneerQ38GdnIdentity q38GdnIdentity={};PioneerQ38GdnBanks* q38GdnBanks=nullptr;PioneerQ38StateZeroLease* q38StateZero=nullptr;
 NativeMemoryFlags span,q38Delta,panelProbe,streamProbe;NativeMemoryFlags* q38Span=&span;
 void* q38Cohort=nullptr;void* fleet=nullptr;NativeMemoryClock q38Clock;
 int q38GdnCallThread=42,runtimeCoreMode=1;bool dispatchOwnerRetained=false,bankStorageOnly=true,q38DispatchRoutes=true,otherUnsafe=false;
 uint64_t bytes=PioneerResidentBanks::Total;
 PioneerStoreContext(SourceFixture& v,PioneerQ38ProjectionGraphMemory& memory):f(v),io(v.io),banks(v.banks),lease(v.cohort->worker[0].Store()),
  bulk(v.zeroBulk),bulkHost(v.zeroTransfer),wideHost(v.zeroWide),host(v.zeroHost),q38ProjectionMemory(&memory),baseline{{{{v.proof}}}},
  q38Dispatch(v.cohort.get()),q38GraphMutable(v.mutableOwner.get()),q38FfnModelPlan(v.plan.get()),q38FfnZero(&v.zeroLease),
  q38FfnModelIdentity(v.id),q38FfnModelLedger(v.learned.data()),initialized(v.learned.data()){
  span.closed=true;io.q38MutableContext=this;io.dram.target=&v.proof;
 }
 bool Unsafe(bool zero=false,bool graph=false,bool controlledProjectionMemory=false){
  (void)graph;
#include "projection_memory_unsafe.inc"
  return otherUnsafe || banks.Quarantined() || q38Clock.quarantined || q38GraphMutable->Unsafe()
   || q38FfnZero->unsafe || (!zero && q38FfnZero->running);
 }
};
struct PioneerProbeCookie{PioneerStoreContext* context;uint32 magic=0x50313030;};
static PioneerProbeCookie* sRuntimeOwner=nullptr;
static bool sColdRecoveryRequired=false,sReloadQuiesced=false,sBankStorageOnly=true;
static uint64_t sReloadGeneration=4;static const void* sQ38LocalDeltaOwner=nullptr;
static int currentThread=42;static int find_thread(void*){return currentThread;}
#include "projection_memory_retain.inc"
#include "projection_memory_scope.inc"
static bool MemoryEndGate(PioneerStoreContext& c){bool clean=true;tt_resident_session_result result={};
#include "projection_memory_end.inc"
 return clean;
}
static void Globals(PioneerProbeCookie& cookie){sRuntimeOwner=&cookie;sColdRecoveryRequired=sReloadQuiesced=false;sBankStorageOnly=true;sReloadGeneration=4;sQ38LocalDeltaOwner=nullptr;currentThread=42;}
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;(void)&FreshInputs;
 unsigned refusals=0;
 {SourceFixture f(argv[1]);auto memory=std::make_unique<PioneerQ38ProjectionGraphMemory>();PioneerQ38ProjectionGraphMemoryResult r;
  PioneerStoreContext c(f,*memory);PioneerProbeCookie cookie{&c};Globals(cookie);
  assert(memory->PrepareHeld(f.io,f.proof,f.banks,*f.cohort,*f.plan,f.id,f.learned.data(),unsigned(f.learned.size()),r));
  auto scope=[&](){return f.io.ProjectionGraphMemoryScope(*memory,*f.plan,f.id,f.learned.data(),unsigned(f.learned.size()));};
  assert(scope());unsigned before=f.io.touches;
  for(unsigned which=0;which<52;which++){
   auto test=[&](auto& field,auto bad){auto saved=field;field=bad;assert(!scope() && f.io.touches==before);field=saved;};
   switch(which){
    case 0:test(sColdRecoveryRequired,true);break;case 1:test(sReloadQuiesced,true);break;case 2:test(sReloadGeneration,uint64_t(5));break;
    case 3:test(cookie.magic,0U);break;case 4:test(cookie.context,static_cast<PioneerStoreContext*>(nullptr));break;
    case 5:test(sRuntimeOwner,static_cast<PioneerProbeCookie*>(nullptr));break;case 6:test(sBankStorageOnly,false);break;
    case 7:test(c.bankStorageOnly,false);break;case 8:test(c.bytes,uint64_t(0));break;case 9:test(c.runtimeCoreMode,0);break;
    case 10:test(f.io.fleetEnabled,false);break;case 11:test(f.io.runtimeMapped,false);break;case 12:test(f.io.q38SpanAllowed,true);break;
    case 13:test(c.q38GdnCallThread,-1);break;case 14:test(currentThread,41);break;case 15:test(c.dispatchOwnerRetained,true);break;
    case 16:test(c.q38ProjectionMemory,static_cast<PioneerQ38ProjectionGraphMemory*>(nullptr));break;
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
    case 41:test(c.q38Clock.value.restored,1U);break;case 42:test(c.otherUnsafe,true);break;case 43:test(f.cohort->worker[60].image.running,true);break;
    case 44:test(f.cohort->booted,true);break;case 45:test(c.q38FfnGraph,f.graphOwner.get());break;case 46:test(c.q38FfnCode,f.code.get());break;
    case 47:test(c.q38FfnModelPlan,static_cast<PioneerQ38CanonicalModelPlan*>(nullptr));break;
    case 48:test(c.q38FfnModelIdentity.session,UINT64_C(2));break;case 49:test(c.q38FfnModelIdentity.sequence,UINT64_C(3));break;
    case 50:test(c.q38FfnModelIdentity.modelEpoch,UINT64_C(4));break;case 51:test(c.q38FfnModelIdentity.driverGeneration,UINT64_C(5));break;
   }
   assert(scope() && f.io.touches==before);refusals++;
  }
  assert(memory->CloseHeldNoLaunch(f.io,r) && MemoryEndGate(c) && !c.Unsafe());
 }
 for(bool lostCall:{false,true}){
  SourceFixture f(argv[1]);auto memory=std::make_unique<PioneerQ38ProjectionGraphMemory>();PioneerQ38ProjectionGraphMemoryResult r;
  PioneerStoreContext c(f,*memory);PioneerProbeCookie cookie{&c};Globals(cookie);
  assert(memory->PrepareHeld(f.io,f.proof,f.banks,*f.cohort,*f.plan,f.id,f.learned.data(),unsigned(f.learned.size()),r));unsigned before=f.io.touches;
  if(lostCall){c.q38GdnCallThread=-1;assert(!memory->CloseHeldNoLaunch(f.io,r));}else assert(!MemoryEndGate(c));
  assert(f.io.touches==before && memory->Unsafe() && c.dispatchOwnerRetained && sColdRecoveryRequired && c.q38Clock.quarantined
   && c.banks.Quarantined() && c.bulkHost.quarantined && c.wideHost.quarantined && f.io.PioneerHostPageIO::quarantined
   && !memory->CloseHeldNoLaunch(f.io,r) && !MemoryEndGate(c) && f.io.touches==before);
 }
 printf("PASS ACTUAL native projection memory scope/Unsafe/retention/END:%umetadata refusals ZERO IO,real canonical joint owner+ACTUAL frozen74page zero publication+ALL120 held/full memory install+restore;lost call/unclosed END retains full memory+clock+ALL host/source/cohort owners with ZERO following card IO;NOT_GENERIC_WORKSPACE_INIT_INPUT_PRODUCER_NATIVE_EXECUTION_OR_SPEED\n",refusals);
}
