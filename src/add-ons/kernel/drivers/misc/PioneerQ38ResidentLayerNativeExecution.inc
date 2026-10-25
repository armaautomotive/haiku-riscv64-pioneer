/* SPDX-License-Identifier: MIT */
// SAME genuine native context,not an old closed-input/ready/map promotion.
// This role remains OFFLINE until complete actual native execution and
// physical qualification. No public opcode or Work/CPU selector here.
bool PioneerSessionIO::ResidentLayerSourceScope(bool claimed)
{
 auto* c=q38MutableContext;auto* m=c?c->q38EmbeddingModel:NULL;auto* state=c?c->q38ResidentLayerNative:NULL;
 if(!c || &c->io!=this || !m || !state || !ResidentLayerCallScopeMatches()
  || c->q38ResidentOutputNative || q38ResidentOutputMemoryMapAttempted
  || sColdRecoveryRequired || sReloadQuiesced || !sReloadGeneration || !sBankStorageOnly || !c->bankStorageOnly
  || c->bytes!=PioneerResidentBanks::Total || c->runtimeCoreMode!=1 || runtimeCore>=120 || !runtimeMapped || !fleetEnabled
  || q38SpanAllowed || c->dispatchOwnerRetained || !c->initialized || !c->hashes || m->ledger!=c->initialized
  || !state->MatchesBinding(*c,*m,sRuntimeOwner,c->hashes,sRuntimeOwner->team,c->initialized,sReloadGeneration)
  || !state->InitializationsClosed(c->banks,dram) || !m->Closed() || m->phase!=PioneerQ38EmbeddingNativeState::ProjectionComplete
  || !c->q38Dispatch || !c->q38DispatchRoutes || m->projectionCohort!=c->q38Dispatch || c->q38Dispatch->worker[0].master!=&c->lease
  || !state->ControlsLayerCohort(c->banks,dram,c->q38Dispatch,c->initialized,PioneerResidentBanks::Pages)
  || (claimed && (!state->memory || !state->layer.Claimed()))
  || c->q38FfnModelPlan || c->q38GraphMutable || c->q38FfnZero || c->q38FfnGraph || c->q38FfnCode
  || c->q38FfnModelIdentity.session || c->q38FfnModelIdentity.sequence || c->q38FfnModelIdentity.modelEpoch || c->q38FfnModelIdentity.driverGeneration
  || c->q38GdnBanks || c->q38StateZero || c->q38GdnIdentity.session || c->q38GdnIdentity.sequence
  || c->q38GdnIdentity.modelEpoch || c->q38GdnIdentity.driverGeneration || c->q38Cohort || c->fleet || sQ38LocalDeltaOwner
  || c->q38Delta.saved || c->q38Delta.dirty || c->q38Delta.ready || c->q38Delta.running || c->q38Delta.awaiting
  || c->panelProbe.saved || c->panelProbe.dirty || c->panelProbe.ready || c->streamProbe.saved || c->streamProbe.dirty || c->streamProbe.ready
  || !c->q38Span || !c->q38Span->closed || c->q38Span->dirty || c->q38Span->ready || c->q38Span->running || c->q38Span->unsafe
  || !c->lease.saved || !c->lease.ready || c->lease.pending || c->lease.recovery_required
  || !c->bulk.saved || !c->bulk.ready || c->bulk.dirty || c->bulk.unsafe
  || c->host.state!=TT_HOST_PAGE_READY || c->host.released || PioneerHostPageIO::quarantined
  || !c->bulkHost.allocated || !c->bulkHost.ready || c->bulkHost.pending || c->bulkHost.quarantined || c->bulkHost.released
  || !c->wideHost.allocated || !c->wideHost.ready || !c->wideHost.memory_type_set || c->wideHost.pending
  || c->wideHost.quarantined || c->wideHost.released || wideQuarantined
  || m->graph.CanonicalModel(m->identity)!=&m->plan || !m->graph.MatchesInitializationOwner(c->banks,dram,m->identity)
  || !m->rms.MatchesNativeOwner(dram,noc,c->banks,m->graph,m->plan,m->embedding,m->identity,c->initialized,PioneerResidentBanks::Pages))return false;
 const auto& clock=c->q38Clock.Snapshot();
 return c->q38Clock.RequiresExplicitEnd() && clock.state==TT_ARC_CLOCK_BUSY_VERIFIED && clock.busy_requested
  && clock.busy_acknowledged && !clock.restored && clock.busy_mhz && clock.busy_mhz<=1400 && !c->Unsafe(true,false,true);
} // metadata ONLY;each real controller guard independently owes exact physical source/code/reset/NoC/counters/maps
bool PioneerSessionIO::ResidentLayerMappingSourceScope()
{
 auto* c=q38MutableContext;auto* state=c?c->q38ResidentLayerNative:NULL;
 return state && !runtimeCore && state->phase==PioneerQ38ResidentLayerNativeState::LayerPreparing && state->memory
  && !state->layer.Claimed() && ResidentLayerSourceScope(false);
}
bool PioneerSessionIO::ResidentLayerMemoryScope(const PioneerQ38ResidentLayerMemory& code,const PioneerQ38CanonicalModelPlan& plan,
 const PioneerQ38GdnIdentity& id,const uint8_t* learned,unsigned pages)
{
 auto* c=q38MutableContext;auto* m=c?c->q38EmbeddingModel:NULL;auto* state=c?c->q38ResidentLayerNative:NULL;
 return m && state && state->memory==&code && &plan==&m->plan && id==m->identity && learned==c->initialized
  && pages==PioneerResidentBanks::Pages && ResidentLayerSourceScope(true)
  && state->layer.MatchesNativeOwner(m->projection,code);
} // deliberately NOT code.MatchesNativeOwner:that held-only helper MUST refuse a legitimately launched new owner
bool PioneerSessionIO::ResidentLayerExecutionScope(const PioneerQ38ResidentLayerOwner& owner,const PioneerQ38ProjectionGraphOwner& input,
 const PioneerQ38ResidentLayerMemory& code,const PioneerQ38ProjectionMutableBanks& graph,const PioneerQ38CanonicalModelPlan& plan,
 const PioneerQ38GdnIdentity& id,const uint8_t* learned,unsigned pages)
{
 auto* c=q38MutableContext;auto* m=c?c->q38EmbeddingModel:NULL;auto* state=c?c->q38ResidentLayerNative:NULL;
 return m && state && &owner==&state->layer && &input==&m->projection && &graph==&m->graph
  && ResidentLayerMemoryScope(code,plan,id,learned,pages);
}
void PioneerSessionIO::ResidentLayerRetainHostOwners()
{
 if(q38MutableContext){tt_resident_session_result retained={};RetainQ38DispatchOwner(*q38MutableContext,retained);}
} // MEMORY ONLY,never STOP/reset/restore/map release/retry after uncertainty
[[maybe_unused]] static bool PrepareQ38ResidentLayerNative(PioneerStoreContext& c,PioneerQ38ResidentLayerResult& r)
{
 static_assert(sizeof(PioneerQ38ResidentLayerResult)==28*sizeof(uint32_t),"audit every result byte,NO padding");
 PioneerQ38ResidentLayerResult pristine;
 const auto* actual=reinterpret_cast<const unsigned char*>(&r);const auto* expected=reinterpret_cast<const unsigned char*>(&pristine);
 for(unsigned i=0;i<sizeof(r);i++)if(actual[i]!=expected[i])return false;
 auto* state=c.q38ResidentLayerNative;auto* m=c.q38EmbeddingModel;
 if(!state || !m || state->phase!=PioneerQ38ResidentLayerNativeState::Bound || state->memory || state->layer.Claimed()
  || c.io.runtimeCore || c.io.q38ResidentLayerMemoryMapAttempted || !c.io.ResidentLayerSourceScope(false))return false;
 // Current exact learned page DATA before new maps/claims/IO. The genuine
 // controller builds/validates its complete private recipes again before
 // its FIRST physical IO, and rechecks those uploads before RUN.
 if(!PioneerQ38ResidentOutputLearned(m->plan,c.initialized,PioneerResidentBanks::Pages))return false;
 auto* memory=new(std::nothrow) PioneerQ38ResidentLayerMemory;if(!memory)return false; // no publication/IO yet
 state->memory=memory;state->phase=PioneerQ38ResidentLayerNativeState::LayerPreparing;
 bool okay=c.io.MapResidentLayerMemory() && state->layer.Prepare(c.io,m->projection,*memory,r);
 if(okay)state->phase=PioneerQ38ResidentLayerNativeState::LayerRunning;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
}
[[maybe_unused]] static bool RunQ38ResidentLayerNative(PioneerStoreContext& c,PioneerQ38ResidentLayerResult& r)
{
 auto* state=c.q38ResidentLayerNative;
 if(!state || state->phase!=PioneerQ38ResidentLayerNativeState::LayerRunning || state->Unsafe())return false;
 bool okay=c.io.ResidentLayerSourceScope(true) && state->layer.Run(c.io,r);
 if(okay)state->phase=PioneerQ38ResidentLayerNativeState::LayerComplete;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
}
[[maybe_unused]] static bool CloseQ38ResidentLayerNative(PioneerStoreContext& c,PioneerQ38ResidentLayerResult& r)
{
 auto* state=c.q38ResidentLayerNative;
 if(!state || state->phase!=PioneerQ38ResidentLayerNativeState::LayerComplete || state->Unsafe())return false;
 state->phase=PioneerQ38ResidentLayerNativeState::LayerClosing;
 bool okay=c.io.ResidentLayerSourceScope(true) && state->layer.Close(c.io,r)
  && state->layer.MatchesClosedNativeOwner(c.q38EmbeddingModel->projection,*state->memory);
 if(okay)state->phase=PioneerQ38ResidentLayerNativeState::LayerClosed;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
}
[[maybe_unused]] static bool CloseQ38ResidentLayerNativeForValidation(PioneerStoreContext& c,PioneerQ38ResidentLayerResult& r,
 uint32_t* values,unsigned words,PioneerQ38ResidentLayerReadbackResult& readback)
{
 auto* state=c.q38ResidentLayerNative;
 if(!state || state->phase!=PioneerQ38ResidentLayerNativeState::LayerComplete || state->Unsafe()
  || !PioneerQ38ResidentLayerReadbackPristine(values,words,readback))return false;
 state->phase=PioneerQ38ResidentLayerNativeState::LayerClosing;
 bool okay=c.io.ResidentLayerSourceScope(true) && state->layer.CloseForValidation(c.io,r,values,words,readback)
  && readback.completed && readback.pages==PioneerQ38ResidentLayerReadbackPlan::Pages
  && readback.words==PioneerQ38ResidentLayerReadbackPlan::Words
  && state->layer.MatchesClosedNativeOwner(c.q38EmbeddingModel->projection,*state->memory);
 if(okay)state->phase=PioneerQ38ResidentLayerNativeState::LayerClosed;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
} // private QLV1 observer ONLY;NO public request/Work-mode registration
