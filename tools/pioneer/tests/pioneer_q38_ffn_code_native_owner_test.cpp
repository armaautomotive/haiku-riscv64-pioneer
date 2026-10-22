/* SPDX-License-Identifier: MIT */
// ACTUAL extracted native owner hook;synthetic allocations/L1/kernel logs.
// No native scope/IO/launch authority comes from these test fixtures.
#define PIONEER_Q38_FFN_CODE_OWNER_MOCK_ONLY
#include "pioneer_q38_ffn_graph_code_owner_test.cpp"
#include <cstdarg>
#include <new>
static bool allocationFailure=false;
struct HookOwner:PioneerQ38FfnGraphCodeOwner {
 static void* operator new(size_t n,const std::nothrow_t& t)noexcept {if(allocationFailure)return nullptr;return ::operator new(n,t);}
 static void operator delete(void* p)noexcept {::operator delete(p);}
 static void operator delete(void* p,const std::nothrow_t&)noexcept {::operator delete(p);}
};
struct HookIO:CodeIO {
 bool mapFailure=false,writeFailure=false;unsigned mapCalls=0;
 bool MapFfnGraphCodePrefix(){mapCalls++;if(mapFailure){observed=touches;return false;}return true;}
 bool FfnGraphProgramWrite(unsigned w,uint32_t a,uint32_t v){Touch();writes++;
  if(writeFailure && !w && a==0x64000){observed=touches;return false;}RawWrite(w,a,v);return true;}
};
static unsigned logCount=0;static char lastLog[512];
static void KernelLog(const char* format,...){logCount++;va_list args;va_start(args,format);vsnprintf(lastLog,sizeof(lastLog),format,args);va_end(args);}
struct HookContext {HookIO& io;PioneerResidentBanks& banks;HookOwner* q38FfnCode=nullptr;};
static bool ActualHook(HookContext& c,const tt_dram_samples_result& dram,PioneerQ38DispatchLease& lease){
#define PioneerQ38FfnGraphCodeOwner HookOwner
#define dprintf KernelLog
#include PIONEER_FFN_NATIVE_OWNER_INCLUDE
#undef dprintf
#undef PioneerQ38FfnGraphCodeOwner
 return true;
}
struct HookFixture {
 HookIO io;PioneerResidentBanks banks;tt_dram_samples_result dram;
 std::unique_ptr<PioneerQ38DispatchLease> cohort{new PioneerQ38DispatchLease};
 std::vector<std::unique_ptr<PanelOwners>> owners;HookContext context{io,banks};
 HookFixture(){dram=BankProof(*io.core[0]);assert(banks.Bind(dram));
  for(unsigned w=0;w<120;w++){owners.emplace_back(new PanelOwners(*io.core[w]));auto& n=cohort->worker[w];n.master=&owners[w]->store;
   n.noc=owners[w]->noc;if(w)n.noc.version=PioneerWorkerVersion(w);assert(PioneerQ38DispatchPrepareRole(io,dram,banks,*cohort,w));}
  io.track=true;logCount=0;lastLog[0]=0;
 }
 // CPU-only fixtures are reclaimed AFTER all retention assertions. This
 // destructor is NOT a native cleanup adapter or permission to free DMA.
 ~HookFixture(){delete context.q38FfnCode;}
 bool Run(){return ActualHook(context,dram,*cohort);}
 void Unknown(){assert(cohort->unsafe && banks.Quarantined() && io.quarantined && !io.armed && !io.released);
  assert(io.observed==io.touches);for(auto& n:cohort->worker)assert(n.image.unsafe && n.Store().recovery_required);}
};
int main(){
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 {HookFixture f;assert(f.Run() && logCount==1 && strstr(lastLog,"P100A_FFN_CODE_PASS workers120 saved867448 installed867448 restored867448"));
  assert(f.context.q38FfnCode && f.context.q38FfnCode->Closed() && !f.context.q38FfnCode->Unsafe() && !f.cohort->unsafe
   && !f.io.armed && !f.io.released && f.io.mapCalls==1 && f.io.writes==2*867448);
  f.io.track=false;assert(PioneerQ38DispatchBootstrap(f.io,f.dram,f.banks,*f.cohort) && f.io.armed==119 && f.io.released==119);
 }
 {HookFixture f;allocationFailure=true;assert(!f.Run());allocationFailure=false;
  assert(!f.context.q38FfnCode && !f.io.touches && !f.io.mapCalls && !f.io.writes && logCount==1 && strstr(lastLog,"CLOSE_ONLY_NO_START_RETRY"));f.Unknown();}
 {HookFixture f;f.io.mapFailure=true;assert(!f.Run() && f.context.q38FfnCode && !f.context.q38FfnCode->Closed()
  && !f.io.writes && f.io.mapCalls==1 && logCount==1 && strstr(lastLog,"CLOSE_ONLY_NO_START_RETRY"));f.Unknown();}
 {HookFixture f;f.io.writeFailure=true;assert(!f.Run() && f.context.q38FfnCode && f.context.q38FfnCode->Unsafe()
  && logCount==1 && strstr(lastLog,"CLOSE_ONLY_NO_START_RETRY"));f.Unknown();}
 {HookFixture f;assert(f.Run());auto* original=f.context.q38FfnCode;unsigned touches=f.io.touches,writes=f.io.writes,mapCalls=f.io.mapCalls,logs=logCount;
  f.io.observed=touches;assert(!f.Run() && f.context.q38FfnCode==original && f.io.touches==touches && f.io.writes==writes
   && f.io.mapCalls==mapCalls && logCount==logs);f.Unknown();}
 puts("PASS ACTUAL native held-only FFN hook:complete owner retained on allocation/mapping/partial-write/reentry faults/no extra IO or launch/clean roundtrip permits original legacy bootstrap;SIMULATED_KERNEL_NOT_NATIVE_QUALIFICATION");
}
