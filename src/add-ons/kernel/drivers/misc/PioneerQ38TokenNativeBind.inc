/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
#include "PioneerQ38TokenNativeRole.h"
bool PioneerSessionIO::TokenCallScopeMatches(){
 auto* c=q38MutableContext;auto* fd=sRuntimeOwner;
 if(!c || &c->io!=this || !fd || fd->magic!=0x50313030 || fd->context!=c || fd->team<0
  || c->q38GdnCallThread<0 || c->q38GdnCallThread!=find_thread(NULL))return false;
 thread_info info={};return get_thread_info(c->q38GdnCallThread,&info)==B_OK && info.team==fd->team
  && sRuntimeOwner==fd && fd->context==c && c->q38GdnCallThread==find_thread(NULL);
}
bool PioneerSessionIO::TokenModelSourceScope(bool beginning){
 auto* c=q38MutableContext;auto* state=c?c->q38TokenNative:NULL;auto* fd=sRuntimeOwner;
 const bool cohort=state && state->phase>=PioneerQ38TokenNativeState::DispatchPreparing && state->phase<=PioneerQ38TokenNativeState::MemoryClosed;
 if(!c || &c->io!=this || !TokenCallScopeMatches() || sColdRecoveryRequired || sReloadQuiesced || !sReloadGeneration
  || !sBankStorageOnly || !c->bankStorageOnly || c->bytes!=PioneerResidentBanks::Total || runtimeCore>=120
  || (!cohort && (runtimeCore || fleetEnabled || c->q38Dispatch || c->q38DispatchRoutes || q38TokenMemoryMapAttempted))
  || !runtimeMapped || c->dispatchOwnerRetained || !c->initialized || !c->hashes
  || c->q38FfnModelPlan || c->q38GraphMutable || c->q38FfnZero || c->q38FfnGraph || c->q38FfnCode
  || c->q38GdnBanks || c->q38StateZero || c->q38Cohort || c->fleet || sQ38LocalDeltaOwner
  || c->q38FfnModelIdentity.session || c->q38FfnModelIdentity.sequence || c->q38FfnModelIdentity.modelEpoch || c->q38FfnModelIdentity.driverGeneration
  || c->q38GdnIdentity.session || c->q38GdnIdentity.sequence || c->q38GdnIdentity.modelEpoch || c->q38GdnIdentity.driverGeneration
  || c->q38Delta.saved || c->q38Delta.dirty || c->q38Delta.ready || c->q38Delta.running || c->q38Delta.awaiting
  || c->panelProbe.saved || c->panelProbe.dirty || c->panelProbe.ready || c->streamProbe.saved || c->streamProbe.dirty || c->streamProbe.ready
  || !c->q38Span || !c->q38Span->closed || c->q38Span->dirty || c->q38Span->ready || c->q38Span->running || c->q38Span->unsafe
  || !c->lease.saved || !c->lease.ready || c->lease.pending || c->lease.recovery_required
  || !c->bulk.saved || !c->bulk.ready || (c->bulk.dirty && (!state || state->phase<PioneerQ38TokenNativeState::RecurrentZeroing
   || state->phase>PioneerQ38TokenNativeState::ProjectionClosing)) || c->bulk.unsafe || c->host.state!=TT_HOST_PAGE_READY || c->host.released
  || PioneerHostPageIO::quarantined || !c->bulkHost.allocated || !c->bulkHost.ready || c->bulkHost.pending || c->bulkHost.quarantined
  || c->bulkHost.released || !c->wideHost.allocated || !c->wideHost.ready || !c->wideHost.memory_type_set || c->wideHost.pending
  || c->wideHost.quarantined || c->wideHost.released || wideQuarantined || c->Unsafe(false,false,true))return false;
 if(beginning){if(state || c->runtimeCoreMode!=-1 || !q38SpanAllowed)return false;}
 else if(!state || c->runtimeCoreMode!=1 || q38SpanAllowed
  || !state->MatchesBinding(*c,fd,c->hashes,c->initialized,fd->team,sReloadGeneration)
  || state->phase<PioneerQ38TokenNativeState::Claiming || state->phase>PioneerQ38TokenNativeState::MemoryClosed
  || (state->phase!=PioneerQ38TokenNativeState::Claiming && !state->MatchesReservation(c->banks,dram)))return false;
 if(cohort){
  const auto* d=c->q38Dispatch;
  if(!d || d!=state->participants || !state->memory || !state->startup || !fleetEnabled || d->worker[0].master!=&c->lease
   || d->unsafe || d->closed || d->bootAttempted || d->booted || d->rootRunning || d->prepared>120
   || !state->InitializationsClosed(c->banks,dram))return false;
  if(state->phase>=PioneerQ38TokenNativeState::MemoryPreparing
   && (!c->q38DispatchRoutes || d->prepared!=120 || !state->startup->MatchesPublishedBinding(state->canonical,state->bank,state->identity,&d->authority.source[0][0])))return false;
  if((state->phase==PioneerQ38TokenNativeState::RuntimePreparing || state->phase==PioneerQ38TokenNativeState::RuntimeReady)
   && (!state->runtime || state->runtime->Unsafe() || !state->memory->Installed()
    || (state->phase==PioneerQ38TokenNativeState::RuntimeReady && !state->runtime->Staged())))return false;
  if((state->phase==PioneerQ38TokenNativeState::WorkersBootstrapping || state->phase==PioneerQ38TokenNativeState::WorkersReady)
   && !state->ControlsCohort(d))return false;
  if(state->runtime && state->phase<PioneerQ38TokenNativeState::RuntimePreparing)return false;
  if(state->runtime && state->phase>=PioneerQ38TokenNativeState::MemoryClosing
   && (!state->runtime->Staged() || state->runtime->Pending()))return false;
 }
 const auto& clock=c->q38Clock.Snapshot();
 return c->q38Clock.RequiresExplicitEnd() && clock.state==TT_ARC_CLOCK_BUSY_VERIFIED && clock.busy_requested && clock.busy_acknowledged
  && !clock.restored && clock.busy_mhz && clock.busy_mhz<=1400;
} // real retained resources,NOT original-file authentication or initialized-state/launch authority
static uint64 sQ38TokenModelEpoch=0; // SAME global call lock;never wrap/reuse
[[maybe_unused]] static bool BindQ38TokenNative(PioneerStoreContext& c,unsigned bank){
 if(bank>=7 || sQ38TokenModelEpoch==UINT64_MAX || !c.io.TokenModelSourceScope(true))return false;
 const PioneerQ38GdnIdentity identity={sQ38TokenModelEpoch+1,1,1,sReloadGeneration};
 auto* state=new(std::nothrow) PioneerQ38TokenNativeState(c,sRuntimeOwner,c.hashes,c.initialized,sRuntimeOwner->team,bank,identity);
 if(!state)return false;
 // FULL851 coverage BEFORE epoch/reservation/model publication,without
 // inventing120 NoC source identities to build a startup template early.
 if(!state->canonical.Build() || !PioneerQ38TokenStartupPlan::CanonicalLearnedPagesPresent(state->canonical,c.initialized,PioneerResidentBanks::Pages)
  || !c.io.TokenModelSourceScope(true)){
  delete state;return false; // unpublished CPU heap only,no claims/device IO
 }
 sQ38TokenModelEpoch++;c.q38TokenNative=state;c.runtimeCoreMode=1;c.io.q38SpanAllowed=false;
 bool okay=state->graph.BindCanonical(c.banks,c.baseline.baseline.baseline.baseline.dram,identity,c.initialized,
  PioneerResidentBanks::Pages,state->canonical,bank);
 if(okay){state->phase=PioneerQ38TokenNativeState::Bound;okay=c.io.TokenModelSourceScope(false);}
 if(!okay){tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}
 return okay; // native atomic runtime reservations only;NO zero/code/upload/RUN
}
#endif
