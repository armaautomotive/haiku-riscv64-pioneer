/* SPDX-License-Identifier: MIT */
// SAME genuine native context,not an old closed-input/ready/map promotion.
// This role remains OFFLINE until complete actual native execution and
// physical qualification. No public opcode or Work/CPU selector here.
bool PioneerSessionIO::ResidentOutputSourceScope(bool claimed)
{
 auto* c=q38MutableContext;auto* m=c?c->q38EmbeddingModel:NULL;auto* state=c?c->q38ResidentOutputNative:NULL;
 if(!c || &c->io!=this || !m || !state || !ResidentOutputCallScopeMatches()
  || sColdRecoveryRequired || sReloadQuiesced || !sReloadGeneration || !sBankStorageOnly || !c->bankStorageOnly
  || c->bytes!=PioneerResidentBanks::Total || c->runtimeCoreMode!=1 || runtimeCore>=120 || !runtimeMapped || !fleetEnabled
  || q38SpanAllowed || c->dispatchOwnerRetained || !c->initialized || !c->hashes || m->ledger!=c->initialized
  || !state->MatchesBinding(*c,*m,sRuntimeOwner,c->hashes,sRuntimeOwner->team,c->initialized,sReloadGeneration)
  || !state->InitializationsClosed(c->banks,dram) || !m->Closed() || m->phase!=PioneerQ38EmbeddingNativeState::ProjectionComplete
  || !c->q38Dispatch || !c->q38DispatchRoutes || m->projectionCohort!=c->q38Dispatch || c->q38Dispatch->worker[0].master!=&c->lease
  || !state->ControlsOutputCohort(c->banks,dram,c->q38Dispatch,c->initialized,PioneerResidentBanks::Pages)
  || (claimed && (!state->memory || !state->output.Claimed()))
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
bool PioneerSessionIO::ResidentOutputMappingSourceScope()
{
 auto* c=q38MutableContext;auto* state=c?c->q38ResidentOutputNative:NULL;
 return state && !runtimeCore && state->phase==PioneerQ38ResidentOutputNativeState::OutputPreparing && state->memory
  && !state->output.Claimed() && ResidentOutputSourceScope(false);
}
bool PioneerSessionIO::ResidentOutputMemoryScope(const PioneerQ38ResidentOutputMemory& code,const PioneerQ38CanonicalModelPlan& plan,
 const PioneerQ38GdnIdentity& id,const uint8_t* learned,unsigned pages)
{
 auto* c=q38MutableContext;auto* m=c?c->q38EmbeddingModel:NULL;auto* state=c?c->q38ResidentOutputNative:NULL;
 return m && state && state->memory==&code && &plan==&m->plan && id==m->identity && learned==c->initialized
  && pages==PioneerResidentBanks::Pages && ResidentOutputSourceScope(true)
  && state->output.MatchesNativeOwner(m->projection,code);
} // deliberately NOT code.MatchesNativeOwner:that held-only helper MUST refuse a legitimately launched new owner
bool PioneerSessionIO::ResidentOutputExecutionScope(const PioneerQ38ResidentOutputOwner& owner,const PioneerQ38ProjectionGraphOwner& input,
 const PioneerQ38ResidentOutputMemory& code,const PioneerQ38ProjectionMutableBanks& graph,const PioneerQ38CanonicalModelPlan& plan,
 const PioneerQ38GdnIdentity& id,const uint8_t* learned,unsigned pages)
{
 auto* c=q38MutableContext;auto* m=c?c->q38EmbeddingModel:NULL;auto* state=c?c->q38ResidentOutputNative:NULL;
 return m && state && &owner==&state->output && &input==&m->projection && &graph==&m->graph
  && ResidentOutputMemoryScope(code,plan,id,learned,pages);
}
void PioneerSessionIO::ResidentOutputRetainHostOwners()
{
 if(q38MutableContext){tt_resident_session_result retained={};RetainQ38DispatchOwner(*q38MutableContext,retained);}
} // MEMORY ONLY,never STOP/reset/restore/map release/retry after uncertainty
[[maybe_unused]] static bool PrepareQ38ResidentOutputNative(PioneerStoreContext& c,PioneerQ38ResidentOutputResult& r)
{
 static_assert(sizeof(PioneerQ38ResidentOutputResult)==28*sizeof(uint32_t),"audit every result byte,NO padding");
 PioneerQ38ResidentOutputResult pristine;
 const auto* actual=reinterpret_cast<const unsigned char*>(&r);const auto* expected=reinterpret_cast<const unsigned char*>(&pristine);
 for(unsigned i=0;i<sizeof(r);i++)if(actual[i]!=expected[i])return false;
 auto* state=c.q38ResidentOutputNative;auto* m=c.q38EmbeddingModel;
 if(!state || !m || state->phase!=PioneerQ38ResidentOutputNativeState::Bound || state->memory || state->output.Claimed()
  || c.io.runtimeCore || c.io.q38ResidentOutputMemoryMapAttempted || !c.io.ResidentOutputSourceScope(false))return false;
 // Current exact learned page DATA before new maps/claims/IO. The genuine
 // controller builds/validates its complete private recipes again before
 // its FIRST physical IO, and rechecks those uploads before RUN.
 for(unsigned i=0;i<m->plan.Count();i++){const auto* p=m->plan.Part(i);
  if(p->kind!=m->plan.WeightKind || p->layer || p->role>13)continue;
  if(p->bank>=7 || (p->offset&4095) || (p->allocated&4095) || !p->allocated)return false;
  unsigned first=p->bank*unsigned(m->plan.Capacity/4096)+p->offset/4096;
  if(first>=PioneerResidentBanks::Pages || p->allocated/4096>PioneerResidentBanks::Pages-first)return false;
  for(unsigned page=0;page<p->allocated/4096;page++)if(c.initialized[first+page]!=1)return false;
 }
 auto* memory=new(std::nothrow) PioneerQ38ResidentOutputMemory;if(!memory)return false; // no publication/IO yet
 state->memory=memory;state->phase=PioneerQ38ResidentOutputNativeState::OutputPreparing;
 bool okay=c.io.MapResidentOutputMemory() && state->output.Prepare(c.io,m->projection,*memory,r);
 if(okay)state->phase=PioneerQ38ResidentOutputNativeState::OutputRunning;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
}
[[maybe_unused]] static bool RunQ38ResidentOutputNative(PioneerStoreContext& c,PioneerQ38ResidentOutputResult& r)
{
 auto* state=c.q38ResidentOutputNative;
 if(!state || state->phase!=PioneerQ38ResidentOutputNativeState::OutputRunning || state->Unsafe())return false;
 bool okay=c.io.ResidentOutputSourceScope(true) && state->output.Run(c.io,r);
 if(okay)state->phase=PioneerQ38ResidentOutputNativeState::OutputComplete;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
}
[[maybe_unused]] static bool CloseQ38ResidentOutputNative(PioneerStoreContext& c,PioneerQ38ResidentOutputResult& r)
{
 auto* state=c.q38ResidentOutputNative;
 if(!state || state->phase!=PioneerQ38ResidentOutputNativeState::OutputComplete || state->Unsafe())return false;
 state->phase=PioneerQ38ResidentOutputNativeState::OutputClosing;
 bool okay=c.io.ResidentOutputSourceScope(true) && state->output.Close(c.io,r)
  && state->output.MatchesClosedNativeOwner(c.q38EmbeddingModel->projection,*state->memory);
 if(okay)state->phase=PioneerQ38ResidentOutputNativeState::OutputClosed;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
}
[[maybe_unused]] static bool CloseQ38ResidentOutputNativeForValidation(PioneerStoreContext& c,PioneerQ38ResidentOutputResult& r,
 uint32_t* values,unsigned words,PioneerQ38ResidentOutputReadbackResult& readback)
{
 auto* state=c.q38ResidentOutputNative;
 if(!state || state->phase!=PioneerQ38ResidentOutputNativeState::OutputComplete || state->Unsafe()
  || !PioneerQ38ResidentOutputReadbackPristine(values,words,readback))return false;
 state->phase=PioneerQ38ResidentOutputNativeState::OutputClosing;
 bool okay=c.io.ResidentOutputSourceScope(true) && state->output.CloseForValidation(c.io,r,values,words,readback)
  && readback.completed && readback.pages==PioneerQ38ResidentOutputReadbackPlan::Pages
  && readback.words==PioneerQ38ResidentOutputReadbackPlan::Words
  && state->output.MatchesClosedNativeOwner(c.q38EmbeddingModel->projection,*state->memory);
 if(okay)state->phase=PioneerQ38ResidentOutputNativeState::OutputClosed;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
}
