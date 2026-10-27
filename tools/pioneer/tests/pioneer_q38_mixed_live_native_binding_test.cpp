/* SPDX-License-Identifier: MIT */
// Actual native bind/base/Unsafe/partial-END bodies and ACTUAL zero/QPE1.
// OS/PCI/GDDR endpoints are TESTS; no mixed mapping/launch or silicon grant.
#include <cstdint>
#include "../PioneerQ38MixedLiveBlockRootLayout.h" // before fixture's unrelated global Input()
#define PIONEER_Q38_EMBEDDING_FACTORY_FIXTURE_ONLY
#include "pioneer_q38_embedding_native_factory_test.cpp"
#include <type_traits>
static_assert(!std::is_default_constructible<PioneerQ38MixedLiveNativeState>::value,"genuine context construction only");
static_assert(!std::is_destructible<PioneerQ38MixedLiveNativeState>::value,"no caller release of retained native state");
static_assert(!std::is_copy_constructible<PioneerQ38MixedLiveNativeState>::value,"no native capability copies");
struct MixedNativeBindingFixture:NativeFactoryFixture {
 MixedNativeBindingFixture(const char* zero,const char* embedding):NativeFactoryFixture(zero,embedding){
  c->q38Clock.value.busy_mhz=1400; // simulated normal-clock metadata, NOT a producer seal
  auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());
  for(unsigned i=0;i<plan->Count();i++){
   const auto* p=plan->Part(i);if(p->kind!=plan->WeightKind || p->layer!=3)continue;
   unsigned first=p->bank*unsigned(plan->Capacity/4096)+p->offset/4096;
   for(unsigned j=0;j<p->allocated/4096;j++)learned[first+j]=1;
  }
  assert(BindQ38EmbeddingModel(*c,token,0,bank));
  unsigned touches=io.touches;assert(!BindQ38MixedLiveNative(*c) && io.touches==touches && !c->q38MixedLiveNative);
  assert(Initialize());touches=io.touches;
  assert(!BindQ38MixedLiveNative(*c) && io.touches==touches && !c->q38MixedLiveNative);
  assert(Run() && c->q38EmbeddingModel->Closed() && io.MixedLiveCallScopeMatches() && io.MixedLiveBaseScope());
 }
};
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 unsigned refused=0;MixedNativeBindingFixture f(argv[1],argv[2]);
 auto& c=*f.c;auto& io=f.io;auto& m=*c.q38EmbeddingModel;unsigned touches=io.touches;
 // A valid-but-different coordinate must not retarget the already completed
 // producer before the new state captures its own immutable binding.
 auto refuse=[&](auto& field,auto bad){auto saved=field;field=bad;
  if(io.MixedLiveBaseScope())fprintf(stderr,"UNEXPECTED_NATIVE_BASE_SCOPE refusal%u\n",refused);
  assert(!io.MixedLiveBaseScope() && !BindQ38MixedLiveNative(c) && !c.q38MixedLiveNative && io.touches==touches);
  field=saved;refused++;
 };
 refuse(m.token,1U);refuse(m.bank,1U);
 refuse(io.q38MutableContext,nullptr);refuse(c.q38EmbeddingModel,nullptr);refuse(sRuntimeOwner,nullptr);
 refuse(f.cookie.magic,0U);refuse(f.cookie.context,nullptr);refuse(f.cookie.team,-1);refuse(currentTeam,99);refuse(failThreadInfo,true);
 refuse(c.q38GdnCallThread,-1);refuse(currentThread,99);refuse(sReloadGeneration,sReloadGeneration+1);
 refuse(sColdRecoveryRequired,true);refuse(sReloadQuiesced,true);refuse(sBankStorageOnly,false);refuse(c.bankStorageOnly,false);
 refuse(c.bytes,c.bytes-4096);refuse(c.runtimeCoreMode,-1);refuse(io.runtimeCore,1U);refuse(io.runtimeCore,120U);refuse(io.fleetEnabled,true);
 refuse(io.runtimeMapped,false);refuse(io.q38SpanAllowed,true);refuse(c.dispatchOwnerRetained,true);
 refuse(c.initialized,nullptr);refuse(c.hashes,nullptr);refuse(m.ledger,nullptr);
 refuse(m.identity.session,UINT64_C(0));refuse(m.identity.sequence,UINT64_C(0));refuse(m.identity.modelEpoch,UINT64_C(0));
 refuse(m.position,1U);refuse(m.token,248320U);refuse(m.bank,7U);refuse(m.phase,PioneerQ38EmbeddingNativeState::Embedding);
 refuse(m.zero.saved,false);refuse(m.zero.dirty,true);refuse(m.zero.ready,true);refuse(m.zero.running,true);
 refuse(m.zero.closed,false);refuse(m.zero.unsafe,true);refuse(m.zero.nonce,1U);refuse(m.zero.owner,nullptr);refuse(m.zero.proof,nullptr);
 refuse(io.q38MixedLiveMemoryMapAttempted,true);
 refuse(c.q38FfnModelPlan,reinterpret_cast<PioneerQ38CanonicalModelPlan*>(uintptr_t(1)));
 refuse(c.q38GraphMutable,reinterpret_cast<PioneerQ38GraphMutableBanks*>(uintptr_t(1)));
 refuse(c.q38FfnZero,reinterpret_cast<PioneerQ38FfnZeroLease*>(uintptr_t(1)));
 refuse(c.q38FfnGraph,reinterpret_cast<PioneerQ38FfnPersistentGraphOwner*>(uintptr_t(1)));
 refuse(c.q38FfnCode,reinterpret_cast<PioneerQ38FfnGraphCodeOwner*>(uintptr_t(1)));
 for(auto* id:{&c.q38FfnModelIdentity,&c.q38GdnIdentity}){
  refuse(id->session,UINT64_C(1));refuse(id->sequence,UINT64_C(1));refuse(id->modelEpoch,UINT64_C(1));refuse(id->driverGeneration,UINT64_C(1));
 }
 refuse(c.q38Cohort,static_cast<void*>(&f));refuse(c.fleet,static_cast<void*>(&f));refuse(sQ38LocalDeltaOwner,static_cast<const void*>(&f));
 refuse(c.q38DispatchRoutes,true);
 for(auto* other:{&c.q38Delta,&c.panelProbe,&c.streamProbe}){refuse(other->saved,true);refuse(other->dirty,true);refuse(other->ready,true);}
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
 refuse(c.q38Clock.value.busy_mhz,0U);refuse(c.q38Clock.value.busy_mhz,1401U);
 // Iterate actual extents; public role numbers need not be contiguous.
 unsigned learnedTensors=0;
 for(unsigned i=0;i<m.plan.Count();i++){
  const auto* p=m.plan.Part(i);if(p->kind!=m.plan.WeightKind || p->layer!=3)continue;
  unsigned page=p->bank*unsigned(m.plan.Capacity/4096)+p->offset/4096;
  for(unsigned bad:{0U,2U,255U}){auto saved=f.learned[page];f.learned[page]=uint8(bad);
   assert(!BindQ38MixedLiveNative(c) && !c.q38MixedLiveNative && io.touches==touches);f.learned[page]=saved;refused++;}
  if(!p->firstRow)learnedTensors++;
 }
 assert(learnedTensors==11 && BindQ38MixedLiveNative(c) && io.touches==touches);
 auto* state=c.q38MixedLiveNative;
 assert(state && !state->Unsafe() && !state->Closed() && c.Unsafe() && !c.Unsafe(true,false,true)
  && io.MixedLiveBaseScope() && state->MatchesBinding(c,m,&f.cookie,c.hashes,f.cookie.team,c.initialized,sReloadGeneration));
 assert(!BindQ38MixedLiveNative(c) && io.touches==touches);
 assert(!state->MatchesBinding(c,m,nullptr,c.hashes,f.cookie.team,c.initialized,sReloadGeneration)
  && !state->MatchesBinding(c,m,&f.cookie,nullptr,f.cookie.team,c.initialized,sReloadGeneration)
  && !state->MatchesBinding(c,m,&f.cookie,c.hashes,f.cookie.team+1,c.initialized,sReloadGeneration)
  && !state->MatchesBinding(c,m,&f.cookie,c.hashes,f.cookie.team,c.initialized,sReloadGeneration+1));
 // After capture, even an otherwise valid replacement cookie/hash address
 // or model selector must not borrow the original retained binding.
 auto boundRefuse=[&](auto& field,auto bad){auto saved=field;field=bad;
  assert(!io.MixedLiveBaseScope() && io.touches==touches);field=saved;refused++;
 };
 PioneerProbeCookie otherCookie=f.cookie;uint32 otherHash=c.hash;
 boundRefuse(sRuntimeOwner,&otherCookie);boundRefuse(c.hashes,&otherHash);
 boundRefuse(m.token,1U);boundRefuse(m.bank,1U);boundRefuse(m.position,1U);
 boundRefuse(io.runtimeCore,1U);boundRefuse(currentThread,99);boundRefuse(currentTeam,99);
 boundRefuse(m.identity.session,m.identity.session+1);boundRefuse(m.identity.sequence,m.identity.sequence+1);
 boundRefuse(m.identity.modelEpoch,m.identity.modelEpoch+1);boundRefuse(m.identity.driverGeneration,m.identity.driverGeneration+1);
 assert(io.MixedLiveBaseScope() && io.touches==touches);
 // An incomplete new owner must refuse END before mapping or old cleanup,
 // and retain the real prefix/clock/host leases with ZERO further IO.
 assert(!EndFactory(c) && state==c.q38MixedLiveNative && state->Unsafe() && c.dispatchOwnerRetained
  && c.banks.Quarantined() && c.q38Clock.quarantined && f.transfer.quarantined && f.wide.quarantined
  && sColdRecoveryRequired && io.touches==touches);
 assert(!EndFactory(c) && !BindQ38MixedLiveNative(c) && io.touches==touches);
 printf("PASS actual NEW native base/bind/Unsafe/incomplete-END refusals%u/11learnedtensors/current completed token+bank/real zero+QPE1 producer/private construction/ALLhosts retained ZERO post-failure IO;OS+physical endpoints SIMULATED,NOT mixed launch/native card or TPS\n",refused);
}
