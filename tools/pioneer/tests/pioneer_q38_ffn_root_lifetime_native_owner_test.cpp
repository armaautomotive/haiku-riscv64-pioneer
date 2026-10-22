/* SPDX-License-Identifier: MIT */
// ACTUAL native cookie/clock/host gate and Retain function. Native MMIO,
// allocations and clock snapshot are SIMULATED,NOT launch/scope authority.
#define PIONEER_Q38_TEST_QUALIFIED_LEGACY
#define PIONEER_Q38_NORMAL_CLOCK
#define PIONEER_Q38_FFN_ROOT_LIFETIME_FIXTURE_ONLY
#include "pioneer_q38_ffn_root_lifetime_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeArcClockGuard.h"
#include <cstdarg>
#include <new>
static unsigned failAllocation=0;
struct NativeCode:PioneerQ38FfnGraphCodeOwner {
 bool forceOpen=false,forceUnsafe=false;
 bool Closed()const{return !forceOpen && PioneerQ38FfnGraphCodeOwner::Closed();}
 bool Unsafe()const{return forceUnsafe || PioneerQ38FfnGraphCodeOwner::Unsafe();}
 static void* operator new(size_t n,const std::nothrow_t& t)noexcept{return failAllocation==1?nullptr: ::operator new(n,t);}
 static void operator delete(void* p)noexcept{::operator delete(p);}
 static void operator delete(void* p,const std::nothrow_t&)noexcept{::operator delete(p);}
};
struct NativeLife:PioneerQ38FfnRootLifetimeOwner {
 bool forceOpen=false,forceUnsafe=false;
 bool Closed()const{return !forceOpen && PioneerQ38FfnRootLifetimeOwner::Closed();}
 bool Unsafe()const{return forceUnsafe || PioneerQ38FfnRootLifetimeOwner::Unsafe();}
 PioneerQ38FfnRootLifetimeResult result;
 static void* operator new(size_t n,const std::nothrow_t& t)noexcept{return failAllocation==2?nullptr: ::operator new(n,t);}
 static void operator delete(void* p)noexcept{::operator delete(p);}
 static void operator delete(void* p,const std::nothrow_t&)noexcept{::operator delete(p);}
 template<class IO>bool Qualify(IO& io,const tt_dram_samples_result& proof,PioneerResidentBanks& banks,
  PioneerQ38DispatchLease& cohort,PioneerQ38FfnGraphCodeOwner& code,PioneerQ38FfnRootLifetimeResult& r) {
  io.life=&r;bool okay=PioneerQ38FfnRootLifetimeOwner::Qualify(io,proof,banks,cohort,code,r);result=r;return okay;
 }
};
struct NativeIO:RootLifetimeIO {
 bool mapFail=false;unsigned mapCalls=0;
 bool MapFfnGraphCodePrefix(){mapCalls++;if(mapFail){observed=touches;return false;}return true;}
};
struct NativeClock {
 tt_arc_clock_guard_result value={};bool requires=true,quarantined=false;
 NativeClock(){value.state=TT_ARC_CLOCK_BUSY_VERIFIED;value.busy_requested=value.busy_acknowledged=1;}
 const auto& Snapshot()const{return value;}
 bool RequiresExplicitEnd()const{return requires;}
 void Quarantine(){quarantined=true;value.recovery_required=1;} // no ARC API exists here
};
struct NativeContext {
 NativeIO& io;PioneerResidentBanks& banks;PioneerResidentLease& lease;
 PioneerQ38DispatchLease* q38Dispatch;NativeCode* q38FfnCode=nullptr;NativeLife* q38FfnRootLifetime=nullptr;
 uint64_t bytes=PioneerResidentBanks::Total;uint8_t bitmap=0;uint8_t* initialized=&bitmap;
 bool bankStorageOnly=true,forcedUnsafe=false,dispatchOwnerRetained=false;
 struct {unsigned state=TT_HOST_PAGE_READY;bool released=false;} host;
 struct {bool allocated=true,ready=true,pending=false,quarantined=false,released=false;} bulkHost,wideHost;
 NativeClock q38Clock;
 NativeContext(NativeIO& i,PioneerResidentBanks& b,PioneerResidentLease& s,PioneerQ38DispatchLease* d):io(i),banks(b),lease(s),q38Dispatch(d){}
 bool Unsafe(){return forcedUnsafe || banks.Quarantined() || q38Clock.quarantined;}
};
static bool sColdRecoveryRequired=false,sReloadQuiesced=false,sBankStorageOnly=true;
static uint64_t sReloadGeneration=32;
struct NativeCookie {uint32_t magic=0x50313030;NativeContext* context=nullptr;};
static NativeCookie* sRuntimeOwner=nullptr;
static unsigned logCount=0;static char lastLog[768];
static void KernelLog(const char* format,...){logCount++;va_list a;va_start(a,format);vsnprintf(lastLog,sizeof(lastLog),format,a);va_end(a);}
#define PioneerStoreContext NativeContext
#include PIONEER_ROOT_RETAIN_INCLUDE
#undef PioneerStoreContext
static bool ActualEndGate(NativeContext& c,tt_resident_session_result& result,bool& clean) {
#include PIONEER_ROOT_END_GATE_INCLUDE
 return true;
}
static bool ActualHook(NativeContext& c,const tt_dram_samples_result& dram,PioneerQ38DispatchLease& lease) {
#define PioneerQ38FfnGraphCodeOwner NativeCode
#define PioneerQ38FfnRootLifetimeOwner NativeLife
#define dprintf KernelLog
#include PIONEER_ROOT_NATIVE_OWNER_INCLUDE
#undef dprintf
#undef PioneerQ38FfnRootLifetimeOwner
#undef PioneerQ38FfnGraphCodeOwner
 return true;
}
struct NativeFixture:RootLifetimeFixture<NativeIO> {
 NativeContext context{io,banks,*cohort->worker[0].master,cohort.get()};NativeCookie cookie;
 NativeFixture(){cookie.context=&context;sRuntimeOwner=&cookie;sColdRecoveryRequired=sReloadQuiesced=false;
  sBankStorageOnly=true;sReloadGeneration=32;logCount=0;lastLog[0]=0;}
 ~NativeFixture(){delete context.q38FfnRootLifetime;delete context.q38FfnCode;} // TEST-only reclamation AFTER retention assertions
 bool Hook(){return ActualHook(context,dram,*cohort);}
 void Retained(){assert(context.dispatchOwnerRetained && sColdRecoveryRequired && context.q38Clock.quarantined
  && context.bulkHost.quarantined && !context.bulkHost.ready && context.wideHost.quarantined && !context.wideHost.ready
  && banks.Quarantined() && io.quarantined && context.lease.recovery_required && cohort->unsafe);
  for(auto& w:cohort->worker)assert(w.image.unsafe && w.Store().recovery_required);
  auto* code=context.q38FfnCode;auto* life=context.q38FfnRootLifetime;unsigned touches=io.touches,maps=io.mapCalls;
  io.life=nullptr;assert(!Hook() && io.touches==touches && io.mapCalls==maps && context.q38FfnCode==code && context.q38FfnRootLifetime==life);
 }
};
int main(){
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 unsigned cases=0;
 {NativeFixture f;tt_resident_session_result r={};bool clean=true;unsigned before=f.io.touches;
  assert(ActualEndGate(f.context,r,clean) && clean && !sColdRecoveryRequired && f.io.touches==before);cases++;}
 {NativeFixture f;assert(f.Hook() && logCount==1 && strstr(lastLog,"P100A_FFN_ROOT_LIFETIME_PASS root1 workers119 saved867448")
   && strstr(lastLog,"frames11424/11424") && strstr(lastLog,"NEW_ROOT_AND_WORKERS_EXECUTED_STOP_ONLY"));
  assert(f.context.q38FfnCode && f.context.q38FfnRootLifetime && f.context.q38FfnCode->Closed()
   && f.context.q38FfnRootLifetime->Closed() && !f.context.dispatchOwnerRetained && !f.context.q38Clock.quarantined
   && !sColdRecoveryRequired && f.io.mapCalls==1 && f.io.resetWrites==480);
  tt_resident_session_result r={};bool clean=true;unsigned before=f.io.touches;
  assert(ActualEndGate(f.context,r,clean) && clean && f.io.touches==before && !sColdRecoveryRequired);
  f.io.life=nullptr;f.io.track=false;f.io.rootDone=false;f.io.armed=f.io.released=0;
  assert(PioneerQ38DispatchBootstrap(f.io,f.dram,f.banks,*f.cohort));cases++;
 }
 for(unsigned fault=0;fault<6;fault++){NativeFixture f;assert(f.Hook());unsigned before=f.io.touches;
  auto* code=f.context.q38FfnCode;auto* life=f.context.q38FfnRootLifetime;
  if(fault==0)f.context.q38FfnCode=nullptr;if(fault==1)f.context.q38FfnRootLifetime=nullptr;
  if(fault==2)code->forceOpen=true;if(fault==3)code->forceUnsafe=true;
  if(fault==4)life->forceOpen=true;if(fault==5)life->forceUnsafe=true;
  tt_resident_session_result r={};bool clean=true;
  assert(!ActualEndGate(f.context,r,clean) && !clean && f.io.touches==before && r.cold && r.active && !r.host_released);
  f.context.q38FfnCode=code;f.context.q38FfnRootLifetime=life;f.Retained();cases++;
 }
 for(unsigned allocation:{1U,2U}){NativeFixture f;failAllocation=allocation;assert(!f.Hook());failAllocation=0;
  assert(!f.io.touches && !f.io.mapCalls && (!f.context.q38FfnCode)==(allocation==1)
   && (!f.context.q38FfnRootLifetime)==(allocation==2));f.Retained();cases++;}
 {NativeFixture f;f.io.mapFail=true;assert(!f.Hook() && f.context.q38FfnCode && f.context.q38FfnRootLifetime
   && !f.io.writes && f.io.mapCalls==1 && strstr(lastLog,"CLOSE_ONLY_NO_RETRY_RESET_RESTORE"));f.Retained();cases++;}
 for(unsigned phase:{1U,2U,5U,6U,7U,8U,9U,10U}){NativeFixture f;f.io.failure=1;f.io.point=1;
  f.io.wantedPhase=phase;f.io.wantedWorker=60;assert(!f.Hook() && f.io.observed==f.io.touches
   && f.context.q38FfnCode && f.context.q38FfnRootLifetime && f.context.q38FfnCode->Unsafe());f.Retained();cases++;}
 for(unsigned fault=0;fault<22;fault++){NativeFixture f;
  if(!fault)sColdRecoveryRequired=true;if(fault==1)sReloadQuiesced=true;if(fault==2)sReloadGeneration=0;
  if(fault==3)sRuntimeOwner=nullptr;if(fault==4)f.cookie.magic=0;if(fault==5)f.cookie.context=nullptr;
  if(fault==6)sBankStorageOnly=false;if(fault==7)f.context.bankStorageOnly=false;if(fault==8)f.context.bytes=4096;
  if(fault==9)f.context.q38Dispatch=nullptr;if(fault==10)f.context.initialized=nullptr;if(fault==11)f.context.forcedUnsafe=true;
  if(fault==12)f.context.host.state=0;if(fault==13)f.context.host.released=true;
  if(fault==14)f.context.bulkHost.ready=false;if(fault==15)f.context.wideHost.ready=false;
  if(fault==16)f.context.q38Clock.requires=false;if(fault==17)f.context.q38Clock.value.state=0;
  if(fault==18)f.context.q38Clock.value.busy_requested=0;if(fault==19)f.context.q38Clock.value.busy_acknowledged=0;
  if(fault==20)f.context.q38Clock.value.restored=1;if(fault==21)f.context.wideHost.pending=true;
  assert(!f.Hook() && !f.io.touches && !f.io.mapCalls && !f.context.q38FfnCode && !f.context.q38FfnRootLifetime);
  if(fault==9)f.context.q38Dispatch=f.cohort.get();
  f.Retained();cases++;
 }
 printf("PASS ACTUAL native ROOT+ALL119 lifetime hook+retain:cases%u/real cookie+clock+host CPU gates/BOTH heap owners retained on allocation-map-install-boot-ROOT-STOP-hold-restore failures/cold+clock+ALL hosts quarantined immediately/no later IO or fallback/legacy bootstrap ONLY after exact restore;SIMULATED_ENDPOINTS_NOT_NATIVE_QUALIFICATION\n",cases);
}
