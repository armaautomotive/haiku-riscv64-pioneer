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
 return true;
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
#endif
