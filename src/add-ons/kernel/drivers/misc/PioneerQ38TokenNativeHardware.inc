/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
#include "PioneerQ38TokenNativeRole.h"
__attribute__((noinline)) bool PioneerSessionIO::TokenFinalOwnershipScope(){
 auto* c=q38MutableContext;auto* s=c?c->q38TokenNative:NULL;auto* fd=sRuntimeOwner;
 if(!c || &c->io!=this || !s || s->phase!=PioneerQ38TokenNativeState::HardwareSealed || !s->HardwareLeasesSealed()
  || !fd || !TokenCallScopeMatches() || sColdRecoveryRequired || sReloadQuiesced || !sReloadGeneration
  || !sBankStorageOnly || !c->bankStorageOnly || c->bytes!=PioneerResidentBanks::Total || c->runtimeCoreMode!=1 || runtimeCore
  || c->dispatchOwnerRetained || !runtimeMapped || !fleetEnabled || !q38TokenMemoryMapAttempted || profile.bar0_host_start!=q38TokenMemoryBar
  || !runtimeEntry || !runtimePc || !scratchData || payloadBytes!=8192 || !c->hashes || !c->initialized || q38SpanAllowed
  || !s->MatchesBinding(*c,fd,c->hashes,c->initialized,fd->team,sReloadGeneration)
  || !s->MatchesReservation(c->banks,dram) || !s->InitializationsClosed(c->banks,dram)
  || !s->runtime || !s->memory || !s->startup || !s->participants || s->participants!=c->q38Dispatch
  || !s->runtime->MatchesRuntimeBinding(dram,c->banks,*s->participants,*s->memory,s->canonical,*s->startup,s->identity,c->initialized,PioneerResidentBanks::Pages)
  || s->participants->worker[0].master!=&c->lease || c->lease.ready || c->bulk.ready || !c->bulk.saved || c->bulk.dirty || c->bulk.unsafe
  || c->host.state!=TT_HOST_PAGE_READY || c->host.released || PioneerHostPageIO::quarantined
  || !c->bulkHost.allocated || !c->bulkHost.ready || !c->bulkHost.memory_type_set || c->bulkHost.pending || c->bulkHost.quarantined || c->bulkHost.released
  || !c->wideHost.allocated || !c->wideHost.ready || !c->wideHost.memory_type_set || c->wideHost.pending || c->wideHost.quarantined || c->wideHost.released || wideQuarantined
  || !c->q38Span || !c->q38Span->closed || c->q38Span->dirty || c->q38Span->ready || c->q38Span->running || c->q38Span->unsafe
  || c->q38FfnModelPlan || c->q38GraphMutable || c->q38FfnZero || c->q38FfnGraph || c->q38FfnCode
  || c->q38GdnBanks || c->q38StateZero || c->q38Cohort || c->fleet || sQ38LocalDeltaOwner
  || c->q38FfnModelIdentity.session || c->q38FfnModelIdentity.sequence || c->q38FfnModelIdentity.modelEpoch || c->q38FfnModelIdentity.driverGeneration
  || c->q38GdnIdentity.session || c->q38GdnIdentity.sequence || c->q38GdnIdentity.modelEpoch || c->q38GdnIdentity.driverGeneration
  || c->q38Delta.saved || c->q38Delta.dirty || c->q38Delta.ready
  || c->q38Delta.running || c->q38Delta.awaiting || c->panelProbe.saved || c->panelProbe.dirty || c->panelProbe.ready
  || c->streamProbe.saved || c->streamProbe.dirty || c->streamProbe.ready || c->Unsafe(false,false,true))return false;
 for(unsigned i=0;i<4;i++)if(q38TokenMemoryArea[i]<B_OK || !q38TokenMemoryView[i])return false;
 const auto& clock=c->q38Clock.Snapshot();
 return s->clockEndAttempted && s->clockEnded && !c->q38Clock.Unsafe() && !c->q38Clock.RequiresExplicitEnd()
  && clock.version==1 && clock.state==TT_ARC_CLOCK_RESTORED && clock.restored && clock.idle_acknowledged && !clock.recovery_required
  && clock.busy_requested && clock.busy_acknowledged && clock.before_mhz==800 && clock.after_mhz==800 && clock.busy_mhz>800 && clock.busy_mhz<=1400
  && clock.before_mode==3 && clock.busy_mode==3 && clock.after_mode==3 && c->q38ClockIO.TokenClockViewsReleased()
  && c->q38ClockIO.tokenClockGuard.Matches(c,Q38TokenNativeClockScope) && c->q38ClockIO.tokenClockGuard.Sealed() && !c->q38ClockIO.tokenClockGuard.Stopped();
} // CPU ONLY:retained final owner,not VM/PCI/host/reservation release or END authority
[[maybe_unused]] static __attribute__((noinline)) bool SealQ38TokenNativeHardware(PioneerStoreContext& c,PioneerQ38TokenHardwareSealResult& result){
 auto* s=c.q38TokenNative;const PioneerQ38TokenHardwareSealResult pristine;
 const auto* a=reinterpret_cast<const uint8_t*>(&result);const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
 for(unsigned i=0;i<sizeof(result);i++)if(a[i]!=b[i])return false;
 if(!s || s->phase!=PioneerQ38TokenNativeState::ClockEnded || s->hardwareSealAttempted || s->hardwareSealed
  || !s->ClockRestored() || !s->runtime || !Q38TokenExecutionOutput(c,&result,sizeof(result)) || !c.io.TokenModelSourceScope(false)
  || !c.q38ClockIO.tokenClockGuard.Matches(&c,Q38TokenNativeClockScope) || !c.q38ClockIO.tokenClockGuard.Sealed()
  || c.q38ClockIO.tokenClockGuard.Stopped() || !c.q38ClockIO.TokenClockViewsReleased() || c.bulk.dirty || !c.bulk.ready)return false;
 s->hardwareSealAttempted=true;s->phase=PioneerQ38TokenNativeState::HardwareSealing;
 bool okay=s->runtime->SealHardwareLeases(c.io,result);
 if(okay && s->runtime->HardwareLeasesSealed()){
  c.bulk.ready=false;s->hardwareSealed=true;s->phase=PioneerQ38TokenNativeState::HardwareSealed;
  okay=c.io.TokenFinalOwnershipScope();
 }
 if(!okay){tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);result.unsafe=1;result.ready=0;}
 return okay;
} // hardware journals/leases ONLY;launched history retained,full native Closed remains FALSE
#endif
