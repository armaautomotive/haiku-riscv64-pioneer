/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT
#include "PioneerQ38RecurrentPrefixNativeRole.h"
[[maybe_unused]] static bool PrepareQ38RecurrentPrefixNativeExecution(PioneerStoreContext& c,PioneerQ38RecurrentPrefixResult& r){
 auto* state=c.q38RecurrentPrefixNative;auto* m=c.q38EmbeddingModel;const PioneerQ38RecurrentPrefixResult pristine;
 const auto* a=reinterpret_cast<const unsigned char*>(&r);const auto* b=reinterpret_cast<const unsigned char*>(&pristine);
 static_assert(sizeof(r)==64,"every result byte has an explicit pristine value");
 for(unsigned i=0;i<sizeof(r);i++)if(a[i]!=b[i])return false;
 if(!state || !m || state->phase!=PioneerQ38RecurrentPrefixNativeState::MemoryInstalled || state->execution || !state->memory || !state->plan
  || !c.q38Dispatch || !state->InitializationsClosed(c.banks,c.baseline.baseline.baseline.baseline.dram) || !c.io.RecurrentPrefixBaseScope())return false;
 auto* execution=new(std::nothrow) PioneerQ38RecurrentPrefixOwner;if(!execution)return false;
 state->execution=execution;state->phase=PioneerQ38RecurrentPrefixNativeState::ExecutionPreparing;
 bool okay=execution->Prepare(c.io,c.baseline.baseline.baseline.baseline.dram,c.banks,*c.q38Dispatch,*state->memory,m->plan,*state->plan,
  m->identity,c.initialized,PioneerResidentBanks::Pages,r);
 if(okay)state->phase=PioneerQ38RecurrentPrefixNativeState::ExecutionReady;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
}
[[maybe_unused]] static bool RunQ38RecurrentPrefixNativeExecution(PioneerStoreContext& c,PioneerQ38RecurrentPrefixResult& r){
 auto* state=c.q38RecurrentPrefixNative;
 if(!state || state->phase!=PioneerQ38RecurrentPrefixNativeState::ExecutionReady || !state->execution || state->Unsafe())return false;
 bool okay=c.io.RecurrentPrefixBaseScope() && state->execution->Run(c.io,r);
 if(okay)state->phase=PioneerQ38RecurrentPrefixNativeState::ExecutionComplete;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
}
[[maybe_unused]] static bool CloseQ38RecurrentPrefixNativeExecution(PioneerStoreContext& c,PioneerQ38RecurrentPrefixResult& r,PioneerQ38RecurrentPrefixMemoryResult& memory){
 auto* state=c.q38RecurrentPrefixNative;
 if(!state || state->phase!=PioneerQ38RecurrentPrefixNativeState::ExecutionComplete || !state->execution || state->Unsafe())return false;
 state->phase=PioneerQ38RecurrentPrefixNativeState::ExecutionClosing;
 bool okay=c.io.RecurrentPrefixBaseScope() && state->execution->Close(c.io,r,memory);
 if(okay)state->phase=PioneerQ38RecurrentPrefixNativeState::MemoryClosed;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
} // private renamed NODRV only;NO public launch/Work/native token commit
#ifdef PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_READBACK
static bool Q38RecurrentPrefixReadbackNativeOverlap(const void* output,size_t bytes,const void* object,size_t size){
 const uintptr_t a=reinterpret_cast<uintptr_t>(output),b=reinterpret_cast<uintptr_t>(object);
 return !output || !object || a>UINTPTR_MAX-bytes || b>UINTPTR_MAX-size || (a<b+size && b<a+bytes);
}
[[maybe_unused]] __attribute__((noinline)) static bool CloseQ38RecurrentPrefixNativeValidation(PioneerStoreContext& c,PioneerQ38RecurrentPrefixResult& r,PioneerQ38RecurrentPrefixMemoryResult& memory,
 uint32_t* values,unsigned words,PioneerQ38RecurrentPrefixReadbackResult& out){
 auto* state=c.q38RecurrentPrefixNative;
 if(!state || !c.q38EmbeddingModel || words!=TT_Q38_RECURRENT_PREFIX_READBACK_WORDS
  || state->phase!=PioneerQ38RecurrentPrefixNativeState::ExecutionComplete || !state->execution || state->Unsafe())return false;
 const void* objects[]={&c,state,c.q38EmbeddingModel};const size_t sizes[]={sizeof(c),sizeof(*state),sizeof(*c.q38EmbeddingModel)};
 for(unsigned i=0;i<3;i++)if(Q38RecurrentPrefixReadbackNativeOverlap(values,words*4,objects[i],sizes[i])
  || Q38RecurrentPrefixReadbackNativeOverlap(&out,sizeof(out),objects[i],sizes[i]))return false;
 if(!state->execution->CanCloseForValidation(values,words,out,r,memory))return false;
 state->phase=PioneerQ38RecurrentPrefixNativeState::ExecutionClosing;
 bool okay=c.io.RecurrentPrefixBaseScope() && state->execution->CloseForValidation(c.io,r,memory,values,words,out);
 if(okay)state->phase=PioneerQ38RecurrentPrefixNativeState::MemoryClosed;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
} // trusted kernel allocation only;never user/CPU-reference or Work selection
#endif
#endif
