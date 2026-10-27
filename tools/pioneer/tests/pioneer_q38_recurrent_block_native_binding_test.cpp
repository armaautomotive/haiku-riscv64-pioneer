/* SPDX-License-Identifier: MIT */
// Verbatim production binding/base/Unsafe/retention/incomplete-END bodies.
// Zero and embedding run their actual RV32 instructions. OS/MMIO are TESTS;
// this does NOT execute QRB1, initialize its state, or qualify hardware.
#include <cstdint>
#include "../PioneerQ38MixedRecurrentBlockRootLayout.h"
#define PIONEER_Q38_EMBEDDING_FACTORY_FIXTURE_ONLY
#include "pioneer_q38_embedding_native_factory_test.cpp"
#include <type_traits>
static_assert(!std::is_default_constructible<PioneerQ38RecurrentBlockNativeState>::value,"only genuine retained context may construct");
static_assert(!std::is_destructible<PioneerQ38RecurrentBlockNativeState>::value,"no caller release of native ownership");
static_assert(!std::is_copy_constructible<PioneerQ38RecurrentBlockNativeState>::value,"no native capability copies");
struct RecurrentBindingFixture:NativeFactoryFixture {
 RecurrentBindingFixture(const char* zero,const char* embedding):NativeFactoryFixture(zero,embedding){
#ifndef PIONEER_Q38_RECURRENT_BLOCK_END_FIXTURE
  c->q38Clock.value.busy_mhz=1400; // simulated clock endpoint metadata, NOT a producer completion seal
#endif
  auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());
  for(unsigned i=0;i<plan->Count();i++){
   const auto* p=plan->Part(i);if(p->kind!=plan->WeightKind || p->layer)continue;
   unsigned page=p->bank*unsigned(plan->Capacity/4096)+p->offset/4096;
   for(unsigned j=0;j<p->allocated/4096;j++)learned[page+j]=1;
  }
 }
 void Prefix(){
  assert(BindQ38EmbeddingModel(*c,token,0,bank));unsigned n=io.touches;
  assert(!BindQ38RecurrentBlockNative(*c) && io.touches==n && !c->q38RecurrentBlockNative);
  assert(Initialize());n=io.touches;
  assert(!BindQ38RecurrentBlockNative(*c) && io.touches==n && !c->q38RecurrentBlockNative);
  assert(Run() && c->q38EmbeddingModel->Closed() && io.RecurrentBlockCallScopeMatches() && io.RecurrentBlockBaseScope());
 }
};
#ifndef PIONEER_Q38_RECURRENT_BINDING_FIXTURE_ONLY
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 RecurrentBindingFixture f(argv[1],argv[2]);auto& c=*f.c;auto& io=f.io;unsigned refused=0,tensors=0,parts=0;
 auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());
 // All layer0 learned extents must be uploaded BEFORE the producer may
 // claim an epoch/reservation or run. The simulated ledger is NOT a
 // numerical/original-weight proof; it exercises the actual source gate.
 unsigned touches=io.touches;uint64 epoch=sQ38EmbeddingModelEpoch;
 for(unsigned i=0;i<plan->Count();i++){
  const auto* p=plan->Part(i);if(p->kind!=plan->WeightKind || p->layer)continue;
  if(!p->firstRow)tensors++;parts++;
  unsigned page=p->bank*unsigned(plan->Capacity/4096)+p->offset/4096;
  for(unsigned bad:{0U,2U,255U}){auto saved=f.learned[page];f.learned[page]=uint8(bad);
   assert(!BindQ38EmbeddingModel(c,f.token,0,f.bank) && !c.q38EmbeddingModel && !c.q38RecurrentBlockNative
    && io.touches==touches && sQ38EmbeddingModelEpoch==epoch && !io.embeddingStarts && !io.rootStarts);
   f.learned[page]=saved;refused++;
  }
 }
 assert(tensors==14 && parts==56);f.Prefix();auto& m=*c.q38EmbeddingModel;touches=io.touches;
 auto refuse=[&](auto& field,auto bad){auto saved=field;field=bad;
  assert(!io.RecurrentBlockBaseScope() && !BindQ38RecurrentBlockNative(c) && !c.q38RecurrentBlockNative && io.touches==touches);
  field=saved;refused++;
 };
 refuse(m.token,1U);refuse(m.bank,1U);refuse(m.position,1U);
 refuse(io.q38MutableContext,nullptr);refuse(c.q38EmbeddingModel,nullptr);refuse(sRuntimeOwner,nullptr);
 refuse(f.cookie.magic,0U);refuse(f.cookie.context,nullptr);refuse(f.cookie.team,-1);refuse(currentTeam,99);refuse(failThreadInfo,true);
 refuse(c.q38GdnCallThread,-1);refuse(currentThread,99);refuse(sReloadGeneration,sReloadGeneration+1);
 refuse(sColdRecoveryRequired,true);refuse(sReloadQuiesced,true);refuse(sBankStorageOnly,false);refuse(c.bankStorageOnly,false);
 refuse(c.bytes,c.bytes-4096);refuse(c.runtimeCoreMode,-1);refuse(io.runtimeCore,1U);refuse(io.runtimeCore,120U);refuse(io.fleetEnabled,true);
 refuse(io.runtimeMapped,false);refuse(io.q38SpanAllowed,true);refuse(c.dispatchOwnerRetained,true);
 refuse(c.initialized,nullptr);refuse(c.hashes,nullptr);refuse(m.ledger,nullptr);
 refuse(m.identity.session,UINT64_C(0));refuse(m.identity.sequence,UINT64_C(0));refuse(m.identity.modelEpoch,UINT64_C(0));
 refuse(m.token,248320U);refuse(m.bank,7U);refuse(m.phase,PioneerQ38EmbeddingNativeState::Embedding);
 refuse(m.zero.saved,false);refuse(m.zero.dirty,true);refuse(m.zero.ready,true);refuse(m.zero.running,true);
 refuse(m.zero.closed,false);refuse(m.zero.unsafe,true);refuse(m.zero.nonce,1U);refuse(m.zero.owner,nullptr);refuse(m.zero.proof,nullptr);
 refuse(io.q38RecurrentBlockMemoryMapAttempted,true);
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
 refuse(c.span.closed,false);refuse(c.span.dirty,true);refuse(c.span.ready,true);
 refuse(c.span.running,true);refuse(c.span.unsafe,true);
 refuse(c.lease.saved,nullptr);refuse(c.lease.ready,false);refuse(c.lease.pending,true);refuse(c.lease.recovery_required,true);
 refuse(c.bulk.saved,false);refuse(c.bulk.ready,false);refuse(c.bulk.dirty,true);refuse(c.bulk.unsafe,true);
 refuse(c.host.state,0U);refuse(c.host.released,1U);refuse(io.PioneerHostPageIO::quarantined,true);
 refuse(c.bulkHost.allocated,false);refuse(c.bulkHost.ready,false);refuse(c.bulkHost.pending,true);refuse(c.bulkHost.quarantined,true);refuse(c.bulkHost.released,true);
 refuse(c.wideHost.allocated,false);refuse(c.wideHost.ready,false);refuse(c.wideHost.memory_type_set,false);refuse(c.wideHost.pending,true);
 refuse(c.wideHost.quarantined,true);refuse(c.wideHost.released,true);refuse(io.wideQuarantined,true);
 refuse(c.q38Clock.required,false);refuse(c.q38Clock.value.state,0U);refuse(c.q38Clock.value.busy_requested,0U);
 refuse(c.q38Clock.value.busy_acknowledged,0U);refuse(c.q38Clock.value.restored,1U);refuse(c.otherUnsafe,true);
 refuse(c.q38Clock.value.busy_mhz,0U);refuse(c.q38Clock.value.busy_mhz,1401U);
 for(unsigned i=0;i<plan->Count();i++){
  const auto* p=plan->Part(i);if(p->kind!=plan->WeightKind || p->layer)continue;
  unsigned page=p->bank*unsigned(plan->Capacity/4096)+p->offset/4096;
  for(unsigned bad:{0U,2U,255U}){auto saved=f.learned[page];f.learned[page]=uint8(bad);
   assert(!BindQ38RecurrentBlockNative(c) && !c.q38RecurrentBlockNative && io.touches==touches);f.learned[page]=saved;refused++;}
 }
 assert(BindQ38RecurrentBlockNative(c) && io.touches==touches);auto* state=c.q38RecurrentBlockNative;
 assert(state && !state->Unsafe() && !state->Closed() && c.Unsafe() && !c.Unsafe(true,false,true)
  && io.RecurrentBlockBaseScope() && state->MatchesBinding(c,m,&f.cookie,c.hashes,f.cookie.team,c.initialized,sReloadGeneration));
 assert(!BindQ38RecurrentBlockNative(c) && io.touches==touches);
 assert(!state->MatchesBinding(c,m,nullptr,c.hashes,f.cookie.team,c.initialized,sReloadGeneration)
  && !state->MatchesBinding(c,m,&f.cookie,nullptr,f.cookie.team,c.initialized,sReloadGeneration)
  && !state->MatchesBinding(c,m,&f.cookie,c.hashes,f.cookie.team+1,c.initialized,sReloadGeneration)
  && !state->MatchesBinding(c,m,&f.cookie,c.hashes,f.cookie.team,c.initialized,sReloadGeneration+1));
 auto boundRefuse=[&](auto& field,auto bad){auto saved=field;field=bad;
  assert(!io.RecurrentBlockBaseScope() && io.touches==touches);field=saved;refused++;
 };
 PioneerProbeCookie otherCookie=f.cookie;uint32 otherHash=c.hash;
 boundRefuse(sRuntimeOwner,&otherCookie);boundRefuse(c.hashes,&otherHash);
 boundRefuse(m.token,1U);boundRefuse(m.bank,1U);boundRefuse(m.position,1U);
 boundRefuse(io.runtimeCore,1U);boundRefuse(currentThread,99);boundRefuse(currentTeam,99);
 boundRefuse(c.q38DispatchRoutes,true);boundRefuse(io.q38RecurrentBlockMemoryMapAttempted,true);
 boundRefuse(m.identity.session,m.identity.session+1);boundRefuse(m.identity.sequence,m.identity.sequence+1);
 boundRefuse(m.identity.modelEpoch,m.identity.modelEpoch+1);boundRefuse(m.identity.driverGeneration,m.identity.driverGeneration+1);
 assert(io.RecurrentBlockBaseScope() && io.touches==touches);
 // Incomplete native ownership cannot become an END/cleanup grant. The
 // real retention path faults prefix+banks+clock+ALL hosts with no IO.
 assert(!EndFactory(c) && state==c.q38RecurrentBlockNative && state->Unsafe() && c.dispatchOwnerRetained
  && c.banks.Quarantined() && c.q38Clock.quarantined && f.transfer.quarantined && f.wide.quarantined
  && sColdRecoveryRequired && io.touches==touches);
 assert(!EndFactory(c) && !BindQ38RecurrentBlockNative(c) && io.touches==touches);
 printf("PASS actual DISTINCT QRBNODRV base/bind/Unsafe/incomplete-END refusals%u/14learnedtensors/56extents/pre-prefix gate/real zero+QPE1 producer/private construction/ALLhosts retained ZERO post-failure IO;OS+physical endpoints SIMULATED,NOT recurrent init/launch/card or TPS\n",refused);
}
#endif
