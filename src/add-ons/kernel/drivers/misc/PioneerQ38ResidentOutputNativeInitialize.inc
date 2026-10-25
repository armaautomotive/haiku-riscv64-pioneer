/* SPDX-License-Identifier: MIT */
// SAME retained new native context/leases and actual zero controllers.
// No native role/compute/Work selector is enabled by these initializers.
bool PioneerSessionIO::ResidentOutputInitializationSourceScope(bool history,bool beginning)
{
 if(!ResidentOutputInitializationBaseScope())return false;
 auto* c=q38MutableContext;auto* m=c->q38EmbeddingModel;auto* state=c->q38ResidentOutputNative;
 bool closing=state && state->phase==(history?PioneerQ38ResidentOutputNativeState::HistoryClosing:PioneerQ38ResidentOutputNativeState::RecurrentClosing);
 if(!state || !state->MatchesBinding(*c,*m,sRuntimeOwner,c->hashes,sRuntimeOwner->team,c->initialized,sReloadGeneration)
  || state->memory || (beginning && (state->phase!=PioneerQ38ResidentOutputNativeState::Bound || c->bulk.dirty))
  || (!beginning && !closing && state->phase!=(history?PioneerQ38ResidentOutputNativeState::HistoryZeroing:PioneerQ38ResidentOutputNativeState::RecurrentZeroing)))return false;
 const auto& g=state->recurrentZero;const auto& h=state->historyZero;
 auto* recurrent=m->graph.Recurrent(m->identity);auto* prior=m->graph.ConvHistory(m->identity);
 if(!recurrent || !prior || prior->InitializationPublished(m->identity))return false;
 auto pristineGdn=[&](){return !g.saved && !g.dirty && !g.ready && !g.running && !g.unsafe && !g.closed && !g.nonce
  && g.identity==PioneerQ38GdnIdentity{};};
 unsigned pages=0;
 if(history){
  if(!state->RecurrentInitialized(c->banks,dram) || h.closed || h.unsafe)return false;
  if(closing && (!h.saved || !h.ready || h.running || h.nonce!=336 || !prior->InitializationProgress(m->identity,pages) || pages!=1680))return false;
  if(beginning || !h.identity.Valid()){
   return PioneerQ38ConvHistoryZeroPristine(h) && !h.closed && !prior->OwnsInitializationLease(&h)
    && prior->InitializationProgress(m->identity,pages) && !pages;
  }
  return h.identity==m->identity && h.owner==prior && h.banks==&c->banks && h.proof==&dram && prior->OwnsInitializationLease(&h);
 }
 if(!PioneerQ38ConvHistoryZeroPristine(h) || h.closed || prior->OwnsInitializationLease(&h)
  || !prior->InitializationProgress(m->identity,pages) || pages || g.closed || g.unsafe || recurrent->InitializationPublished(m->identity))return false;
 if(closing && (!g.saved || !g.ready || g.running || g.nonce!=672 || !recurrent->InitializationProgress(m->identity,pages) || pages!=73728))return false;
 if(beginning || !g.identity.Valid())return pristineGdn() && recurrent->InitializationProgress(m->identity,pages) && !pages;
 return g.identity==m->identity; // actual zero controller independently proves ALL physical sources/code/coverage/holds
}
bool PioneerSessionIO::ResidentOutputInitializationOtherCoresHeld(bool history)
{
 if(!ResidentOutputInitializationSourceScope(history,false) || !NonRootHeldMappingStable())return false;
 if(PioneerQ38NonRootHeld(*this,dram))return true;
 tt_resident_session_result retained={};RetainQ38DispatchOwner(*q38MutableContext,retained);return false;
} // first physical mismatch retains ALL;no root-view restoration after uncertainty
bool PioneerSessionIO::ConvHistoryZeroBeforeCohort(){return ResidentOutputInitializationSourceScope(true,false);}
bool PioneerSessionIO::ConvHistoryZeroOtherCoresHeld(){return ResidentOutputInitializationOtherCoresHeld(true);}
bool PioneerSessionIO::ResidentOutputPrefixInitializationScope()
{
 auto* c=q38MutableContext;auto* m=c?c->q38EmbeddingModel:NULL;auto* state=c?c->q38ResidentOutputNative:NULL;
 return c && &c->io==this && m && state && ResidentOutputCallScopeMatches()
  && state->phase==PioneerQ38ResidentOutputNativeState::Bound && !state->memory
  && state->MatchesBinding(*c,*m,sRuntimeOwner,c->hashes,sRuntimeOwner->team,c->initialized,sReloadGeneration)
  && state->InitializationsClosed(c->banks,dram);
} // necessary initialized binding ONLY;complete original prefix source/clock/host/physical checks still mandatory
[[maybe_unused]] static bool InitializeQ38ResidentOutputRecurrent(PioneerStoreContext& c,PioneerQ38StateZeroResult& r)
{
 auto* state=c.q38ResidentOutputNative;
 if(!state || !c.io.ResidentOutputInitializationSourceScope(false,true))return false; // inert BEFORE claims/phase/IO
 auto* m=c.q38EmbeddingModel;auto* g=m->graph.Recurrent(m->identity);
 unsigned regions=0,pages=0;
 if(!g || !g->InitializationRegions(m->identity,regions) || regions!=672 || !g->InitializationProgress(m->identity,pages) || pages)return false;
 state->phase=PioneerQ38ResidentOutputNativeState::RecurrentZeroing;
 auto& dram=c.baseline.baseline.baseline.baseline.dram;auto& noc=c.baseline.baseline.baseline.noc;
 bool okay=PioneerQ38StateZeroPrepare(c.io,dram,noc,*g,c.banks,m->identity,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,state->recurrentZero);
 for(unsigned region=0;region<regions && okay;region++){
  unsigned count=0;
  okay=g->InitializationPages(m->identity,region,count) && (count==96 || count==112)
   && PioneerQ38StateZeroRun(c.io,dram,noc,*g,c.banks,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,state->recurrentZero,m->identity,region,0,count,r)
   && r.state==TT_RUNTIME_PASSED && r.pages==count && r.bytes==count*4096 && r.nonce==region+1
   && r.started && r.completed && r.held && !r.recovery_required;
 }
 if(okay)state->phase=PioneerQ38ResidentOutputNativeState::RecurrentClosing;
 okay=okay && PioneerQ38StateZeroClose(c.io,dram,noc,*g,c.banks,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,state->recurrentZero,true)
  && state->RecurrentInitialized(c.banks,dram);
 if(okay)state->phase=PioneerQ38ResidentOutputNativeState::Bound;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}
 return okay; // actual BOTH slots73728pages and clean CLOSE,not current-state/model compute readiness
}
[[maybe_unused]] static bool InitializeQ38ResidentOutputHistory(PioneerStoreContext& c,PioneerQ38ConvHistoryZeroResult& r)
{
 auto* state=c.q38ResidentOutputNative;
 if(!state || !c.io.ResidentOutputInitializationSourceScope(true,true))return false;
 auto* m=c.q38EmbeddingModel;auto* h=m->graph.ConvHistory(m->identity);if(!h)return false;
 state->phase=PioneerQ38ResidentOutputNativeState::HistoryZeroing;
 auto& dram=c.baseline.baseline.baseline.baseline.dram;auto& noc=c.baseline.baseline.baseline.noc;
 bool okay=PioneerQ38ConvHistoryZeroPrepare(c.io,dram,noc,*h,c.banks,m->identity,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,state->historyZero);
 for(unsigned region=0;region<336 && okay;region++){
  okay=PioneerQ38ConvHistoryZeroRun(c.io,dram,noc,*h,c.banks,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,state->historyZero,m->identity,region,0,5,r)
   && r.state==TT_RUNTIME_PASSED && r.pages==5 && r.bytes==20480 && r.nonce==region+1
   && r.started && r.completed && r.held && !r.recovery_required;
 }
 if(okay)state->phase=PioneerQ38ResidentOutputNativeState::HistoryClosing;
 okay=okay && PioneerQ38ConvHistoryZeroClose(c.io,dram,noc,*h,c.banks,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,state->historyZero,true)
  && state->InitializationsClosed(c.banks,dram);
 if(okay)state->phase=PioneerQ38ResidentOutputNativeState::Bound;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}
 return okay; // all336x5 observed zero pages/CLOSE,never copies or replays old decoder readiness
}
