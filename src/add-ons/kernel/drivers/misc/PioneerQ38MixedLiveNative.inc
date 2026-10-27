/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT
#include "PioneerQ38MixedLiveNativeRole.h"
#ifdef PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE
#include "PioneerQ38MixedLiveFinalReleaseScope.inc"
#endif
bool PioneerSessionIO::MixedLiveCallScopeMatches(){
 auto* c=q38MutableContext;auto* fd=sRuntimeOwner;
 if(!c || &c->io!=this || !fd || fd->magic!=0x50313030 || fd->context!=c || fd->team<0
  || c->q38GdnCallThread<0 || c->q38GdnCallThread!=find_thread(NULL))return false;
 thread_info info={};
 return get_thread_info(c->q38GdnCallThread,&info)==B_OK && info.team==fd->team
  && sRuntimeOwner==fd && fd->context==c && c->q38GdnCallThread==find_thread(NULL);
}
bool PioneerSessionIO::MixedLiveBaseScope(){
 auto* c=q38MutableContext;auto* m=c?c->q38EmbeddingModel:NULL;auto* state=c?c->q38MixedLiveNative:NULL;
 if(!c || &c->io!=this || !m || !MixedLiveCallScopeMatches() || sColdRecoveryRequired || sReloadQuiesced || !sReloadGeneration
  || !sBankStorageOnly || !c->bankStorageOnly || c->bytes!=PioneerResidentBanks::Total || c->runtimeCoreMode!=1
  || runtimeCore>=120 || !runtimeMapped || q38SpanAllowed || c->dispatchOwnerRetained || !c->initialized || !c->hashes
  || m->ledger!=c->initialized || !m->identity.Valid() || m->identity.driverGeneration!=sReloadGeneration
  || m->position || m->token>=248320 || m->bank>=7 || m->phase!=PioneerQ38EmbeddingNativeState::Complete || !m->Closed()
  || !PioneerQ38MixedLiveNativeState::CompletedInputMatches(*m)
  || !m->embedding.Completed() || !m->embedding.MatchesNativeOwner(dram,noc,c->banks,m->graph,m->plan,m->identity,c->initialized,PioneerResidentBanks::Pages)
  || !m->zero.saved || !m->zero.closed || m->zero.dirty || m->zero.ready || m->zero.running || m->zero.unsafe || m->zero.nonce!=2
  || m->zero.owner!=m->graph.Projection(m->identity) || m->zero.banks!=&c->banks || m->zero.proof!=&dram || !(m->zero.identity==m->identity)
  || m->graph.CanonicalModel(m->identity)!=&m->plan || !m->graph.MatchesInitializationOwner(c->banks,dram,m->identity)
  || c->q38FfnModelPlan || c->q38GraphMutable || c->q38FfnZero || c->q38FfnGraph || c->q38FfnCode
  || c->q38FfnModelIdentity.session || c->q38FfnModelIdentity.sequence || c->q38FfnModelIdentity.modelEpoch || c->q38FfnModelIdentity.driverGeneration
  || c->q38GdnBanks || c->q38StateZero || c->q38GdnIdentity.session || c->q38GdnIdentity.sequence || c->q38GdnIdentity.modelEpoch
  || c->q38GdnIdentity.driverGeneration || c->q38Cohort || c->fleet || sQ38LocalDeltaOwner
  || c->q38Delta.saved || c->q38Delta.dirty || c->q38Delta.ready || c->q38Delta.running || c->q38Delta.awaiting
  || c->panelProbe.saved || c->panelProbe.dirty || c->panelProbe.ready || c->streamProbe.saved || c->streamProbe.dirty || c->streamProbe.ready
  || !c->q38Span || !c->q38Span->closed || c->q38Span->dirty || c->q38Span->ready || c->q38Span->running || c->q38Span->unsafe
  || !c->lease.saved || !c->lease.ready || c->lease.pending || c->lease.recovery_required
  || !c->bulk.saved || !c->bulk.ready || c->bulk.dirty || c->bulk.unsafe || c->host.state!=TT_HOST_PAGE_READY
  || c->host.released || PioneerHostPageIO::quarantined || !c->bulkHost.allocated || !c->bulkHost.ready || c->bulkHost.pending
  || c->bulkHost.quarantined || c->bulkHost.released || !c->wideHost.allocated || !c->wideHost.ready || !c->wideHost.memory_type_set
  || c->wideHost.pending || c->wideHost.quarantined || c->wideHost.released || wideQuarantined)return false;
 if(state){
  if(!state->MatchesBinding(*c,*m,sRuntimeOwner,c->hashes,sRuntimeOwner->team,c->initialized,sReloadGeneration))return false;
  if(state->phase==PioneerQ38MixedLiveNativeState::Bound){if(runtimeCore || c->q38Dispatch || c->q38DispatchRoutes || fleetEnabled)return false;}
  else if(!c->q38Dispatch || c->q38Dispatch->worker[0].master!=&c->lease || !fleetEnabled || c->q38Dispatch->unsafe
   || c->q38Dispatch->closed || c->q38Dispatch->booted || c->q38Dispatch->bootAttempted)return false;
  if(c->q38Dispatch && c->q38Dispatch->rootRunning && !state->ControlsCohort(c->q38Dispatch))return false;
  if(state->phase>=PioneerQ38MixedLiveNativeState::MemoryPreparing
   && (!state->memory || !state->plan || !c->q38DispatchRoutes || c->q38Dispatch->prepared!=120 || !state->plan->Matches(m->plan)))return false;
 }else if(runtimeCore || c->q38Dispatch || c->q38DispatchRoutes || fleetEnabled || q38MixedLiveMemoryMapAttempted)return false;
 const auto& clock=c->q38Clock.Snapshot();
 return c->q38Clock.RequiresExplicitEnd() && clock.state==TT_ARC_CLOCK_BUSY_VERIFIED && clock.busy_requested && clock.busy_acknowledged
  && !clock.restored && clock.busy_mhz && clock.busy_mhz<=1400 && !c->Unsafe(true,false,true);
} // necessary authentic native binding; per-word current physical guards remain independent
bool PioneerSessionIO::MixedLiveMappingSourceScope(){
 auto* c=q38MutableContext;auto* state=c?c->q38MixedLiveNative:NULL;
 return state && !runtimeCore && state->phase==PioneerQ38MixedLiveNativeState::MemoryPreparing && state->memory
  && !state->memory->Installed() && !state->memory->Closed() && MixedLiveBaseScope();
}
bool PioneerSessionIO::MixedLiveMemoryScope(const PioneerQ38MixedLiveMemory& memory,const PioneerQ38CanonicalModelPlan& canonical,
 const PioneerQ38GdnIdentity& id,const uint8_t* learned,unsigned pages){
 auto* c=q38MutableContext;auto* state=c?c->q38MixedLiveNative:NULL;auto* m=c?c->q38EmbeddingModel:NULL;
 return state && m && state->memory==&memory && &canonical==&m->plan && id==m->identity && learned==c->initialized
  && pages==PioneerResidentBanks::Pages && (state->phase==PioneerQ38MixedLiveNativeState::MemoryPreparing
   || state->phase==PioneerQ38MixedLiveNativeState::MemoryInstalled || state->phase==PioneerQ38MixedLiveNativeState::MemoryClosing)
  && MixedLiveBaseScope();
}
bool PioneerSessionIO::MixedLiveExecutionScope(const PioneerQ38MixedLiveOwner& execution,const PioneerQ38MixedLiveMemory& memory,
 const PioneerQ38CanonicalModelPlan& canonical,const PioneerQ38MixedLiveBlockPlan& plan,const PioneerQ38GdnIdentity& id,const uint8_t* learned,unsigned pages){
 auto* c=q38MutableContext;auto* state=c?c->q38MixedLiveNative:NULL;auto* m=c?c->q38EmbeddingModel:NULL;
 return state && m && state->execution==&execution && state->memory==&memory && state->plan==&plan && &canonical==&m->plan
  && id==m->identity && learned==c->initialized && pages==PioneerResidentBanks::Pages
  && (state->phase==PioneerQ38MixedLiveNativeState::ExecutionPreparing || state->phase==PioneerQ38MixedLiveNativeState::ExecutionReady
   || state->phase==PioneerQ38MixedLiveNativeState::ExecutionComplete || state->phase==PioneerQ38MixedLiveNativeState::ExecutionClosing)
  && execution.MatchesNativeOwner(memory,plan) && MixedLiveBaseScope();
}
void PioneerSessionIO::MixedLiveRetainHostOwners(){
 if(q38MutableContext){tt_resident_session_result retained={};RetainQ38DispatchOwner(*q38MutableContext,retained);}
} // memory only; actual retention quarantines the SAME graph,clock,hosts and L1 maps
[[maybe_unused]] static bool MixedLiveLearnedPages(const PioneerQ38EmbeddingNativeState& m){
 unsigned tensors=0;
 for(unsigned i=0;i<m.plan.Count();i++){
  const auto* p=m.plan.Part(i);if(p->kind!=m.plan.WeightKind || p->layer!=3)continue;
  if(!p->firstRow)tensors++;
  unsigned first=p->bank*unsigned(m.plan.Capacity/4096)+p->offset/4096;
  for(unsigned j=0;j<p->allocated/4096;j++)if(m.ledger[first+j]!=1)return false;
 }
 return tensors==11;
}
[[maybe_unused]] static bool BindQ38MixedLiveNative(PioneerStoreContext& c){
 if(c.q38MixedLiveNative || c.io.runtimeCore || !c.io.MixedLiveBaseScope() || !MixedLiveLearnedPages(*c.q38EmbeddingModel))return false;
 auto* m=c.q38EmbeddingModel;
 auto* state=new(std::nothrow) PioneerQ38MixedLiveNativeState(c,*m,sRuntimeOwner,c.hashes,sRuntimeOwner->team);
 if(!state)return false;
 if(!c.io.MixedLiveBaseScope() || !state->MatchesBinding(c,*m,sRuntimeOwner,c.hashes,sRuntimeOwner->team,c.initialized,sReloadGeneration)){
  delete state;return false; // unpublished ordinary CPU allocation, no new map/card mutation
 }
 c.q38MixedLiveNative=state;return true;
}
[[maybe_unused]] static bool PrepareQ38MixedLiveNativeHeld(PioneerStoreContext& c,PioneerQ38MixedLiveMemoryResult& r){
 auto* state=c.q38MixedLiveNative;auto* m=c.q38EmbeddingModel;
 const PioneerQ38MixedLiveMemoryResult pristine;
 const auto* a=reinterpret_cast<const unsigned char*>(&r);const auto* b=reinterpret_cast<const unsigned char*>(&pristine);
 static_assert(sizeof(r)==12*sizeof(uint32_t),"every native result byte has an explicit initial value");
 for(unsigned i=0;i<sizeof(r);i++)if(a[i]!=b[i])return false;
 if(!state || !m || state->phase!=PioneerQ38MixedLiveNativeState::Bound || state->memory || state->plan
  || c.q38Dispatch || c.io.q38MixedLiveMemoryMapAttempted || !c.io.MixedLiveBaseScope() || !MixedLiveLearnedPages(*m))return false;
 auto* memory=new(std::nothrow) PioneerQ38MixedLiveMemory;
 auto* plan=new(std::nothrow) PioneerQ38MixedLiveBlockPlan;
 auto* participants=new(std::nothrow) PioneerQ38DispatchLease;
 if(!memory || !plan || !participants){
  delete participants;delete plan;delete memory;c.q38MixedLiveNative=NULL;delete state;return false; // no new IO yet
 }
 state->memory=memory;state->plan=plan;state->phase=PioneerQ38MixedLiveNativeState::DispatchPreparing;c.q38Dispatch=participants;
 auto& dram=c.baseline.baseline.baseline.baseline.dram;auto& noc=c.baseline.baseline.baseline.noc;
 auto& lease=*participants;lease.worker[0].master=&c.lease;lease.worker[0].noc=noc;c.io.fleetEnabled=true;
 bool okay=c.io.SelectRuntimeCore(0) && c.lease.ready && c.io.MapRuntime() && BlackholeDramAllCoresHeld(c.io,dram);
 for(unsigned w=1;w<120 && okay;w++)okay=PioneerWorkerNocProbe(c.io,dram,w,lease.worker[w].noc);
 c.q38DispatchRoutes=okay;
 for(unsigned w=0;w<120 && okay;w++){
  auto& node=lease.worker[w];okay=c.io.SelectRuntimeCore(w) && c.io.MapQ38Panel();
  if(okay && w)okay=PioneerQ38CohortPeerLeaseBegin(c.io,dram,w,node);
  if(okay)okay=PioneerQ38DispatchPrepareRole(c.io,dram,c.banks,lease,w);
  if(!okay)lease.prepared=w+1;
 }
 // This fixed layer3/position0 standalone operator uses the genuine raw
 // embedding producer; it is NOT a claim that GDN layers0..2 executed.
 // Future full-model execution MUST supply the correctly sequenced hidden
 // producer and initialized/committed KV ownership, not reuse this shortcut.
 if(okay)okay=plan->Build(m->plan,3,0,m->token,m->bank,2,1,m->identity,&lease.authority.source[0][0],m->bank,
  m->plan.Base+m->plan.Workspace(m->bank)->offset) && plan->LearnedPagesPresent(c.initialized,PioneerResidentBanks::Pages);
 if(okay){
  state->phase=PioneerQ38MixedLiveNativeState::MemoryPreparing;
  okay=c.io.SelectRuntimeCore(0) && c.io.MapMixedLiveMemory()
   && memory->PrepareHeld(c.io,dram,c.banks,lease,m->plan,m->identity,c.initialized,PioneerResidentBanks::Pages,r);
 }
 if(okay)state->phase=PioneerQ38MixedLiveNativeState::MemoryInstalled;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
} // real native held preparation only; no reset/start/NoC arithmetic/Work or token publication
[[maybe_unused]] static bool CloseQ38MixedLiveNativeHeld(PioneerStoreContext& c,PioneerQ38MixedLiveMemoryResult& r){
 auto* state=c.q38MixedLiveNative;
 if(!state || state->phase!=PioneerQ38MixedLiveNativeState::MemoryInstalled || !state->memory || state->execution || state->Unsafe())return false;
 state->phase=PioneerQ38MixedLiveNativeState::MemoryClosing;
 bool okay=c.io.MixedLiveBaseScope() && state->memory->CloseHeldNoLaunch(c.io,r);
 if(okay)state->phase=PioneerQ38MixedLiveNativeState::MemoryClosed;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
}
[[maybe_unused]] static bool PrepareQ38MixedLiveNativeExecution(PioneerStoreContext& c,PioneerQ38MixedLiveResult& r){
 auto* state=c.q38MixedLiveNative;auto* m=c.q38EmbeddingModel;
 const PioneerQ38MixedLiveResult pristine;
 const auto* actual=reinterpret_cast<const unsigned char*>(&r);const auto* initial=reinterpret_cast<const unsigned char*>(&pristine);
 static_assert(sizeof(r)==16*sizeof(uint32_t),"every execution result byte has an explicit initial value");
 for(unsigned i=0;i<sizeof(r);i++)if(actual[i]!=initial[i])return false; // BEFORE publishing an owner or any IO
 if(!state || !m || state->phase!=PioneerQ38MixedLiveNativeState::MemoryInstalled || state->execution || !state->memory || !state->plan
  || !c.q38Dispatch || !c.io.MixedLiveBaseScope())return false;
 auto* execution=new(std::nothrow) PioneerQ38MixedLiveOwner;if(!execution)return false;
 state->execution=execution;state->phase=PioneerQ38MixedLiveNativeState::ExecutionPreparing;
 bool okay=execution->Prepare(c.io,c.baseline.baseline.baseline.baseline.dram,c.banks,*c.q38Dispatch,*state->memory,m->plan,*state->plan,
  m->identity,c.initialized,PioneerResidentBanks::Pages,r);
 if(okay)state->phase=PioneerQ38MixedLiveNativeState::ExecutionReady;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
}
[[maybe_unused]] static bool RunQ38MixedLiveNativeExecution(PioneerStoreContext& c,PioneerQ38MixedLiveResult& r){
 auto* state=c.q38MixedLiveNative;
 if(!state || state->phase!=PioneerQ38MixedLiveNativeState::ExecutionReady || !state->execution || state->Unsafe())return false;
 bool okay=c.io.MixedLiveBaseScope() && state->execution->Run(c.io,r);
 if(okay)state->phase=PioneerQ38MixedLiveNativeState::ExecutionComplete;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
}
[[maybe_unused]] static bool CloseQ38MixedLiveNativeExecution(PioneerStoreContext& c,PioneerQ38MixedLiveResult& r,PioneerQ38MixedLiveMemoryResult& memory){
 auto* state=c.q38MixedLiveNative;
 if(!state || state->phase!=PioneerQ38MixedLiveNativeState::ExecutionComplete || !state->execution || state->Unsafe())return false;
 state->phase=PioneerQ38MixedLiveNativeState::ExecutionClosing;
 bool okay=c.io.MixedLiveBaseScope() && state->execution->Close(c.io,r,memory);
 if(okay)state->phase=PioneerQ38MixedLiveNativeState::MemoryClosed;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
}
#ifdef PIONEER_Q38_MIXED_LIVE_VALIDATION_READBACK
[[maybe_unused]] static bool CloseQ38MixedLiveNativeValidation(PioneerStoreContext& c,PioneerQ38MixedLiveResult& r,PioneerQ38MixedLiveMemoryResult& memory,
 uint32_t* values,unsigned words,PioneerQ38MixedLiveReadbackResult& out){
 auto* state=c.q38MixedLiveNative;
 if(!state || state->phase!=PioneerQ38MixedLiveNativeState::ExecutionComplete || !state->execution || state->Unsafe()
  || !state->execution->CanCloseForValidation(values,words,out,r,memory))return false; // BEFORE any state change/IO
 state->phase=PioneerQ38MixedLiveNativeState::ExecutionClosing;
 bool okay=c.io.MixedLiveBaseScope() && state->execution->CloseForValidation(c.io,r,memory,values,words,out);
 if(okay)state->phase=PioneerQ38MixedLiveNativeState::MemoryClosed;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
} // private NODRV compile path ONLY;no public ioctl/Work promotion
#endif
#endif
