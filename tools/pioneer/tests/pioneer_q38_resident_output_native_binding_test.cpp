/* SPDX-License-Identifier: MIT */
// Actual new native bind/source/lifetime code; OS/byte endpoints are TESTS.
// No initialization or new execution factory, silicon or speed claim.
#define PIONEER_Q38_RMS_FACTORY_FIXTURE_ONLY
#include "pioneer_q38_learned_rms_native_factory_test.cpp"
#include <type_traits>
static_assert(!std::is_default_constructible<PioneerQ38ResidentOutputNativeState>::value,"private genuine context construction only");
static_assert(!std::is_destructible<PioneerQ38ResidentOutputNativeState>::value,"caller cannot release retained native lifetime");
static_assert(!std::is_copy_constructible<PioneerQ38ResidentOutputNativeState>::value,"no native capability copies");
struct NativeBindingFixture:NativeFactoryFixture {
 explicit NativeBindingFixture(const char* zero,const char* embedding):NativeFactoryFixture(zero,embedding){
  auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());
  for(unsigned i=0;i<plan->Count();i++){const auto* p=plan->Part(i);
   if(p->kind!=plan->WeightKind || p->layer || p->role>13)continue;
   unsigned first=p->bank*unsigned(plan->Capacity/4096)+p->offset/4096;
   for(unsigned page=0;page<p->allocated/4096;page++)learned[first+page]=1;
  }
  assert(BindQ38EmbeddingModel(*c,token,0,bank) && io.ResidentOutputCallScopeMatches() && io.ResidentOutputBindSourceScope());
 }
};
#ifndef PIONEER_Q38_OUTPUT_BINDING_FIXTURE_ONLY
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 unsigned refused=0;
 {
  NativeBindingFixture f(argv[1],argv[2]);auto& c=*f.c;auto& m=*c.q38EmbeddingModel;auto& io=f.io;
  unsigned n=io.touches;
  auto refuse=[&](auto& field,auto bad){auto original=field;field=bad;
   assert(!io.ResidentOutputBindSourceScope() && !BindQ38ResidentOutputNativeState(c)
    && !c.q38ResidentOutputNative && io.touches==n);field=original;refused++;
  };
  refuse(io.q38MutableContext,nullptr);refuse(c.q38EmbeddingModel,nullptr);refuse(sRuntimeOwner,nullptr);
  refuse(f.cookie.magic,0U);refuse(f.cookie.context,nullptr);refuse(f.cookie.team,-1);refuse(currentTeam,99);refuse(failThreadInfo,true);
  refuse(c.q38GdnCallThread,-1);refuse(currentThread,99);refuse(sReloadGeneration,sReloadGeneration+1);
  refuse(sColdRecoveryRequired,true);refuse(sReloadQuiesced,true);refuse(sBankStorageOnly,false);refuse(c.bankStorageOnly,false);
  refuse(c.bytes,c.bytes-4096);refuse(c.runtimeCoreMode,-1);refuse(io.runtimeCore,1U);refuse(io.fleetEnabled,true);
  refuse(io.runtimeMapped,false);refuse(io.q38SpanAllowed,true);refuse(c.dispatchOwnerRetained,true);
  refuse(c.initialized,nullptr);refuse(c.hashes,nullptr);refuse(m.ledger,nullptr);
  refuse(m.identity.session,UINT64_C(0));refuse(m.identity.sequence,UINT64_C(0));refuse(m.identity.modelEpoch,UINT64_C(0));
  refuse(m.position,1U);refuse(m.token,248320U);refuse(m.bank,7U);refuse(m.phase,PioneerQ38EmbeddingNativeState::Zeroing);
  refuse(m.zero.saved,true);refuse(m.zero.dirty,true);refuse(m.zero.ready,true);refuse(m.zero.running,true);refuse(m.zero.closed,true);
  refuse(io.q38EmbeddingMemoryMapAttempted,true);refuse(io.q38ProjectionMemoryMapAttempted,true);
  refuse(c.q38FfnModelPlan,static_cast<void*>(&f));refuse(c.q38GraphMutable,static_cast<void*>(&f));
  refuse(c.q38FfnZero,static_cast<void*>(&f));refuse(c.q38FfnGraph,static_cast<void*>(&f));refuse(c.q38FfnCode,static_cast<void*>(&f));
  for(auto* id:{&c.q38FfnModelIdentity,&c.q38GdnIdentity}){
   refuse(id->session,UINT64_C(1));refuse(id->sequence,UINT64_C(1));refuse(id->modelEpoch,UINT64_C(1));refuse(id->driverGeneration,UINT64_C(1));
  }
  refuse(c.q38Cohort,static_cast<void*>(&f));refuse(c.fleet,static_cast<void*>(&f));refuse(sQ38LocalDeltaOwner,static_cast<const void*>(&f));
  refuse(c.q38DispatchRoutes,true);
  for(auto* other:{&c.q38Delta,&c.panelProbe,&c.streamProbe}){
   refuse(other->saved,true);refuse(other->dirty,true);refuse(other->ready,true);
  }
  refuse(c.q38Delta.running,true);refuse(c.q38Delta.awaiting,true);refuse(c.q38Span,nullptr);
  refuse(c.span.closed,false);refuse(c.span.dirty,true);refuse(c.span.ready,true);refuse(c.span.running,true);refuse(c.span.unsafe,true);
  refuse(c.lease.saved,nullptr);refuse(c.lease.ready,false);refuse(c.lease.pending,true);refuse(c.lease.recovery_required,true);
  refuse(c.bulk.saved,false);refuse(c.bulk.ready,false);refuse(c.bulk.dirty,true);refuse(c.bulk.unsafe,true);
  refuse(c.host.state,0U);refuse(c.host.released,1U);refuse(io.PioneerHostPageIO::quarantined,true);
  refuse(c.bulkHost.allocated,false);refuse(c.bulkHost.ready,false);refuse(c.bulkHost.pending,true);refuse(c.bulkHost.quarantined,true);refuse(c.bulkHost.released,true);
  refuse(c.wideHost.allocated,false);refuse(c.wideHost.ready,false);refuse(c.wideHost.memory_type_set,false);refuse(c.wideHost.pending,true);
  refuse(c.wideHost.quarantined,true);refuse(c.wideHost.released,true);refuse(io.wideQuarantined,true);
  refuse(c.q38Clock.required,false);refuse(c.q38Clock.value.state,0U);refuse(c.q38Clock.value.busy_requested,0U);
  refuse(c.q38Clock.value.busy_acknowledged,0U);refuse(c.q38Clock.value.restored,1U);refuse(c.otherUnsafe,true);
  // Cached page presence must be current at the NEW bind; the old prefix
  // factory accepted this model earlier and cannot substitute for this test.
  for(unsigned role:{7U,8U,9U,12U,13U}){
   bool tested=false;
   for(unsigned i=0;i<m.plan.Count() && !tested;i++){const auto* p=m.plan.Part(i);
    if(p->kind!=m.plan.WeightKind || p->layer || p->role!=role)continue;
    unsigned page=p->bank*unsigned(m.plan.Capacity/4096)+p->offset/4096;
    auto original=f.learned[page];f.learned[page]=0;
    assert(!BindQ38ResidentOutputNativeState(c) && !c.q38ResidentOutputNative && io.touches==n);
    f.learned[page]=original;tested=true;refused++;
   }assert(tested);
  }
  assert(BindQ38ResidentOutputNativeState(c) && io.touches==n);auto* state=c.q38ResidentOutputNative;
  assert(state && !state->Unsafe() && state->MatchesBinding(c,m,&f.cookie,c.hashes,f.cookie.team,c.initialized,sReloadGeneration)
   && !state->InitializationsClosed(c.banks,f.owner.dram) && !state->Closed(c.banks,f.owner.dram)
   && c.Unsafe() && !c.Unsafe(true,false,true));
  assert(!BindQ38ResidentOutputNativeState(c) && !io.ResidentOutputBindSourceScope() && io.touches==n);
  assert(!f.Initialize() && !io.EmbeddingSourceScope(false) && !io.LearnedRmsSourceScope(false) && io.touches==n);
  assert(!state->MatchesBinding(c,m,&f.cookie,c.hashes,f.cookie.team,c.initialized,sReloadGeneration+1)
   && !state->MatchesBinding(c,m,&f.cookie,c.hashes,f.cookie.team+1,c.initialized,sReloadGeneration)
   && !state->MatchesBinding(c,m,nullptr,c.hashes,f.cookie.team,c.initialized,sReloadGeneration)
   && !state->MatchesBinding(c,m,&f.cookie,nullptr,f.cookie.team,c.initialized,sReloadGeneration));
  assert(!EndFactory(c) && c.q38ResidentOutputNative==state && state->Unsafe() && c.dispatchOwnerRetained
   && c.banks.Quarantined() && c.q38Clock.quarantined && f.transfer.quarantined && f.wide.quarantined
   && sColdRecoveryRequired && io.touches==n);
  assert(!EndFactory(c) && !BindQ38ResidentOutputNativeState(c) && !f.Initialize() && io.touches==n);
 }
 printf("PASS genuine new native bind/FD+team+thread/source/host+clock+model perimeter:refusals%u,private construction/destruction,actual retained lifetime/initialization and END interlocks;ZERO MMIO/map/reset/STOP/free. OS/learned endpoints are TEST,NOT initialization/execution/native-driver/silicon or rate qualification\n",refused);
}
#endif
