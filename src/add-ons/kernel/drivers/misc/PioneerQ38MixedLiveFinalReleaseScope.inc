/* SPDX-License-Identifier: MIT */
#include "PioneerQ38MixedLiveFinalReleaseRole.h"
bool PioneerSessionIO::MixedLiveFinalReleaseScope(){
 auto* c=q38MutableContext;auto* m=c?c->q38EmbeddingModel:NULL;auto* state=c?c->q38MixedLiveNative:NULL;
 if(!c || &c->io!=this || !m || !state || !MixedLiveCallScopeMatches() || sColdRecoveryRequired || sReloadQuiesced || !sReloadGeneration
  || !sBankStorageOnly || !c->bankStorageOnly || c->bytes!=PioneerResidentBanks::Total || c->runtimeCoreMode!=1 || runtimeCore
  || c->dispatchOwnerRetained || c->q38EndPhase!=50 || c->Pci().restored || c->Pci().state!=TT_PIONEER_MMIO_RESTORE_FAILED
  || !c->transaction.map_attempted || c->transaction.config_attempted || c->Unsafe()
  || !state->Closed() || !state->MatchesBinding(*c,*m,sRuntimeOwner,c->hashes,sRuntimeOwner->team,c->initialized,sReloadGeneration)
  || !state->plan || !state->plan->Matches(m->plan) || !c->q38Dispatch || !c->q38Dispatch->closed || c->q38Dispatch->unsafe
  || c->q38Dispatch->rootRunning || c->q38Dispatch->worker[0].master!=&c->lease
  || !c->q38Span || !c->q38Span->closed || c->q38Span->unsafe || c->q38Span->dirty || c->q38Span->running
  || !c->lease.saved || !c->lease.controls_saved || c->lease.ready || c->lease.pending || c->lease.recovery_required
  || !c->lease.l1_restored || !c->lease.niu_restored || !c->lease.tlb_restored || !c->bulk.saved || c->bulk.ready || c->bulk.dirty || c->bulk.unsafe
  || c->host.state!=TT_HOST_PAGE_READY || c->host.released || PioneerHostPageIO::quarantined || PioneerHostPageIO::area<B_OK || outputArea<B_OK || wideArea<B_OK
  || !c->bulkHost.allocated || !c->bulkHost.ready || c->bulkHost.pending || c->bulkHost.quarantined || c->bulkHost.released
  || !c->wideHost.allocated || !c->wideHost.ready || !c->wideHost.memory_type_set || c->wideHost.pending || c->wideHost.quarantined
  || c->wideHost.released || wideQuarantined || !MixedLiveMemoryMappingStable(0))return false;
 const auto& clock=c->q38Clock.Snapshot();
 if(c->q38Clock.RequiresExplicitEnd() || c->q38Clock.Unsafe() || clock.version!=1 || clock.state!=TT_ARC_CLOCK_RESTORED
  || !clock.busy_requested || !clock.busy_acknowledged || !clock.idle_acknowledged || !clock.restored || clock.recovery_required
  || clock.before_mhz!=800 || clock.after_mhz!=clock.before_mhz || clock.busy_mhz<=800 || clock.busy_mhz>1400
  || clock.before_mode!=3 || clock.busy_mode!=3 || clock.after_mode!=3)return false;
 // These are retained baseline/closed-owner results,not a caller "ready"
 // boolean. No implicit TLB/NIU/L1/ARC rollback is allowed by final Unmap.
 return (!PioneerArcIO::dirty || arc.tlb_restored)
  && (!telemetryAttempted || telemetry.tlb_restored) && (!resetAttempted || tensix.tlb_restored)
  && (!dramAttempted || (dram.tlb_restored && dram.data_restored && !dram.recovery_required))
  && (!nocAttempted || (noc.tlb_restored && !noc.recovery_required))
  && (!pathAttempted || (path.tlb_restored && !path.recovery_required))
  && (!auditAttempted || (audit.tlb_restored && !audit.recovery_required))
  && (!readAttempted || (read.bme_restored && read.l1_restored && read.niu_restored && read.tlb_restored && !read.recovery_required));
} // SAME genuine CLOSED native source+FD+team+callthread+clock+ALL hosts,CPU checks ONLY
