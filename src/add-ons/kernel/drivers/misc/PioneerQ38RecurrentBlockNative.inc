/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT
#include "PioneerQ38RecurrentBlockNativeRole.h"
bool PioneerSessionIO::RecurrentBlockCallScopeMatches(){
 auto* c=q38MutableContext;auto* fd=sRuntimeOwner;
 if(!c || &c->io!=this || !fd || fd->magic!=0x50313030 || fd->context!=c || fd->team<0
  || c->q38GdnCallThread<0 || c->q38GdnCallThread!=find_thread(NULL))return false;
 thread_info info={};
 return get_thread_info(c->q38GdnCallThread,&info)==B_OK && info.team==fd->team
  && sRuntimeOwner==fd && fd->context==c && c->q38GdnCallThread==find_thread(NULL);
}
bool PioneerSessionIO::RecurrentBlockBaseScope(){
 auto* c=q38MutableContext;auto* m=c?c->q38EmbeddingModel:NULL;auto* state=c?c->q38RecurrentBlockNative:NULL;
 if(!c || &c->io!=this || !m || !RecurrentBlockCallScopeMatches() || sColdRecoveryRequired || sReloadQuiesced || !sReloadGeneration
  || !sBankStorageOnly || !c->bankStorageOnly || c->bytes!=PioneerResidentBanks::Total || c->runtimeCoreMode!=1
  || runtimeCore>=120 || !runtimeMapped || q38SpanAllowed || c->dispatchOwnerRetained || !c->initialized || !c->hashes
  || m->ledger!=c->initialized || !m->identity.Valid() || m->identity.driverGeneration!=sReloadGeneration
  || m->position || m->token>=248320 || m->bank>=7 || m->phase!=PioneerQ38EmbeddingNativeState::Complete || !m->Closed()
  || !PioneerQ38RecurrentBlockNativeState::CompletedInput(*m)
  || !m->embedding.MatchesNativeOwner(dram,noc,c->banks,m->graph,m->plan,m->identity,c->initialized,PioneerResidentBanks::Pages)
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
  || !c->bulk.saved || !c->bulk.ready || (c->bulk.dirty && (!state || state->phase<PioneerQ38RecurrentBlockNativeState::RecurrentZeroing
   || state->phase>PioneerQ38RecurrentBlockNativeState::HistoryClosing)) || c->bulk.unsafe || c->host.state!=TT_HOST_PAGE_READY || c->host.released
  || PioneerHostPageIO::quarantined || !c->bulkHost.allocated || !c->bulkHost.ready || c->bulkHost.pending
  || c->bulkHost.quarantined || c->bulkHost.released || !c->wideHost.allocated || !c->wideHost.ready || !c->wideHost.memory_type_set
  || c->wideHost.pending || c->wideHost.quarantined || c->wideHost.released || wideQuarantined)return false;
 if(state){
  if(!state->MatchesBinding(*c,*m,sRuntimeOwner,c->hashes,sRuntimeOwner->team,c->initialized,sReloadGeneration))return false;
  if(state->phase<=PioneerQ38RecurrentBlockNativeState::HistoryClosing){if(runtimeCore || c->q38Dispatch || c->q38DispatchRoutes || fleetEnabled || q38RecurrentBlockMemoryMapAttempted)return false;}
  else if(!c->q38Dispatch || c->q38Dispatch->worker[0].master!=&c->lease || !fleetEnabled || c->q38Dispatch->unsafe
   || c->q38Dispatch->closed || c->q38Dispatch->booted || c->q38Dispatch->bootAttempted || c->q38Dispatch->rootRunning)return false;
  if(state->phase>=PioneerQ38RecurrentBlockNativeState::MemoryPreparing
   && (!state->memory || !state->plan || !c->q38DispatchRoutes || c->q38Dispatch->prepared!=120 || !state->plan->Matches(m->plan)))return false;
 }else if(runtimeCore || c->q38Dispatch || c->q38DispatchRoutes || fleetEnabled || q38RecurrentBlockMemoryMapAttempted)return false;
 const auto& clock=c->q38Clock.Snapshot();
 return c->q38Clock.RequiresExplicitEnd() && clock.state==TT_ARC_CLOCK_BUSY_VERIFIED && clock.busy_requested && clock.busy_acknowledged
  && !clock.restored && clock.busy_mhz && clock.busy_mhz<=1400 && !c->Unsafe(true,false,true);
} // never infers initialized-state or compute permission from geometry
bool PioneerSessionIO::RecurrentBlockMappingSourceScope(){
 auto* c=q38MutableContext;auto* state=c?c->q38RecurrentBlockNative:NULL;
 return state && !runtimeCore && state->phase==PioneerQ38RecurrentBlockNativeState::MemoryPreparing && state->memory
  && !state->memory->Installed() && !state->memory->Closed() && RecurrentBlockBaseScope();
}
bool PioneerSessionIO::RecurrentBlockMemoryAccessScope(){
 auto* c=q38MutableContext;auto* state=c?c->q38RecurrentBlockNative:NULL;auto* m=c?c->q38EmbeddingModel:NULL;auto* fd=sRuntimeOwner;
 // Cheap genuine per-WORD cookie/thread/generation/source-pointer/phase
 // check under the SAME global call lock. Full physical/native/clock/host
 // guards remain mandatory before AND after each participant's pass.
 return c && &c->io==this && state && m && fd && fd->magic==0x50313030 && fd->context==c && c->q38GdnCallThread>=0
  && c->q38GdnCallThread==find_thread(NULL) && !c->dispatchOwnerRetained && !sColdRecoveryRequired && !sReloadQuiesced
  && c->bytes==PioneerResidentBanks::Total && c->runtimeCoreMode==1 && state->memory && !state->memory->Unsafe() && !state->memory->Closed()
  && (state->phase==PioneerQ38RecurrentBlockNativeState::MemoryPreparing || state->phase==PioneerQ38RecurrentBlockNativeState::MemoryInstalled
   || state->phase==PioneerQ38RecurrentBlockNativeState::MemoryClosing)
  && state->MatchesBinding(*c,*m,fd,c->hashes,fd->team,c->initialized,sReloadGeneration);
}
bool PioneerSessionIO::RecurrentBlockMemoryScope(const PioneerQ38RecurrentBlockMemory& memory,const PioneerQ38CanonicalModelPlan& canonical,
 const PioneerQ38GdnIdentity& id,const uint8_t* learned,unsigned pages){
 auto* c=q38MutableContext;auto* state=c?c->q38RecurrentBlockNative:NULL;auto* m=c?c->q38EmbeddingModel:NULL;
 return state && m && state->memory==&memory && &canonical==&m->plan && id==m->identity && learned==c->initialized
  && pages==PioneerResidentBanks::Pages && RecurrentBlockMemoryAccessScope() && RecurrentBlockBaseScope();
}
void PioneerSessionIO::RecurrentBlockRetainHostOwners(){
 if(q38MutableContext){tt_resident_session_result retained={};RetainQ38DispatchOwner(*q38MutableContext,retained);}
}
[[maybe_unused]] static bool BindQ38RecurrentBlockNative(PioneerStoreContext& c){
 if(c.q38RecurrentBlockNative || c.io.runtimeCore || !c.io.RecurrentBlockBaseScope()
  || !PioneerQ38ResidentOutputLearned(c.q38EmbeddingModel->plan,c.initialized,PioneerResidentBanks::Pages))return false;
 auto* m=c.q38EmbeddingModel;auto* state=new(std::nothrow) PioneerQ38RecurrentBlockNativeState(c,*m,sRuntimeOwner,c.hashes,sRuntimeOwner->team);
 if(!state)return false;
 if(!c.io.RecurrentBlockBaseScope() || !state->MatchesBinding(c,*m,sRuntimeOwner,c.hashes,sRuntimeOwner->team,c.initialized,sReloadGeneration)){
  delete state;return false; // unpublished ordinary CPU allocation ONLY
 }
 c.q38RecurrentBlockNative=state;return true;
}
#include "PioneerQ38RecurrentBlockNativeInitialize.inc"
[[maybe_unused]] static bool PrepareQ38RecurrentBlockNativeHeld(PioneerStoreContext& c,PioneerQ38RecurrentBlockMemoryResult& r){
 auto* state=c.q38RecurrentBlockNative;auto* m=c.q38EmbeddingModel;const PioneerQ38RecurrentBlockMemoryResult pristine;
 const auto* a=reinterpret_cast<const unsigned char*>(&r);const auto* b=reinterpret_cast<const unsigned char*>(&pristine);
 static_assert(sizeof(r)==48,"every result byte has an explicit pristine value");
 for(unsigned i=0;i<sizeof(r);i++)if(a[i]!=b[i])return false;
 if(!state || !m || state->phase!=PioneerQ38RecurrentBlockNativeState::Bound || state->memory || state->plan || c.q38Dispatch
  || c.io.q38RecurrentBlockMemoryMapAttempted || !c.io.RecurrentBlockBaseScope()
  || !PioneerQ38ResidentOutputLearned(m->plan,c.initialized,PioneerResidentBanks::Pages))return false;
 auto* memory=new(std::nothrow) PioneerQ38RecurrentBlockMemory;
 auto* plan=new(std::nothrow) PioneerQ38MixedRecurrentBlockPlan;auto* participants=new(std::nothrow) PioneerQ38DispatchLease;
 if(!memory || !plan || !participants){delete participants;delete plan;delete memory;c.q38RecurrentBlockNative=NULL;delete state;return false;}
 state->memory=memory;state->plan=plan;state->phase=PioneerQ38RecurrentBlockNativeState::DispatchPreparing;c.q38Dispatch=participants;
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
 if(okay)okay=plan->Build(m->plan,0,0,m->token,m->bank,0,1001,1,m->identity,&lease.authority.source[0][0],m->bank,
  m->plan.Base+m->plan.Workspace(m->bank)->offset) && plan->LearnedPagesPresent(c.initialized,PioneerResidentBanks::Pages);
 // HELD preparation only. Current recurrent/history initialization and
 // execution ownership are deliberately NOT inferred from canonical DATA.
 if(okay){state->phase=PioneerQ38RecurrentBlockNativeState::MemoryPreparing;
  okay=c.io.SelectRuntimeCore(0) && c.io.MapRecurrentBlockMemory()
   && memory->PrepareHeld(c.io,dram,c.banks,lease,m->plan,m->identity,c.initialized,PioneerResidentBanks::Pages,r);}
 if(okay)state->phase=PioneerQ38RecurrentBlockNativeState::MemoryInstalled;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
}
[[maybe_unused]] static bool CloseQ38RecurrentBlockNativeHeld(PioneerStoreContext& c,PioneerQ38RecurrentBlockMemoryResult& r){
 auto* state=c.q38RecurrentBlockNative;
 if(!state || state->phase!=PioneerQ38RecurrentBlockNativeState::MemoryInstalled || !state->memory || state->Unsafe())return false;
 state->phase=PioneerQ38RecurrentBlockNativeState::MemoryClosing;
 bool okay=c.io.RecurrentBlockBaseScope() && state->memory->CloseHeldNoLaunch(c.io,r);
 if(okay)state->phase=PioneerQ38RecurrentBlockNativeState::MemoryClosed;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
}
#endif
