/* SPDX-License-Identifier: MIT */
// Verbatim native binding/call-scope/WHOLE shared lock/Unsafe/retention/END.
// OS, clock and unused foreign owners are TEST metadata endpoints only.
// Actual canonical plan, learned-page checks and atomic bank claims are used.
// NO zero/init, code upload, launch, MMIO, original-model or hardware proof.
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeState.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeArcClockGuard.h"
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#include <memory>
#include <type_traits>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;
using int64_t=std::int64_t;using status_t=int;using thread_id=int;
static constexpr int B_OK=0,B_BAD_VALUE=-1,B_NOT_ALLOWED=-2,B_DEV_NOT_READY=-3,B_BUSY=-4;
static constexpr unsigned TT_PIONEER_MMIO_RESTORE_FAILED=5; // simulated PCI-result endpoint enum
struct thread_info{int team=-1;};
static int currentThread=17,currentTeam=23,lockHeld=0,lockAcquires=0,lockReleases=0;
static bool failThreadInfo=false;static status_t lockStatus=B_OK;
static thread_id find_thread(const char*){return currentThread;}
static status_t get_thread_info(thread_id thread,thread_info* out){
 if(failThreadInfo || thread!=currentThread)return B_BAD_VALUE;
 out->team=currentTeam;return B_OK;
}
static int sLock=1;
static status_t acquire_sem(int lock){
 assert(lock==sLock);if(lockStatus!=B_OK)return lockStatus;if(lockHeld)return B_BUSY;
 lockHeld=1;lockAcquires++;return B_OK;
} // ONLY the OS semaphore syscall is simulated;the production lock is below
static status_t release_sem(int lock){assert(lock==sLock && lockHeld==1);lockHeld=0;lockReleases++;return B_OK;}
struct PioneerProbeCookie{uint32 magic=0x50313030;PioneerStoreContext* context=nullptr;int team=23;};
static PioneerProbeCookie* sRuntimeOwner=nullptr;
static bool sColdRecoveryRequired=false,sReloadQuiesced=false,sBankStorageOnly=true;
static uint64 sReloadGeneration=UINT64_C(0x100000007);
struct MockLease {
 bool saved=false,ready=false,pending=false,recovery_required=false,dirty=false,unsafe=false;
 bool running=false,awaiting=false,closed=false;
};
struct MockHost{bool allocated=true,ready=true,pending=false,quarantined=false,released=false,memory_type_set=true;};
struct MockForeignOwner:MockLease {
 bool Unsafe()const{return unsafe;}
 bool Closed()const{return false;}
 void Fault(){unsafe=true;}
 template<class... Args>bool MatchesNativeOwner(const Args&...)const{return false;}
}; // foreign owners NEVER succeed or provide initialization/producer authority
struct MockFleet {MockLease compute,store,noc;MockLease& Compute(){return compute;}MockLease& Store(){return store;}};
struct MockCohort {bool unsafe=false;struct Node{MockLease noc,image,store;MockLease& Store(){return store;}}worker[120];};
struct MockClock {
 struct Value{uint32 state=TT_ARC_CLOCK_BUSY_VERIFIED,busy_requested=1,busy_acknowledged=1,restored=0,busy_mhz=1400;}value;
 bool required=true,quarantined=false;
 bool Unsafe()const{return quarantined;}
 bool RequiresExplicitEnd()const{return required;}
 const Value& Snapshot()const{return value;}
 void Quarantine(){quarantined=true;}
};
struct PioneerHostPageIO{bool quarantined=false;};
struct PioneerSessionIO:PioneerHostPageIO {
 tt_dram_samples_result& dram;
 explicit PioneerSessionIO(tt_dram_samples_result& actual):dram(actual){}
 PioneerStoreContext* q38MutableContext=nullptr;unsigned runtimeCore=0;
 bool fleetEnabled=false,runtimeMapped=true,q38SpanAllowed=true,wideQuarantined=false;
 bool TokenCallScopeMatches();bool TokenModelSourceScope(bool);
 void QuarantineHostPage(){quarantined=true;}
};
struct PioneerStoreContext {
 struct D{tt_dram_samples_result dram={};};struct B1{D baseline;MockLease noc;};
 struct B2{B1 baseline;MockLease path;};struct B3{B2 baseline;MockLease audit;};B3 baseline;
 PioneerSessionIO io{baseline.baseline.baseline.baseline.dram};PioneerResidentBanks banks;
 struct{uint32 state=0;}pci;
 PioneerQ38TokenNativeState* q38TokenNative=nullptr;
 PioneerQ38CanonicalModelPlan* q38FfnModelPlan=nullptr;
 MockForeignOwner *q38GraphMutable=nullptr,*q38FfnZero=nullptr,*q38FfnGraph=nullptr,*q38FfnCode=nullptr;
 MockForeignOwner *q38GdnBanks=nullptr,*q38StateZero=nullptr;
 PioneerQ38DispatchLease* q38Dispatch=nullptr;MockCohort* q38Cohort=nullptr;MockFleet* fleet=nullptr;
 PioneerQ38GdnIdentity q38FfnModelIdentity={},q38GdnIdentity={};
 MockLease q38Delta,panelProbe,streamProbe,span,lease,bulk,secondLease,secondNoc,idle;
 MockLease peerAttention[8],decoder,layer,coordinator,attention,compute,secondCompute;
 MockLease* q38Span=&span;MockHost bulkHost,wideHost;MockClock q38Clock;
 tt_host_page_result host={};uint8* initialized=nullptr;uint32* hashes=nullptr;
 thread_id q38GdnCallThread=-1;uint64 bytes=PioneerResidentBanks::Total;
 bool bankStorageOnly=true,dispatchOwnerRetained=false,q38DispatchRoutes=false;unsigned q38EndPhase=0;int runtimeCoreMode=-1;
 auto& Pci(){return pci;}
#include "token_native_unsafe.inc"
 ~PioneerStoreContext(){delete q38TokenNative;} // CPU-only fixture disposal; NO live device resources exist
};
#include "token_native_lock.inc"
static void TestKernelLog(const char*,...){}
#define dprintf TestKernelLog
#include "token_native_retain.inc"
#undef dprintf
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeBind.inc"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenCallScope.inc"
static bool IncompleteEnd(PioneerStoreContext& c,tt_resident_session_result& result,bool& clean){
#include "token_native_end.inc"
 return true; // fixture fallback reached ONLY with no new token owner
}
static_assert(!std::is_default_constructible<PioneerQ38TokenNativeState>::value
 && !std::is_destructible<PioneerQ38TokenNativeState>::value
 && !std::is_copy_constructible<PioneerQ38TokenNativeState>::value,"native factory/lifetime is private");
struct Fixture {
 BanksMock endpoint;
 std::unique_ptr<PioneerStoreContext> c=std::make_unique<PioneerStoreContext>();
 std::unique_ptr<PioneerQ38CanonicalModelPlan> plan=std::make_unique<PioneerQ38CanonicalModelPlan>();
 std::vector<uint8> learned=std::vector<uint8>(PioneerResidentBanks::Pages);
 uint32 hash=0x12345678;PioneerProbeCookie cookie;
 Fixture(){
  assert(!lockHeld);sColdRecoveryRequired=false;sReloadQuiesced=false;sBankStorageOnly=true;
  sQ38LocalDeltaOwner=nullptr;currentThread=17;currentTeam=23;failThreadInfo=false;lockStatus=B_OK;
  cookie.context=c.get();sRuntimeOwner=&cookie;c->io.q38MutableContext=c.get();
  c->baseline.baseline.baseline.baseline.dram=BankProof(endpoint);
  assert(c->banks.Bind(c->baseline.baseline.baseline.baseline.dram) && plan->Build());
  for(unsigned i=0;i<plan->Count();i++){const auto* p=plan->Part(i);if(p->kind!=plan->WeightKind)continue;
   unsigned first=p->bank*unsigned(plan->Capacity/4096)+p->offset/4096;
   for(unsigned j=0;j<p->allocated/4096;j++)learned[first+j]=1;
  }
  c->initialized=learned.data();c->hashes=&hash;c->span.closed=true;c->lease.saved=true;c->lease.ready=true;
  c->bulk.saved=true;c->bulk.ready=true;c->host.state=TT_HOST_PAGE_READY;
 }
 const tt_dram_samples_result& Dram()const{return c->baseline.baseline.baseline.baseline.dram;}
 bool Bind(unsigned bank=0){PioneerQ38TokenCallScope scope(&cookie);return scope.Context() && BindQ38TokenNative(*scope.Context(),bank);}
 bool Binding(){PioneerQ38TokenCallScope scope(&cookie);return scope.Context() && scope.Context()->io.TokenModelSourceScope(false);}
 void Inert()const{assert(!endpoint.Mock::writes && !endpoint.requests && !lockHeld && c->q38GdnCallThread==-1);}
 void Retained()const{
  assert(c->q38TokenNative && c->q38TokenNative->Unsafe() && !c->q38TokenNative->Closed()
   && c->dispatchOwnerRetained && c->banks.Quarantined() && c->q38Clock.quarantined && c->bulk.unsafe
   && c->io.quarantined && c->bulkHost.quarantined && c->wideHost.quarantined
   && !c->bulkHost.released && !c->wideHost.released && sColdRecoveryRequired);
 }
};
int main(){
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;
 setvbuf(stdout,nullptr,_IOLBF,0);unsigned refused=0,positive=0;
 for(unsigned bank=0;bank<7;bank++){
  Fixture f;auto& c=*f.c;const auto before=f.learned;const auto epoch=sQ38TokenModelEpoch;
  assert(f.Bind(bank) && sQ38TokenModelEpoch==epoch+1 && c.q38TokenNative && !c.q38TokenNative->Unsafe()
   && !c.q38TokenNative->Closed() && c.Unsafe() && !c.Unsafe(false,false,true) && f.Binding()
   && c.q38TokenNative->MatchesReservation(c.banks,f.Dram()) && f.learned==before);
  // All seven full runtime suffixes,not only the selected workspace,are
  // protected against ordinary learned uploads by the ACTUAL reservation.
  PioneerResidentMutableRange runtime[7];assert(PioneerQ38RuntimeRanges(*f.plan,runtime));
  for(const auto& r:runtime)for(unsigned page:{r.firstPage,r.firstPage+r.pages-1}){
   uint32 address=9,route=8,index=7;
   assert(!c.banks.Page(f.Dram(),r.bank,uint64(page)*4096,address,route,index) && address==9 && route==8 && index==7);
  }
  assert(!f.Bind(bank) && sQ38TokenModelEpoch==epoch+1);refused++;
  assert(Q38MutableScopeOwned(&f.cookie));
  assert(AcquireProbeLock(&f.cookie,true)==B_NOT_ALLOWED && !lockHeld);
  assert(AcquireQ38DeltaLock(&f.cookie)==B_NOT_ALLOWED && !lockHeld);
  assert(AcquireQ38GdnSessionLock(&f.cookie)==B_OK && lockHeld);release_sem(sLock);refused+=2;
  auto* state=c.q38TokenNative;auto copy=f.cookie;uint32 otherHash=0;
  assert(!state->MatchesBinding(c,&copy,c.hashes,c.initialized,f.cookie.team,sReloadGeneration)
   && !state->MatchesBinding(c,&f.cookie,&otherHash,c.initialized,f.cookie.team,sReloadGeneration)
   && !state->MatchesBinding(c,&f.cookie,c.hashes,nullptr,f.cookie.team,sReloadGeneration)
   && !state->MatchesBinding(c,&f.cookie,c.hashes,c.initialized,f.cookie.team+1,sReloadGeneration)
   && !state->MatchesBinding(c,&f.cookie,c.hashes,c.initialized,f.cookie.team,sReloadGeneration+1));refused+=5;
  tt_resident_session_result result={};bool clean=true;
  {PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
   assert(!IncompleteEnd(c,result,clean) && !clean && state==c.q38TokenNative);}
  f.Retained();assert(!f.Bind(bank) && !f.Binding());f.Inert();positive++;
 }
 puts("PASS genuine production bind/7banks/all7runtime suffixes reserved/unchanged learned ledger/private retained lifetime/incomplete END quarantines ALL hosts/NO device IO");
 {
  Fixture f;auto& c=*f.c;PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
  const auto epoch=sQ38TokenModelEpoch;
  for(unsigned i=0;i<f.plan->Count();i++){
   const auto* p=f.plan->Part(i);if(p->kind!=f.plan->WeightKind)continue;
   unsigned page=p->bank*unsigned(f.plan->Capacity/4096)+p->offset/4096+p->allocated/4096-1;
   const auto saved=f.learned[page];f.learned[page]=uint8(0);
   assert(!BindQ38TokenNative(c,0) && !c.q38TokenNative && sQ38TokenModelEpoch==epoch && !c.dispatchOwnerRetained);
   f.learned[page]=saved;refused++;
  }
  for(unsigned bad:{2U,255U}){f.learned[0]=uint8(bad); // embedding begins at canonical page zero
   assert(!BindQ38TokenNative(c,0) && !c.q38TokenNative && sQ38TokenModelEpoch==epoch);
   f.learned[0]=1;refused++;
  }
  puts("PASS full851 learned-page preclaim refusals/all3551 extent LAST pages/no identity consumed/no reservations/no device IO");
 }
 {
  Fixture f;auto& c=*f.c;PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
  auto refuse=[&](auto& field,auto bad){auto saved=field;field=bad;auto epoch=sQ38TokenModelEpoch;
   assert(!c.io.TokenModelSourceScope(true) && !BindQ38TokenNative(c,0) && !c.q38TokenNative
    && sQ38TokenModelEpoch==epoch);field=saved;refused++;};
  refuse(sRuntimeOwner,nullptr);refuse(f.cookie.context,nullptr);refuse(f.cookie.magic,0U);refuse(f.cookie.team,-1);
  refuse(currentThread,99);refuse(currentTeam,99);refuse(failThreadInfo,true);refuse(c.q38GdnCallThread,-1);
  refuse(sColdRecoveryRequired,true);refuse(sReloadQuiesced,true);refuse(sReloadGeneration,UINT64_C(0));
  refuse(sBankStorageOnly,false);refuse(c.bankStorageOnly,false);refuse(c.bytes,c.bytes-4096);
  refuse(c.runtimeCoreMode,1);refuse(c.io.runtimeCore,1U);refuse(c.io.fleetEnabled,true);refuse(c.io.runtimeMapped,false);
  refuse(c.io.q38SpanAllowed,false);refuse(c.dispatchOwnerRetained,true);refuse(c.initialized,nullptr);refuse(c.hashes,nullptr);
  refuse(c.io.q38MutableContext,nullptr);refuse(c.q38Span,nullptr);refuse(c.span.closed,false);
  refuse(c.span.dirty,true);refuse(c.span.ready,true);refuse(c.span.running,true);refuse(c.span.unsafe,true);
  refuse(c.lease.saved,false);refuse(c.lease.ready,false);refuse(c.lease.pending,true);refuse(c.lease.recovery_required,true);
  refuse(c.bulk.saved,false);refuse(c.bulk.ready,false);refuse(c.bulk.dirty,true);refuse(c.bulk.unsafe,true);
  refuse(c.host.state,0U);refuse(c.host.released,1U);refuse(c.io.quarantined,true);
  for(auto* h:{&c.bulkHost,&c.wideHost}){refuse(h->allocated,false);refuse(h->ready,false);refuse(h->pending,true);refuse(h->quarantined,true);refuse(h->released,true);}
  refuse(c.wideHost.memory_type_set,false);refuse(c.io.wideQuarantined,true);
  refuse(c.q38Clock.required,false);refuse(c.q38Clock.quarantined,true);refuse(c.q38Clock.value.state,0U);
  refuse(c.q38Clock.value.busy_requested,0U);refuse(c.q38Clock.value.busy_acknowledged,0U);refuse(c.q38Clock.value.restored,1U);
  refuse(c.q38Clock.value.busy_mhz,0U);refuse(c.q38Clock.value.busy_mhz,1401U);
  MockForeignOwner other;
  for(auto* owner:{&c.q38GraphMutable,&c.q38FfnZero,&c.q38FfnGraph,&c.q38FfnCode,&c.q38GdnBanks,&c.q38StateZero})refuse(*owner,&other);
  refuse(c.q38FfnModelPlan,f.plan.get());refuse(c.q38DispatchRoutes,true);refuse(sQ38LocalDeltaOwner,static_cast<const void*>(&f));
  for(auto* id:{&c.q38FfnModelIdentity,&c.q38GdnIdentity}){
   refuse(id->session,UINT64_C(1));refuse(id->sequence,UINT64_C(1));refuse(id->modelEpoch,UINT64_C(1));refuse(id->driverGeneration,UINT64_C(1));}
  for(auto* lease:{&c.q38Delta,&c.panelProbe,&c.streamProbe}){refuse(lease->saved,true);refuse(lease->dirty,true);refuse(lease->ready,true);}
  refuse(c.q38Delta.running,true);refuse(c.q38Delta.awaiting,true);
  auto epoch=sQ38TokenModelEpoch;sQ38TokenModelEpoch=UINT64_MAX;
  assert(!BindQ38TokenNative(c,0) && !c.q38TokenNative && sQ38TokenModelEpoch==UINT64_MAX);sQ38TokenModelEpoch=epoch;refused++;
 }
 {
  Fixture f;PioneerResidentMutableRange runtime[7];assert(PioneerQ38RuntimeRanges(*f.plan,runtime));
  // Valid learned bytes but a conflicting mutable-page upload:actual
  // atomic reservation fails AFTER private publication and retains all.
  f.learned[runtime[6].bank*786432+runtime[6].firstPage+runtime[6].pages-1]=1;
  assert(!f.Bind() && f.c->q38TokenNative);f.Retained();f.Inert();
 }
 {
  Fixture f;assert(f.Bind());sReloadGeneration++;
  assert(!f.Binding());f.Retained();f.Inert();sReloadGeneration--;
 }
 {
  Fixture f;lockStatus=B_BUSY;auto acquired=lockAcquires,released=lockReleases;
  assert(!f.Bind() && lockAcquires==acquired && lockReleases==released && !f.c->q38TokenNative);f.Inert();
 }
 {
  Fixture f;auto& c=*f.c;
  {PioneerQ38TokenCallScope invalid(nullptr);assert(!invalid.Context() && invalid.Status()==B_BAD_VALUE);}
  auto refuse=[&](auto& field,auto bad){auto saved=field;field=bad;
   {PioneerQ38TokenCallScope invalid(&f.cookie);assert(!invalid.Context() && invalid.Status()!=B_OK);}
   field=saved;assert(!c.q38TokenNative && !c.dispatchOwnerRetained && !lockHeld);refused++;
  };
  refuse(f.cookie.magic,0U);refuse(currentThread,-1);refuse(currentTeam,99);refuse(failThreadInfo,true);
  refuse(f.cookie.context,nullptr);refuse(sBankStorageOnly,false);refuse(c.bankStorageOnly,false);
  refuse(c.bytes,c.bytes-4096);refuse(c.io.q38MutableContext,nullptr);refuse(c.q38GdnCallThread,17);
  {PioneerQ38TokenCallScope first(&f.cookie);assert(first.Context()==&c && lockHeld);
   auto releases=lockReleases;PioneerQ38TokenCallScope reentrant(&f.cookie);
   assert(!reentrant.Context() && reentrant.Status()==B_BUSY && lockHeld && lockReleases==releases);refused++;
  }
  f.Inert();
 }
 {
  Fixture f;assert(f.Bind());auto& c=*f.c;c.compute.unsafe=true;
  assert(c.Unsafe(false,false,true) && !f.Binding()); // controlledToken does not hide OTHER owners
  f.Retained();f.Inert();refused++;
 }
 {
  Fixture f;assert(f.Bind());auto& c=*f.c;auto acquired=lockAcquires,released=lockReleases;
  {PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
   sRuntimeOwner=nullptr; // lost actual FD owner at scope exit,not a fabricated completion flag
  }
  assert(lockAcquires==acquired+1 && lockReleases==released+1 && !lockHeld && c.q38GdnCallThread==currentThread);
  f.Retained();assert(!f.endpoint.Mock::writes && !f.endpoint.requests);refused++;
 }
 assert(lockAcquires==lockReleases && !lockHeld);
 printf("PASS production full851 token binding/WHOLE actual shared-lock ordinary+delta exclusion/SAME-owner session reentry/native scope/retention/Unsafe/incomplete-END banks%u/refusals%u/epoch wrap+stale generation+atomic claim failure/NO following device IO;OS+clock+unused foreign owners SIMULATED;NOT init/upload/RUN/STOP/physical/TPS\n",positive,refused);
}
