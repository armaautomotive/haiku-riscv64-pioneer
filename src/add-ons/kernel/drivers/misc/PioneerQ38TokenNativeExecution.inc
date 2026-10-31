/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
#include "PioneerQ38TokenNativeRole.h"
[[maybe_unused]] static bool Q38TokenExecutionOutput(const PioneerStoreContext& c,const void* output,size_t bytes){
 const auto* s=c.q38TokenNative;
 if(!s || !s->runtime || !s->startup || !s->memory || !s->participants || !output || bytes!=sizeof(PioneerQ38TokenExecutionResult)
  || !s->startup->PublishedOutput(output,bytes))return false;
 const uintptr_t out=reinterpret_cast<uintptr_t>(output);if(out>UINTPTR_MAX-bytes)return false;
 const void* objects[]={&c,&c.io,s,s->memory,s->participants,s->runtime,c.initialized,c.hashes,PioneerQ38TokenRope::Data};
 const size_t sizes[]={sizeof(c),sizeof(c.io),sizeof(*s),sizeof(*s->memory),sizeof(*s->participants),sizeof(*s->runtime),
  PioneerResidentBanks::Pages,size_t(PioneerResidentBanks::Pages)*sizeof(uint32_t),PioneerQ38TokenRope::Bytes};
 for(unsigned i=0;i<9;i++){
  const uintptr_t owned=reinterpret_cast<uintptr_t>(objects[i]);
  if(!owned || owned>UINTPTR_MAX-sizes[i] || (out<owned+sizes[i] && owned<out+bytes))return false;
 }
 return s->runtime->DispatchOutput(output,bytes);
}
[[maybe_unused]] static __attribute__((noinline)) bool StartQ38TokenNativeRoot(PioneerStoreContext& c,PioneerQ38TokenExecutionResult& result){
 auto* s=c.q38TokenNative;const PioneerQ38TokenExecutionResult pristine;
 const auto* a=reinterpret_cast<const uint8_t*>(&result);const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
 for(unsigned i=0;i<sizeof(result);i++)if(a[i]!=b[i])return false;
 if(!s || s->phase!=PioneerQ38TokenNativeState::WorkersReady || !s->runtime || !s->runtime->WorkersReady()
  || !Q38TokenExecutionOutput(c,&result,sizeof(result)) || !c.io.TokenModelSourceScope(false))return false;
 s->phase=PioneerQ38TokenNativeState::RootStarting;bool okay=s->runtime->StartRoot(c.io,result);
 if(okay && s->runtime->ExecutionStarted())s->phase=PioneerQ38TokenNativeState::RootRunning;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);result.unsafe=1;result.acknowledged=0;okay=false;}
 return okay;
} // actual entered ROOT ROM ONLY;not completed layers/tokens/STOP/END/Work
[[maybe_unused]] static __attribute__((noinline)) bool CancelQ38TokenNativeRoot(PioneerStoreContext& c,PioneerQ38TokenExecutionResult& result){
 auto* s=c.q38TokenNative;const PioneerQ38TokenExecutionResult pristine;
 const auto* a=reinterpret_cast<const uint8_t*>(&result);const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
 for(unsigned i=0;i<sizeof(result);i++)if(a[i]!=b[i])return false;
 if(!s || s->phase!=PioneerQ38TokenNativeState::RootRunning || !s->runtime || !s->runtime->ExecutionStarted()
  || !Q38TokenExecutionOutput(c,&result,sizeof(result)) || !c.io.TokenModelSourceScope(false))return false;
 return s->runtime->RequestCancel(c.io,result); // one monotonic RAW request,NEVER a STOP/END receipt
}
[[maybe_unused]] static __attribute__((noinline)) bool ObserveQ38TokenNativeRoot(PioneerStoreContext& c,PioneerQ38TokenExecutionResult& result){
 auto* s=c.q38TokenNative;const PioneerQ38TokenExecutionResult pristine;
 const auto* a=reinterpret_cast<const uint8_t*>(&result);const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
 for(unsigned i=0;i<sizeof(result);i++)if(a[i]!=b[i])return false;
 if(!s || s->phase!=PioneerQ38TokenNativeState::RootRunning || !s->runtime || !s->runtime->ExecutionStarted()
  || !Q38TokenExecutionOutput(c,&result,sizeof(result)) || !c.io.TokenModelSourceScope(false))return false;
 return s->runtime->ObserveRoot(c.io,result);
} // bounded source/marker/PC observation while firmware owns NoC;NEVER a token/STOP/END receipt
[[maybe_unused]] static __attribute__((noinline)) bool ObserveQ38TokenNativeTerminal(PioneerStoreContext& c,PioneerQ38TokenTerminalResult& result){
 auto* s=c.q38TokenNative;const PioneerQ38TokenTerminalResult pristine;
 const auto* a=reinterpret_cast<const uint8_t*>(&result);const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
 for(unsigned i=0;i<sizeof(result);i++)if(a[i]!=b[i])return false;
 if(!s || s->phase!=PioneerQ38TokenNativeState::RootRunning || !s->runtime || !s->runtime->ExecutionStarted()
  || !Q38TokenExecutionOutput(c,&result,sizeof(result)) || !c.io.TokenModelSourceScope(false))return false;
 return s->runtime->ObserveTerminal(c.io,result);
} // accepted STOP observation only. State::Closed stays false;no old cleanup/END/Work route is enabled.
[[maybe_unused]] static __attribute__((noinline)) bool HoldQ38TokenNativeStopped(PioneerStoreContext& c,PioneerQ38TokenHoldResult& result){
 auto* s=c.q38TokenNative;const PioneerQ38TokenHoldResult pristine;
 const auto* a=reinterpret_cast<const uint8_t*>(&result);const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
 for(unsigned i=0;i<sizeof(result);i++)if(a[i]!=b[i])return false;
 if(!s || s->phase!=PioneerQ38TokenNativeState::RootRunning || !s->TerminalObserved() || !s->runtime
  || !Q38TokenExecutionOutput(c,&result,sizeof(result)) || !c.io.TokenModelSourceScope(false))return false;
 s->phase=PioneerQ38TokenNativeState::RootHolding;bool okay=s->runtime->HoldStopped(c.io,result);
 if(okay && s->runtime->HeldAfterTerminal())s->phase=PioneerQ38TokenNativeState::RootHeld;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);result.unsafe=1;result.ready=0;okay=false;}
 return okay;
} // fresh owned handoff ONLY;no CSR/L1 restore,unmapping,host release,END/public Work or generated-token delivery
[[maybe_unused]] static __attribute__((noinline)) bool ReadQ38TokenNativeRecords(PioneerStoreContext& c,unsigned records,uint32_t* output,PioneerQ38TokenReadResult& result){
 auto* s=c.q38TokenNative;const PioneerQ38TokenReadResult pristine;
 const auto* a=reinterpret_cast<const uint8_t*>(&result);const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
 for(unsigned i=0;i<sizeof(result);i++)if(a[i]!=b[i])return false;
 if(!s || s->phase!=PioneerQ38TokenNativeState::RootHeld || !s->runtime || !s->TerminalHeld()
  || s->runtime->CommittedRecordsRead() || records>64 || (records?!output:output!=nullptr)
  || !Q38TokenExecutionOutput(c,&result,sizeof(result)))return false;
 const unsigned bytes=records*unsigned(PioneerQ38TokenLayout::RecordBytes);
 if(records){
  const uintptr_t out=reinterpret_cast<uintptr_t>(output),r=reinterpret_cast<uintptr_t>(&result);
  if((out&3) || out>UINTPTR_MAX-bytes || (out<r+sizeof(result) && r<out+bytes) || !s->startup->PublishedOutput(output,bytes))return false;
  const void* objects[]={&c,&c.io,s,s->memory,s->participants,s->runtime,c.initialized,c.hashes,PioneerQ38TokenRope::Data};
  const size_t sizes[]={sizeof(c),sizeof(c.io),sizeof(*s),sizeof(*s->memory),sizeof(*s->participants),sizeof(*s->runtime),
   PioneerResidentBanks::Pages,size_t(PioneerResidentBanks::Pages)*sizeof(uint32_t),PioneerQ38TokenRope::Bytes};
  for(unsigned i=0;i<9;i++){
   const uintptr_t owned=reinterpret_cast<uintptr_t>(objects[i]);
   if(!owned || owned>UINTPTR_MAX-sizes[i] || (out<owned+sizes[i] && owned<out+bytes))return false;
  }
  for(unsigned i=0;i<records*64;i++)if(output[i])return false;
  if(!s->runtime->DispatchOutput(output,bytes))return false;
 } // private trusted kernel buffers ONLY;not an unchecked public/user-pointer ABI
 if(!c.io.TokenModelSourceScope(false))return false;
 return s->runtime->ReadCommitted(c.io,records,output,result);
} // immutable committed DATA after genuine HOLD;NO CPU model reference,restore,END or public Work route
#if defined(PIONEER_Q38_TOKEN_HELD_HIDDEN_DIAGNOSTIC) || defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE)
#include "PioneerQ38TokenHiddenReadRole.h"
[[maybe_unused]] static __attribute__((noinline)) bool ReadQ38TokenNativeHiddenChunk(PioneerStoreContext& c,unsigned part,
 uint32_t* output,unsigned words,PioneerQ38TokenHiddenReadResult& result){
 const PioneerQ38TokenHiddenReadResult pristine;
 const auto* a=reinterpret_cast<const uint8_t*>(&result);const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
 for(unsigned i=0;i<sizeof(result);i++)if(a[i]!=b[i])return false;
 auto* s=c.q38TokenNative;
 if(part>=2 || words!=(part?1024U:4096U) || !output || !s || s->phase!=PioneerQ38TokenNativeState::RootHeld
  || !s->runtime || !s->TerminalHeld() || !s->runtime->CommittedRecordsRead()
  || !Q38TokenExecutionOutput(c,&result,sizeof(result)))return false;
 const unsigned bytes=words*4;const uintptr_t out=reinterpret_cast<uintptr_t>(output),r=reinterpret_cast<uintptr_t>(&result);
 if((out&3) || out>UINTPTR_MAX-bytes || (out<r+sizeof(result) && r<out+bytes)
  || !s->startup->PublishedOutput(output,bytes) || !s->runtime->DispatchOutput(output,bytes))return false;
 const void* objects[]={&c,&c.io,s,s->memory,s->participants,s->runtime,c.initialized,c.hashes,PioneerQ38TokenRope::Data};
 const size_t sizes[]={sizeof(c),sizeof(c.io),sizeof(*s),sizeof(*s->memory),sizeof(*s->participants),sizeof(*s->runtime),
  PioneerResidentBanks::Pages,size_t(PioneerResidentBanks::Pages)*sizeof(uint32_t),PioneerQ38TokenRope::Bytes};
 for(unsigned i=0;i<9;i++){
  const uintptr_t owned=reinterpret_cast<uintptr_t>(objects[i]);
  if(!owned || owned>UINTPTR_MAX-sizes[i] || (out<owned+sizes[i] && owned<out+bytes))return false;
 }
 for(unsigned i=0;i<words;i++)if(output[i])return false;
 if(!c.io.TokenModelSourceScope(false))return false;
 return s->runtime->ReadHiddenChunk(c.io,part,output,words,result);
} // private trusted-buffer DATA read ONLY;no opcode,Work,restore or completion grant
#endif
[[maybe_unused]] static __attribute__((noinline)) bool RestoreQ38TokenNativeL1(PioneerStoreContext& c,PioneerQ38TokenRestoreResult& result){
 auto* s=c.q38TokenNative;const PioneerQ38TokenRestoreResult pristine;
 const auto* a=reinterpret_cast<const uint8_t*>(&result);const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
 for(unsigned i=0;i<sizeof(result);i++)if(a[i]!=b[i])return false;
 if(!s || s->phase!=PioneerQ38TokenNativeState::RootHeld || !s->runtime || !s->TerminalHeld()
  || !s->runtime->CommittedRecordsRead() || !Q38TokenExecutionOutput(c,&result,sizeof(result)) || !c.io.TokenModelSourceScope(false))return false;
 s->phase=PioneerQ38TokenNativeState::RootRestoring;bool okay=s->runtime->RestoreOriginalL1(c.io,result);
 if(okay && s->runtime->OriginalL1Restored())s->phase=PioneerQ38TokenNativeState::RootRestored;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);result.unsafe=1;result.ready=0;okay=false;}
 return okay;
} // exact full-token L1 journal ONLY;dispatch/CSR leases,clock,maps,hosts remain owned. END STILL REFUSES.
[[maybe_unused]] static __attribute__((noinline)) bool RestoreQ38TokenNativeDispatchImages(PioneerStoreContext& c,PioneerQ38TokenDispatchRestoreResult& result){
 auto* s=c.q38TokenNative;const PioneerQ38TokenDispatchRestoreResult pristine;
 const auto* a=reinterpret_cast<const uint8_t*>(&result);const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
 for(unsigned i=0;i<sizeof(result);i++)if(a[i]!=b[i])return false;
 if(!s || s->phase!=PioneerQ38TokenNativeState::RootRestored || !s->runtime || !s->OriginalL1Restored()
  || !Q38TokenExecutionOutput(c,&result,sizeof(result)) || !c.io.TokenModelSourceScope(false))return false;
 s->phase=PioneerQ38TokenNativeState::DispatchRestoring;bool okay=s->runtime->RestoreDispatchImages(c.io,result);
 if(okay && s->runtime->DispatchImagesRestored())s->phase=PioneerQ38TokenNativeState::DispatchRestored;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);result.unsafe=1;result.ready=0;okay=false;}
 return okay;
} // distinct lower dispatch L1 restoration ONLY;NIU/scratch/TLB/master/clock/maps/hosts and END remain owned
[[maybe_unused]] static __attribute__((noinline)) bool RestoreQ38TokenNativePeerStores(PioneerStoreContext& c,PioneerQ38TokenPeerRestoreResult& result){
 auto* s=c.q38TokenNative;const PioneerQ38TokenPeerRestoreResult pristine;
 const auto* a=reinterpret_cast<const uint8_t*>(&result);const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
 for(unsigned i=0;i<sizeof(result);i++)if(a[i]!=b[i])return false;
 if(!s || s->phase!=PioneerQ38TokenNativeState::DispatchRestored || !s->runtime || !s->DispatchImagesRestored()
  || !Q38TokenExecutionOutput(c,&result,sizeof(result)) || !c.io.TokenModelSourceScope(false))return false;
 s->phase=PioneerQ38TokenNativeState::PeerStoresRestoring;bool okay=s->runtime->RestorePeerStores(c.io,result);
 if(okay && s->runtime->PeerStoresRestored())s->phase=PioneerQ38TokenNativeState::PeerStoresRestored;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);result.unsafe=1;result.ready=0;okay=false;}
 return okay;
} //119 local NIU/scratch leases ONLY;ROOT/sharedTLBs/master/clock/maps/hosts/END remain owned
[[maybe_unused]] static __attribute__((noinline)) bool RestoreQ38TokenNativeRootStore(PioneerStoreContext& c,PioneerQ38TokenRootStoreRestoreResult& result){
 auto* s=c.q38TokenNative;const PioneerQ38TokenRootStoreRestoreResult pristine;
 const auto* a=reinterpret_cast<const uint8_t*>(&result);const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
 for(unsigned i=0;i<sizeof(result);i++)if(a[i]!=b[i])return false;
 if(!s || s->phase!=PioneerQ38TokenNativeState::PeerStoresRestored || !s->runtime || !s->PeerStoreOriginalsRestored()
  || !Q38TokenExecutionOutput(c,&result,sizeof(result)) || !c.io.TokenModelSourceScope(false))return false;
 s->phase=PioneerQ38TokenNativeState::RootStoreRestoring;bool okay=s->runtime->RestoreRootStore(c.io,result);
 if(okay && s->runtime->RootStoreOriginalsRestored())s->phase=PioneerQ38TokenNativeState::RootStoreRestored;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);result.unsafe=1;result.ready=0;okay=false;}
 return okay;
} // ROOT-local originals only;master.ready/shared-TLB/clock/maps/hosts/END retained
[[maybe_unused]] static __attribute__((noinline)) bool RestoreQ38TokenNativeSharedMappings(PioneerStoreContext& c,PioneerQ38TokenSharedTlbRestoreResult& result){
 auto* s=c.q38TokenNative;const PioneerQ38TokenSharedTlbRestoreResult pristine;
 const auto* a=reinterpret_cast<const uint8_t*>(&result);const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
 for(unsigned i=0;i<sizeof(result);i++)if(a[i]!=b[i])return false;
 if(!s || s->phase!=PioneerQ38TokenNativeState::RootStoreRestored || !s->runtime || !s->RootStoreOriginalsRestored()
  || !Q38TokenExecutionOutput(c,&result,sizeof(result)) || !c.io.TokenModelSourceScope(false))return false;
 s->phase=PioneerQ38TokenNativeState::SharedTlbRestoring;bool okay=s->runtime->RestoreSharedMappings(c.io,result);
 if(okay && s->runtime->SharedMappingsRestored())s->phase=PioneerQ38TokenNativeState::SharedTlbRestored;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);result.unsafe=1;result.ready=0;okay=false;}
 return okay;
} // shared original mappings only;clock/master-ready/bulk/VM/hosts/END remain owned
#endif
