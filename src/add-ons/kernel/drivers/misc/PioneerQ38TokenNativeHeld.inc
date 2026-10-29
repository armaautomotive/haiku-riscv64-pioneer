/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
#include "PioneerQ38TokenNativeRole.h"
#include "PioneerQ38TokenNativePrepare.inc"
bool PioneerSessionIO::TokenMemoryMappingSourceScope(){
 auto* c=q38MutableContext;auto* s=c?c->q38TokenNative:NULL;
 return s && !runtimeCore && s->phase==PioneerQ38TokenNativeState::MemoryPreparing && s->memory && s->startup
  && !s->memory->Installed() && !s->memory->Closed() && TokenModelSourceScope(false);
}
bool PioneerSessionIO::TokenMemoryAccessScope(){
 auto* c=q38MutableContext;auto* s=c?c->q38TokenNative:NULL;auto* fd=sRuntimeOwner;
 // Per-WORD metadata under the SAME call lock. Full physical/native guards
 // remain mandatory before+after EVERY participant;no public ready hint.
 return c && &c->io==this && s && fd && fd->magic==0x50313030 && fd->context==c && c->q38GdnCallThread>=0
  && c->q38GdnCallThread==find_thread(NULL) && !c->dispatchOwnerRetained && !sColdRecoveryRequired && !sReloadQuiesced
  && c->bytes==PioneerResidentBanks::Total && c->runtimeCoreMode==1 && s->memory && s->startup
  && s->participants && s->participants==c->q38Dispatch && !s->memory->Unsafe() && !s->memory->Closed()
  && (s->phase==PioneerQ38TokenNativeState::MemoryPreparing || s->phase==PioneerQ38TokenNativeState::MemoryInstalled
   || s->phase==PioneerQ38TokenNativeState::RuntimePreparing || s->phase==PioneerQ38TokenNativeState::RuntimeReady
   || ((s->phase==PioneerQ38TokenNativeState::WorkersBootstrapping || s->phase==PioneerQ38TokenNativeState::WorkersReady
    || s->phase==PioneerQ38TokenNativeState::RootStarting || s->phase==PioneerQ38TokenNativeState::RootRunning
    || s->phase==PioneerQ38TokenNativeState::RootHolding || s->phase==PioneerQ38TokenNativeState::RootHeld
    || s->phase==PioneerQ38TokenNativeState::RootRestoring || s->phase==PioneerQ38TokenNativeState::RootRestored
    || s->phase==PioneerQ38TokenNativeState::DispatchRestoring || s->phase==PioneerQ38TokenNativeState::DispatchRestored
    || s->phase==PioneerQ38TokenNativeState::PeerStoresRestoring || s->phase==PioneerQ38TokenNativeState::PeerStoresRestored
    || s->phase==PioneerQ38TokenNativeState::RootStoreRestoring || s->phase==PioneerQ38TokenNativeState::RootStoreRestored)
    && s->runtime && !s->runtime->Unsafe())
   || s->phase==PioneerQ38TokenNativeState::MemoryClosing)
  && s->MatchesBinding(*c,fd,c->hashes,c->initialized,fd->team,sReloadGeneration);
}
bool PioneerSessionIO::TokenDispatchImageAccessScope(unsigned worker){
 auto* c=q38MutableContext;auto* s=c?c->q38TokenNative:NULL;
 return worker<120 && worker==runtimeCore && s && s->phase==PioneerQ38TokenNativeState::DispatchRestoring
  && TokenMemoryAccessScope();
} // exact native dispatch-restoration call only,not a broader L1 address grant
bool PioneerSessionIO::TokenPeerStoreSelectionScope(){
 auto* c=q38MutableContext;auto* s=c?c->q38TokenNative:NULL;
 return s && (s->phase==PioneerQ38TokenNativeState::PeerStoresRestoring || s->phase==PioneerQ38TokenNativeState::PeerStoresRestored
   || s->phase==PioneerQ38TokenNativeState::RootStoreRestoring || s->phase==PioneerQ38TokenNativeState::RootStoreRestored)
  && TokenMemoryAccessScope() && TokenModelSourceScope(false);
} // CPU ownership checks ONLY;selection may be between exact known TLB writes
bool PioneerSessionIO::TokenPeerStoreAccessScope(unsigned worker){
 auto* c=q38MutableContext;auto* s=c?c->q38TokenNative:NULL;
 return worker && worker<120 && worker==runtimeCore && s && s->phase==PioneerQ38TokenNativeState::PeerStoresRestoring
  && TokenPeerStoreSelectionScope();
} // never ROOT or a public arbitrary NIU/scratch operation
bool PioneerSessionIO::TokenRootStoreAccessScope(){
 auto* c=q38MutableContext;auto* s=c?c->q38TokenNative:NULL;
 return !runtimeCore && s && s->phase==PioneerQ38TokenNativeState::RootStoreRestoring
  && TokenPeerStoreSelectionScope();
} // distinct ROOT-only local restoration;not master/shared-resource close
bool PioneerSessionIO::TokenMemoryScope(const PioneerQ38TokenMemory& memory,const PioneerQ38CanonicalModelPlan& canonical,
 const PioneerQ38TokenStartupPlan& startup,const PioneerQ38GdnIdentity& id,const uint8_t* learned,unsigned pages){
 auto* c=q38MutableContext;auto* s=c?c->q38TokenNative:NULL;
 return s && s->memory==&memory && s->startup==&startup && &canonical==&s->canonical && id==s->identity
  && learned==c->initialized && pages==PioneerResidentBanks::Pages && TokenMemoryAccessScope() && TokenModelSourceScope(false);
}
void PioneerSessionIO::TokenMemoryRetainHostOwners(){
 if(q38MutableContext){tt_resident_session_result retained={};RetainQ38DispatchOwner(*q38MutableContext,retained);}
}
[[maybe_unused]] static __attribute__((noinline)) bool PrepareQ38TokenNativeHeld(PioneerStoreContext& c,const uint32_t* prompt,
 unsigned prompts,unsigned limit,bool stopEos,unsigned eos0,unsigned eos1,PioneerQ38TokenMemoryResult& result){
 auto* s=c.q38TokenNative;const PioneerQ38TokenMemoryResult pristine;
 static_assert(sizeof(result)==48,"all result bytes have an explicit pristine value");
 const auto* a=reinterpret_cast<const uint8_t*>(&result);const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
 for(unsigned i=0;i<sizeof(result);i++)if(a[i]!=b[i])return false;
 if(!s || s->phase!=PioneerQ38TokenNativeState::Initialized || s->memory || s->startup || s->participants || c.q38Dispatch
  || c.io.q38TokenMemoryMapAttempted || !c.io.TokenModelSourceScope(false)
  || !s->InitializationsClosed(c.banks,c.baseline.baseline.baseline.baseline.dram)
  || !PioneerQ38TokenStartupPlan::CanonicalLearnedPagesPresent(s->canonical,c.initialized,PioneerResidentBanks::Pages)
  || !prompt || !prompts || prompts>limit || !limit || limit>8192 || eos0>=248320 || eos1>=248320)return false;
 const uintptr_t input=reinterpret_cast<uintptr_t>(prompt),out=reinterpret_cast<uintptr_t>(&result);
 const uintptr_t context=reinterpret_cast<uintptr_t>(&c),state=reinterpret_cast<uintptr_t>(s);
 const unsigned inputBytes=prompts*4;
 if(input>UINTPTR_MAX-inputBytes || out>UINTPTR_MAX-sizeof(result)
  || (input<context+sizeof(c) && context<input+inputBytes) || (input<state+sizeof(*s) && state<input+inputBytes)
  || (out<context+sizeof(c) && context<out+sizeof(result)) || (out<state+sizeof(*s) && state<out+sizeof(result))
  || (input<out+sizeof(result) && out<input+inputBytes))return false; // trusted kernel buffers,never owned-state/output aliases
 for(unsigned i=0;i<prompts;i++)if(prompt[i]>=248320)return false;
 auto* memory=new(std::nothrow) PioneerQ38TokenMemory;
 auto* startup=new(std::nothrow) PioneerQ38TokenStartupPlan;auto* participants=new(std::nothrow) PioneerQ38DispatchLease;
 if(!memory || !startup || !participants){
  delete participants;delete startup;delete memory;return false;
 } // unpublished CPU children ONLY;NEVER delete/detach the existing sticky full-model owner
 if(!c.io.TokenModelSourceScope(false)){delete participants;delete startup;delete memory;return false;}
 for(unsigned i=0;i<prompts;i++)s->prompt[i]=prompt[i];
 s->prompts=prompts;s->limit=limit;s->stopEos=stopEos;s->eos[0]=eos0;s->eos[1]=eos1;
 s->memory=memory;s->startup=startup;s->participants=participants;c.q38Dispatch=participants;
 s->phase=PioneerQ38TokenNativeState::DispatchPreparing;
 auto& dram=c.baseline.baseline.baseline.baseline.dram;auto& noc=c.baseline.baseline.baseline.noc;
 auto& d=*participants;d.worker[0].master=&c.lease;d.worker[0].noc=noc;c.io.fleetEnabled=true;
 bool okay=c.io.SelectRuntimeCore(0) && c.lease.ready && c.io.MapRuntime() && BlackholeDramAllCoresHeld(c.io,dram)
  && c.io.TokenModelSourceScope(false);
 for(unsigned w=1;w<120 && okay;w++)okay=PioneerWorkerNocProbe(c.io,dram,w,d.worker[w].noc);
 c.q38DispatchRoutes=okay;
 for(unsigned w=0;w<120 && okay;w++){
  okay=c.io.PrepareTokenNativeParticipant(w);
  if(!okay)d.prepared=w+1;
 }
 if(okay)okay=startup->Build(s->canonical,s->bank,s->identity,&d.authority.source[0][0],s->prompt,s->prompts,s->limit,s->stopEos,s->eos[0],s->eos[1])
  && startup->LearnedPagesPresent(c.initialized,PioneerResidentBanks::Pages);
 if(okay){s->phase=PioneerQ38TokenNativeState::MemoryPreparing;
  okay=c.io.SelectRuntimeCore(0) && c.io.MapTokenMemory()
   && memory->PrepareHeld(c.io,dram,c.banks,d,s->canonical,*startup,s->identity,c.initialized,PioneerResidentBanks::Pages,result);}
 if(okay)s->phase=PioneerQ38TokenNativeState::MemoryInstalled;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);
  result.unsafe=1;result.installed=0;result.closed=0;}return okay;
} // no bootstrap/reset/start/model compute;ALL120 participants remain held
[[maybe_unused]] static __attribute__((noinline)) bool CloseQ38TokenNativeHeld(PioneerStoreContext& c,PioneerQ38TokenMemoryResult& result){
 auto* s=c.q38TokenNative;
 if(!s || (s->phase!=PioneerQ38TokenNativeState::MemoryInstalled && s->phase!=PioneerQ38TokenNativeState::RuntimeReady)
  || !s->memory || !s->startup || s->Unsafe() || (s->runtime && (!s->runtime->Staged() || s->runtime->Pending())))return false;
 s->phase=PioneerQ38TokenNativeState::MemoryClosing;
 bool okay=c.io.TokenModelSourceScope(false) && s->memory->CloseHeldNoLaunch(c.io,result);
 if(okay)s->phase=PioneerQ38TokenNativeState::MemoryClosed;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);
  result.unsafe=1;result.installed=0;result.closed=0;}return okay;
} // full held-only journal restoration,NOT actual final END/disposal permission
#endif
