/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
#include "PioneerQ38TokenNativeRole.h"
// The SAME full851 owner runs the real zero controllers. No completed old
// embedding/prefix, caller-ready flag, host-zero shortcut or public route.
// kind0 recurrent BOTH slots;kind1 Conv history;kind2 selected workspace.
bool PioneerSessionIO::TokenInitializationScope(unsigned kind,bool beginning){
 if(kind>=3 || !TokenModelSourceScope(false))return false;
 auto* c=q38MutableContext;auto* s=c->q38TokenNative;const auto& id=s->identity;
 if(s->memory || s->startup || runtimeCore || fleetEnabled || c->q38Dispatch || c->q38DispatchRoutes
  || (beginning && (s->phase!=PioneerQ38TokenNativeState::Bound || c->bulk.dirty)))return false;
 const auto zeroing=kind==0?PioneerQ38TokenNativeState::RecurrentZeroing:
  kind==1?PioneerQ38TokenNativeState::HistoryZeroing:PioneerQ38TokenNativeState::ProjectionZeroing;
 const auto closing=kind==0?PioneerQ38TokenNativeState::RecurrentClosing:
  kind==1?PioneerQ38TokenNativeState::HistoryClosing:PioneerQ38TokenNativeState::ProjectionClosing;
 if(!beginning && s->phase!=zeroing && s->phase!=closing)return false;
 auto* g=s->graph.Recurrent(id);auto* h=s->graph.ConvHistory(id);auto* p=s->graph.Projection(id);
 if(!g || !h || !p || !g->MatchesInitializationOwner(c->banks,dram,id)
  || !h->MatchesInitializationOwner(c->banks,dram,id) || !p->MatchesInitializationOwner(c->banks,dram,id))return false;
 const auto& gz=s->recurrentZero;const auto& hz=s->historyZero;const auto& pz=s->projectionZero;
 const bool historyPristine=PioneerQ38ConvHistoryZeroPristine(hz) && !hz.closed && !h->OwnsInitializationLease(&hz);
 const bool projectionPristine=PioneerQ38ProjectionZeroPristine(pz) && !pz.closed && !p->OwnsInitializationLease(&pz);
 unsigned pages=0;
 if(kind<2 && (!projectionPristine || !p->InitializationProgress(id,pages) || pages || p->InitializationPublished(id)))return false;
 if(kind==0){
  if(!historyPristine || !h->InitializationProgress(id,pages) || pages || h->InitializationPublished(id)
   || gz.closed || gz.unsafe || g->InitializationPublished(id))return false;
  if(s->phase==closing && (!gz.saved || !gz.ready || gz.running || gz.nonce!=672
   || !g->InitializationProgress(id,pages) || pages!=73728))return false;
  if(beginning || !gz.identity.Valid())return !gz.saved && !gz.dirty && !gz.ready && !gz.running && !gz.unsafe && !gz.closed
   && !gz.nonce && gz.identity==PioneerQ38GdnIdentity{} && g->InitializationProgress(id,pages) && !pages;
  return gz.identity==id; // actual controller separately proves sources/code/coverage/holds
 }
 if(!s->RecurrentInitialized(c->banks,dram))return false;
 if(kind==1){
  if(hz.closed || hz.unsafe || h->InitializationPublished(id))return false;
  if(s->phase==closing && (!hz.saved || !hz.ready || hz.running || hz.nonce!=336
   || !h->InitializationProgress(id,pages) || pages!=1680))return false;
  if(beginning || !hz.identity.Valid())return historyPristine && h->InitializationProgress(id,pages) && !pages;
  return hz.identity==id && hz.owner==h && hz.banks==&c->banks && hz.proof==&dram && h->OwnsInitializationLease(&hz);
 }
 if(!s->HistoryInitialized(c->banks,dram) || pz.closed || pz.unsafe || p->InitializationPublished(id))return false;
 if(s->phase==closing && (!pz.saved || !pz.ready || pz.running || pz.nonce!=2
  || !p->InitializationProgress(id,pages) || pages!=512))return false;
 if(beginning || !pz.identity.Valid())return projectionPristine && p->InitializationProgress(id,pages) && !pages;
 return pz.identity==id && pz.owner==p && pz.banks==&c->banks && pz.proof==&dram && p->OwnsInitializationLease(&pz);
}
bool PioneerSessionIO::TokenInitializationOtherCoresHeld(unsigned kind){
 if(!TokenInitializationScope(kind,false) || !NonRootHeldMappingStable())return false;
 if(PioneerQ38NonRootHeld(*this,dram))return true;
 tt_resident_session_result retained={};RetainQ38DispatchOwner(*q38MutableContext,retained);return false;
}
bool PioneerSessionIO::ConvHistoryZeroBeforeCohort(){return TokenInitializationScope(1,false);}
bool PioneerSessionIO::ConvHistoryZeroOtherCoresHeld(){return TokenInitializationOtherCoresHeld(1);}
bool PioneerSessionIO::ProjectionZeroBeforeCohort(){return TokenInitializationScope(2,false);}
bool PioneerSessionIO::ProjectionZeroOtherCoresHeld(){return TokenInitializationOtherCoresHeld(2);}

[[maybe_unused]] static __attribute__((noinline)) bool InitializeQ38TokenRecurrent(PioneerStoreContext& c,PioneerQ38StateZeroResult& r){
 auto* s=c.q38TokenNative;if(!s || !c.io.TokenInitializationScope(0,true))return false;
 const auto& id=s->identity;auto* g=s->graph.Recurrent(id);unsigned regions=0,pages=0;
 if(!g || !g->InitializationRegions(id,regions) || regions!=672 || !g->InitializationProgress(id,pages) || pages)return false;
 s->phase=PioneerQ38TokenNativeState::RecurrentZeroing;
 auto& dram=c.baseline.baseline.baseline.baseline.dram;auto& noc=c.baseline.baseline.baseline.noc;
 bool okay=PioneerQ38StateZeroPrepare(c.io,dram,noc,*g,c.banks,id,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,s->recurrentZero);
 for(unsigned region=0;region<regions && okay;region++){
  unsigned count=0;
  okay=g->InitializationPages(id,region,count) && (count==96 || count==112)
   && PioneerQ38StateZeroRun(c.io,dram,noc,*g,c.banks,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,s->recurrentZero,id,region,0,count,r)
   && r.state==TT_RUNTIME_PASSED && r.pages==count && r.bytes==count*4096 && r.nonce==region+1
   && r.started && r.completed && r.held && !r.recovery_required;
 }
 if(okay)s->phase=PioneerQ38TokenNativeState::RecurrentClosing;
 okay=okay && PioneerQ38StateZeroClose(c.io,dram,noc,*g,c.banks,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,s->recurrentZero,true)
  && s->RecurrentInitialized(c.banks,dram);
 if(okay)s->phase=PioneerQ38TokenNativeState::Bound;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
}
[[maybe_unused]] static __attribute__((noinline)) bool InitializeQ38TokenHistory(PioneerStoreContext& c,PioneerQ38ConvHistoryZeroResult& r){
 auto* s=c.q38TokenNative;if(!s || !c.io.TokenInitializationScope(1,true))return false;
 const auto& id=s->identity;auto* h=s->graph.ConvHistory(id);if(!h)return false;
 s->phase=PioneerQ38TokenNativeState::HistoryZeroing;
 auto& dram=c.baseline.baseline.baseline.baseline.dram;auto& noc=c.baseline.baseline.baseline.noc;
 bool okay=PioneerQ38ConvHistoryZeroPrepare(c.io,dram,noc,*h,c.banks,id,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,s->historyZero);
 for(unsigned region=0;region<336 && okay;region++){
  okay=PioneerQ38ConvHistoryZeroRun(c.io,dram,noc,*h,c.banks,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,s->historyZero,id,region,0,5,r)
   && r.state==TT_RUNTIME_PASSED && r.pages==5 && r.bytes==20480 && r.nonce==region+1
   && r.started && r.completed && r.held && !r.recovery_required;
 }
 if(okay)s->phase=PioneerQ38TokenNativeState::HistoryClosing;
 okay=okay && PioneerQ38ConvHistoryZeroClose(c.io,dram,noc,*h,c.banks,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,s->historyZero,true)
  && s->HistoryInitialized(c.banks,dram);
 if(okay)s->phase=PioneerQ38TokenNativeState::Bound;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
}
[[maybe_unused]] static __attribute__((noinline)) bool InitializeQ38TokenProjection(PioneerStoreContext& c,PioneerQ38ProjectionZeroResult& r){
 auto* s=c.q38TokenNative;if(!s || !c.io.TokenInitializationScope(2,true))return false;
 const auto& id=s->identity;auto* p=s->graph.Projection(id);if(!p)return false;
 s->phase=PioneerQ38TokenNativeState::ProjectionZeroing;
 auto& dram=c.baseline.baseline.baseline.baseline.dram;auto& noc=c.baseline.baseline.baseline.noc;
 bool okay=PioneerQ38ProjectionZeroPrepare(c.io,dram,noc,*p,c.banks,id,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,s->projectionZero);
 for(unsigned run=0;run<2 && okay;run++){
  okay=PioneerQ38ProjectionZeroRun(c.io,dram,noc,*p,c.banks,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,s->projectionZero,id,0,run*256,256,r)
   && r.state==TT_RUNTIME_PASSED && r.pages==256 && r.bytes==1048576 && r.nonce==run+1
   && r.started && r.completed && r.held && !r.recovery_required;
 }
 if(okay)s->phase=PioneerQ38TokenNativeState::ProjectionClosing;
 okay=okay && PioneerQ38ProjectionZeroClose(c.io,dram,noc,*p,c.banks,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,s->projectionZero,true)
  && s->InitializationsClosed(c.banks,dram);
 if(okay)s->phase=PioneerQ38TokenNativeState::Initialized;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
} // private initialization ONLY;KV/metadata/input producers,token execution and END remain owed
#endif
