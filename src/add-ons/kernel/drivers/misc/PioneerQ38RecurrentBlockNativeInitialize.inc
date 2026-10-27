/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT
#include "PioneerQ38RecurrentBlockNativeRole.h"
// SAME new private owner,AFTER actual embedding CLOSE. No old native flags,
// copied readiness,host-zero shortcut or public init/compute dispatcher.
bool PioneerSessionIO::RecurrentBlockInitializationScope(bool history,bool beginning){
 if(!RecurrentBlockBaseScope())return false;
 auto* c=q38MutableContext;auto* m=c->q38EmbeddingModel;auto* state=c->q38RecurrentBlockNative;
 if(!state || state->memory || state->plan || q38RecurrentBlockMemoryMapAttempted || runtimeCore || fleetEnabled
  || c->q38Dispatch || c->q38DispatchRoutes || (beginning && (state->phase!=PioneerQ38RecurrentBlockNativeState::Bound || c->bulk.dirty)))return false;
 bool closing=state->phase==(history?PioneerQ38RecurrentBlockNativeState::HistoryClosing:PioneerQ38RecurrentBlockNativeState::RecurrentClosing);
 if(!beginning && !closing && state->phase!=(history?PioneerQ38RecurrentBlockNativeState::HistoryZeroing:PioneerQ38RecurrentBlockNativeState::RecurrentZeroing))return false;
 auto* g=m->graph.Recurrent(m->identity);auto* h=m->graph.ConvHistory(m->identity);
 if(!g || !h || !g->MatchesInitializationOwner(c->banks,dram,m->identity)
  || !h->MatchesInitializationOwner(c->banks,dram,m->identity) || h->InitializationPublished(m->identity))return false;
 const auto& gz=state->recurrentZero;const auto& hz=state->historyZero;unsigned pages=0;
 if(history){
  if(!state->RecurrentInitialized(c->banks,dram) || hz.closed || hz.unsafe)return false;
  if(closing && (!hz.saved || !hz.ready || hz.running || hz.nonce!=336 || !h->InitializationProgress(m->identity,pages) || pages!=1680))return false;
  if(beginning || !hz.identity.Valid())return PioneerQ38ConvHistoryZeroPristine(hz) && !hz.closed
   && !h->OwnsInitializationLease(&hz) && h->InitializationProgress(m->identity,pages) && !pages;
  return hz.identity==m->identity && hz.owner==h && hz.banks==&c->banks && hz.proof==&dram && h->OwnsInitializationLease(&hz);
 }
 if(!PioneerQ38ConvHistoryZeroPristine(hz) || hz.closed || h->OwnsInitializationLease(&hz)
  || !h->InitializationProgress(m->identity,pages) || pages || gz.closed || gz.unsafe || g->InitializationPublished(m->identity))return false;
 if(closing && (!gz.saved || !gz.ready || gz.running || gz.nonce!=672 || !g->InitializationProgress(m->identity,pages) || pages!=73728))return false;
 if(beginning || !gz.identity.Valid())return !gz.saved && !gz.dirty && !gz.ready && !gz.running && !gz.unsafe && !gz.closed && !gz.nonce
  && gz.identity==PioneerQ38GdnIdentity{} && g->InitializationProgress(m->identity,pages) && !pages;
 return gz.identity==m->identity; // actual controller independently proves sources/code/coverage/holds
}
bool PioneerSessionIO::RecurrentBlockInitializationOtherCoresHeld(bool history){
 if(!RecurrentBlockInitializationScope(history,false) || !NonRootHeldMappingStable())return false;
 if(PioneerQ38NonRootHeld(*this,dram))return true;
 tt_resident_session_result retained={};RetainQ38DispatchOwner(*q38MutableContext,retained);return false;
}
bool PioneerSessionIO::ConvHistoryZeroBeforeCohort(){return RecurrentBlockInitializationScope(true,false);}
bool PioneerSessionIO::ConvHistoryZeroOtherCoresHeld(){return RecurrentBlockInitializationOtherCoresHeld(true);}
[[maybe_unused]] static __attribute__((noinline)) bool InitializeQ38RecurrentBlockState(PioneerStoreContext& c,PioneerQ38StateZeroResult& r){
 auto* state=c.q38RecurrentBlockNative;
 if(!state || !c.io.RecurrentBlockInitializationScope(false,true))return false;
 auto* m=c.q38EmbeddingModel;auto* g=m->graph.Recurrent(m->identity);unsigned regions=0,pages=0;
 if(!g || !g->InitializationRegions(m->identity,regions) || regions!=672 || !g->InitializationProgress(m->identity,pages) || pages)return false;
 state->phase=PioneerQ38RecurrentBlockNativeState::RecurrentZeroing;
 auto& dram=c.baseline.baseline.baseline.baseline.dram;auto& noc=c.baseline.baseline.baseline.noc;
 bool okay=PioneerQ38StateZeroPrepare(c.io,dram,noc,*g,c.banks,m->identity,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,state->recurrentZero);
 for(unsigned region=0;region<regions && okay;region++){
  unsigned count=0;
  okay=g->InitializationPages(m->identity,region,count) && (count==96 || count==112)
   && PioneerQ38StateZeroRun(c.io,dram,noc,*g,c.banks,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,state->recurrentZero,m->identity,region,0,count,r)
   && r.state==TT_RUNTIME_PASSED && r.pages==count && r.bytes==count*4096 && r.nonce==region+1
   && r.started && r.completed && r.held && !r.recovery_required;
 }
 if(okay)state->phase=PioneerQ38RecurrentBlockNativeState::RecurrentClosing;
 okay=okay && PioneerQ38StateZeroClose(c.io,dram,noc,*g,c.banks,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,state->recurrentZero,true)
  && state->RecurrentInitialized(c.banks,dram);
 if(okay)state->phase=PioneerQ38RecurrentBlockNativeState::Bound;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
}
[[maybe_unused]] static __attribute__((noinline)) bool InitializeQ38RecurrentBlockHistory(PioneerStoreContext& c,PioneerQ38ConvHistoryZeroResult& r){
 auto* state=c.q38RecurrentBlockNative;
 if(!state || !c.io.RecurrentBlockInitializationScope(true,true))return false;
 auto* m=c.q38EmbeddingModel;auto* h=m->graph.ConvHistory(m->identity);if(!h)return false;
 state->phase=PioneerQ38RecurrentBlockNativeState::HistoryZeroing;
 auto& dram=c.baseline.baseline.baseline.baseline.dram;auto& noc=c.baseline.baseline.baseline.noc;
 bool okay=PioneerQ38ConvHistoryZeroPrepare(c.io,dram,noc,*h,c.banks,m->identity,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,state->historyZero);
 for(unsigned region=0;region<336 && okay;region++){
  okay=PioneerQ38ConvHistoryZeroRun(c.io,dram,noc,*h,c.banks,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,state->historyZero,m->identity,region,0,5,r)
   && r.state==TT_RUNTIME_PASSED && r.pages==5 && r.bytes==20480 && r.nonce==region+1
   && r.started && r.completed && r.held && !r.recovery_required;
 }
 if(okay)state->phase=PioneerQ38RecurrentBlockNativeState::HistoryClosing;
 okay=okay && PioneerQ38ConvHistoryZeroClose(c.io,dram,noc,*h,c.banks,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,state->historyZero,true)
  && state->InitializationsClosed(c.banks,dram);
 if(okay)state->phase=PioneerQ38RecurrentBlockNativeState::Bound;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
} // Initial state only;NO token advance/model commit or QRB1 launch.
#endif
