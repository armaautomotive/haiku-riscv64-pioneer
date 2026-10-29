/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
#include "PioneerQ38TokenNativeRole.h"
bool PioneerSessionIO::TokenRuntimeScope(const PioneerQ38TokenNativeOwner& runtime,const PioneerQ38TokenMemory& memory,
 const PioneerQ38CanonicalModelPlan& canonical,const PioneerQ38TokenStartupPlan& startup,const PioneerQ38GdnIdentity& id,
 const uint8_t* learned,unsigned pages){
 auto* c=q38MutableContext;auto* s=c?c->q38TokenNative:NULL;
 return s && s->runtime==&runtime && s->memory==&memory && &s->canonical==&canonical && s->startup==&startup
  && s->identity==id && c->initialized==learned && pages==PioneerResidentBanks::Pages
  && (s->phase==PioneerQ38TokenNativeState::RuntimePreparing || s->phase==PioneerQ38TokenNativeState::RuntimeReady
   || s->phase==PioneerQ38TokenNativeState::WorkersBootstrapping || s->phase==PioneerQ38TokenNativeState::WorkersReady
   || s->phase==PioneerQ38TokenNativeState::RootStarting || s->phase==PioneerQ38TokenNativeState::RootRunning
   || s->phase==PioneerQ38TokenNativeState::RootHolding || s->phase==PioneerQ38TokenNativeState::RootHeld
   || s->phase==PioneerQ38TokenNativeState::RootRestoring || s->phase==PioneerQ38TokenNativeState::RootRestored
   || s->phase==PioneerQ38TokenNativeState::DispatchRestoring || s->phase==PioneerQ38TokenNativeState::DispatchRestored
   || s->phase==PioneerQ38TokenNativeState::PeerStoresRestoring || s->phase==PioneerQ38TokenNativeState::PeerStoresRestored
   || s->phase==PioneerQ38TokenNativeState::RootStoreRestoring || s->phase==PioneerQ38TokenNativeState::RootStoreRestored
   || s->phase==PioneerQ38TokenNativeState::SharedTlbRestoring || s->phase==PioneerQ38TokenNativeState::SharedTlbRestored
   || s->phase==PioneerQ38TokenNativeState::ClockEnding || s->phase==PioneerQ38TokenNativeState::ClockEnded)
  && TokenModelSourceScope(false)
  && runtime.MatchesRuntimeBinding(dram,c->banks,*c->q38Dispatch,memory,canonical,startup,id,learned,pages);
}
[[maybe_unused]] static __attribute__((noinline)) bool PrepareQ38TokenNativeRuntime(PioneerStoreContext& c,PioneerQ38TokenRuntimeResult& result){
 auto* s=c.q38TokenNative;const PioneerQ38TokenRuntimeResult pristine;
 const auto* a=reinterpret_cast<const uint8_t*>(&result);const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
 for(unsigned i=0;i<sizeof(result);i++)if(a[i]!=b[i])return false;
 if(!s || s->phase!=PioneerQ38TokenNativeState::MemoryInstalled || s->runtime || !s->memory || !s->memory->Installed()
  || !s->startup || !s->participants || s->participants!=c.q38Dispatch || !c.io.TokenModelSourceScope(false))return false;
 const uintptr_t out=reinterpret_cast<uintptr_t>(&result);
 if(out>UINTPTR_MAX-sizeof(result) || !s->startup->PublishedOutput(&result,sizeof(result)))return false;
 const void* objects[]={&c,&c.io,s,s->memory,s->participants,c.initialized,c.hashes,PioneerQ38TokenRope::Data,c.io.BulkSavedWords()};
 const size_t sizes[]={sizeof(c),sizeof(c.io),sizeof(*s),sizeof(*s->memory),sizeof(*s->participants),
  PioneerResidentBanks::Pages,size_t(PioneerResidentBanks::Pages)*sizeof(uint32_t),PioneerQ38TokenRope::Bytes,size_t(TT_BULK_UPLOAD_SAVED_WORDS)*4};
 for(unsigned i=0;i<9;i++){
  const uintptr_t owned=reinterpret_cast<uintptr_t>(objects[i]);
  if(!owned || owned>UINTPTR_MAX-sizes[i] || (out<owned+sizes[i] && owned<out+sizeof(result)))return false;
 }
 auto* runtime=new(std::nothrow) PioneerQ38TokenNativeOwner;if(!runtime)return false;
 if(!c.io.TokenModelSourceScope(false)){delete runtime;return false;} // unpublished CPU child ONLY
 s->runtime=runtime;s->phase=PioneerQ38TokenNativeState::RuntimePreparing;
 bool okay=runtime->PrepareHeldRuntime(c.io,c.baseline.baseline.baseline.baseline.dram,c.banks,*c.q38Dispatch,
  *s->memory,s->canonical,*s->startup,s->identity,c.initialized,PioneerResidentBanks::Pages,result);
 if(okay)s->phase=PioneerQ38TokenNativeState::RuntimeReady;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);result.unsafe=1;result.staged=0;}
 return okay;
} // startup DATA,empty KV frontier ONLY;NO bootstrap/RUN/STOP/END/Work capability
[[maybe_unused]] static __attribute__((noinline)) bool BootstrapQ38TokenNativeWorkers(PioneerStoreContext& c,PioneerQ38TokenBootstrapResult& result){
 auto* s=c.q38TokenNative;const PioneerQ38TokenBootstrapResult pristine;
 const auto* a=reinterpret_cast<const uint8_t*>(&result);const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
 for(unsigned i=0;i<sizeof(result);i++)if(a[i]!=b[i])return false;
 if(!s || s->phase!=PioneerQ38TokenNativeState::RuntimeReady || !s->runtime || !s->memory || !s->memory->Installed()
  || !s->runtime->Staged() || s->runtime->Pending() || !s->startup || !s->participants
  || s->participants!=c.q38Dispatch || !c.io.TokenModelSourceScope(false))return false;
 const uintptr_t out=reinterpret_cast<uintptr_t>(&result);
 if(out>UINTPTR_MAX-sizeof(result) || !s->startup->PublishedOutput(&result,sizeof(result)))return false;
 const void* objects[]={&c,&c.io,s,s->memory,s->participants,s->runtime,c.initialized,c.hashes,PioneerQ38TokenRope::Data};
 const size_t sizes[]={sizeof(c),sizeof(c.io),sizeof(*s),sizeof(*s->memory),sizeof(*s->participants),sizeof(*s->runtime),
  PioneerResidentBanks::Pages,size_t(PioneerResidentBanks::Pages)*sizeof(uint32_t),PioneerQ38TokenRope::Bytes};
 for(unsigned i=0;i<9;i++){
  const uintptr_t owned=reinterpret_cast<uintptr_t>(objects[i]);
  if(!owned || owned>UINTPTR_MAX-sizes[i] || (out<owned+sizes[i] && owned<out+sizeof(result)))return false;
 }
 if(!s->runtime->DispatchOutput(&result,sizeof(result)))return false;
 s->phase=PioneerQ38TokenNativeState::WorkersBootstrapping;
 bool okay=s->runtime->BootstrapWorkers(c.io,result);
 if(okay && s->runtime->WorkersReady())s->phase=PioneerQ38TokenNativeState::WorkersReady;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);result.unsafe=1;result.ready=0;okay=false;}
 return okay;
} // distinct fresh worker startup ONLY;NO ROOT/model RUN,cancel,STOP,END or Work route
#endif
