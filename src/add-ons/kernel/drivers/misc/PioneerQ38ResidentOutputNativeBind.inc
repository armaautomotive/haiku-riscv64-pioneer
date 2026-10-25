/* SPDX-License-Identifier: MIT */
// Genuine native FD/team/thread + complete retained-context bind perimeter.
// This is NOT a zero/compute factory and does not permit the new role to
// enter a driver. No PCI/MMIO/map/reset/STOP/free or public selector here.
bool PioneerSessionIO::ResidentOutputCallScopeMatches()
{
 auto* c=q38MutableContext;auto* fd=sRuntimeOwner;
 if(!c || &c->io!=this || !fd || fd->magic!=0x50313030 || fd->context!=c || fd->team<0
  || c->q38GdnCallThread<0 || c->q38GdnCallThread!=find_thread(NULL))return false;
 thread_info info={};
 return get_thread_info(c->q38GdnCallThread,&info)==B_OK && info.team==fd->team
  && sRuntimeOwner==fd && fd->context==c && c->q38GdnCallThread==find_thread(NULL);
}
bool PioneerSessionIO::ResidentOutputInitializationBaseScope()
{
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
 if(q38MutableContext && q38MutableContext->q38ResidentLayerNative)return false;
#endif
 auto* c=q38MutableContext;auto* m=c?c->q38EmbeddingModel:NULL;
 if(!c || &c->io!=this || !m || !ResidentOutputCallScopeMatches()
  || sColdRecoveryRequired || sReloadQuiesced || !sReloadGeneration || !sBankStorageOnly || !c->bankStorageOnly
  || c->bytes!=PioneerResidentBanks::Total || c->runtimeCoreMode!=1 || runtimeCore || fleetEnabled || !runtimeMapped
  || q38SpanAllowed || c->dispatchOwnerRetained || !c->initialized || !c->hashes || m->ledger!=c->initialized
  || !m->identity.Valid() || m->identity.driverGeneration!=sReloadGeneration || m->position || m->token>=248320 || m->bank>=7
  || m->phase!=PioneerQ38EmbeddingNativeState::Bound || m->Unsafe() || !m->plan.Ready() || m->plan.RecurrentCount()!=672
  || !PioneerQ38ProjectionZeroPristine(m->zero) || m->zero.closed || m->embedding.Completed() || m->embedding.Running()
  || m->rms.Claimed() || m->projection.Claimed() || m->projectionCohort
  || q38EmbeddingMemoryMapAttempted || q38ProjectionMemoryMapAttempted
  || c->q38FfnModelPlan || c->q38GraphMutable || c->q38FfnZero || c->q38FfnGraph || c->q38FfnCode
  || c->q38FfnModelIdentity.session || c->q38FfnModelIdentity.sequence || c->q38FfnModelIdentity.modelEpoch || c->q38FfnModelIdentity.driverGeneration
  || c->q38GdnBanks || c->q38StateZero || c->q38GdnIdentity.session || c->q38GdnIdentity.sequence
  || c->q38GdnIdentity.modelEpoch || c->q38GdnIdentity.driverGeneration || c->q38Dispatch || c->q38DispatchRoutes
  || c->q38Cohort || c->fleet || sQ38LocalDeltaOwner
  || c->q38Delta.saved || c->q38Delta.dirty || c->q38Delta.ready || c->q38Delta.running || c->q38Delta.awaiting
  || c->panelProbe.saved || c->panelProbe.dirty || c->panelProbe.ready || c->streamProbe.saved || c->streamProbe.dirty || c->streamProbe.ready
  || !c->q38Span || !c->q38Span->closed || c->q38Span->dirty || c->q38Span->ready || c->q38Span->running || c->q38Span->unsafe
  || !c->lease.saved || !c->lease.ready || c->lease.pending || c->lease.recovery_required
  || !c->bulk.saved || !c->bulk.ready || c->bulk.unsafe
  || c->host.state!=TT_HOST_PAGE_READY || c->host.released || PioneerHostPageIO::quarantined
  || !c->bulkHost.allocated || !c->bulkHost.ready || c->bulkHost.pending || c->bulkHost.quarantined || c->bulkHost.released
  || !c->wideHost.allocated || !c->wideHost.ready || !c->wideHost.memory_type_set || c->wideHost.pending
  || c->wideHost.quarantined || c->wideHost.released || wideQuarantined
  || m->graph.CanonicalModel(m->identity)!=&m->plan || !m->graph.MatchesInitializationOwner(c->banks,dram,m->identity))return false;
 auto* g=m->graph.Recurrent(m->identity);auto* h=m->graph.ConvHistory(m->identity);
 if(!g || !h || !g->MatchesInitializationOwner(c->banks,dram,m->identity) || !h->MatchesInitializationOwner(c->banks,dram,m->identity)
  )return false;
 const auto& clock=c->q38Clock.Snapshot();
 return c->q38Clock.RequiresExplicitEnd() && clock.state==TT_ARC_CLOCK_BUSY_VERIFIED && clock.busy_requested
  && clock.busy_acknowledged && !clock.restored && !c->Unsafe(true,false,true);
}
bool PioneerSessionIO::ResidentOutputBindSourceScope()
{
 auto* c=q38MutableContext;
 if(!ResidentOutputInitializationBaseScope() || c->q38ResidentOutputNative || c->bulk.dirty)return false;
 auto* m=c->q38EmbeddingModel;auto* g=m->graph.Recurrent(m->identity);auto* h=m->graph.ConvHistory(m->identity);
 unsigned pages=0;
 return g && h && !g->InitializationPublished(m->identity) && !h->InitializationPublished(m->identity)
  && g->InitializationProgress(m->identity,pages) && !pages && h->InitializationProgress(m->identity,pages) && !pages;
}
[[maybe_unused]] static bool BindQ38ResidentOutputNativeState(PioneerStoreContext& c)
{
 if(!c.io.ResidentOutputBindSourceScope())return false;
 auto* m=c.q38EmbeddingModel;
 // All original first-layer learned input/Conv/GDN/output pages BEFORE
 // publishing the new native lifetime. This bitmap is not GGUF provenance;
 // SAME-FILE host authentication remains an independent prerequisite.
 for(unsigned i=0;i<m->plan.Count();i++){
  const auto* p=m->plan.Part(i);
  if(p->kind!=m->plan.WeightKind || p->layer || p->role<7 || p->role>13)continue;
  if(p->bank>=7 || (p->offset&4095) || (p->allocated&4095) || !p->allocated)return false;
  unsigned first=p->bank*unsigned(m->plan.Capacity/4096)+p->offset/4096;
  if(first>=PioneerResidentBanks::Pages || p->allocated/4096>PioneerResidentBanks::Pages-first)return false;
  for(unsigned page=0;page<p->allocated/4096;page++)if(c.initialized[first+page]!=1)return false;
 }
 auto* state=new(std::nothrow) PioneerQ38ResidentOutputNativeState(c,*m,sRuntimeOwner,c.hashes,sRuntimeOwner->team);
 if(!state)return false; // original prefix still retained,NO IO or partial publication
 if(!c.io.ResidentOutputBindSourceScope()
  || !state->MatchesBinding(c,*m,sRuntimeOwner,c.hashes,sRuntimeOwner->team,c.initialized,sReloadGeneration)){
  delete state;return false; // unpublished CPU object ONLY;no map/journal/card ownership was created
 }
 c.q38ResidentOutputNative=state;return true;
}
