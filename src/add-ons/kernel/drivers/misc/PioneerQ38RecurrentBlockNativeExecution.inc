/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT
#include "PioneerQ38RecurrentBlockNativeRole.h"
[[maybe_unused]] static bool PrepareQ38RecurrentBlockNativeExecution(PioneerStoreContext& c,PioneerQ38RecurrentBlockResult& r){
 auto* state=c.q38RecurrentBlockNative;auto* m=c.q38EmbeddingModel;const PioneerQ38RecurrentBlockResult pristine;
 const auto* a=reinterpret_cast<const unsigned char*>(&r);const auto* b=reinterpret_cast<const unsigned char*>(&pristine);
 static_assert(sizeof(r)==64,"every result byte has an explicit pristine value");
 for(unsigned i=0;i<sizeof(r);i++)if(a[i]!=b[i])return false;
 if(!state || !m || state->phase!=PioneerQ38RecurrentBlockNativeState::MemoryInstalled || state->execution || !state->memory || !state->plan
  || !c.q38Dispatch || !state->InitializationsClosed(c.banks,c.baseline.baseline.baseline.baseline.dram) || !c.io.RecurrentBlockBaseScope())return false;
 auto* execution=new(std::nothrow) PioneerQ38RecurrentBlockOwner;if(!execution)return false;
 state->execution=execution;state->phase=PioneerQ38RecurrentBlockNativeState::ExecutionPreparing;
 bool okay=execution->Prepare(c.io,c.baseline.baseline.baseline.baseline.dram,c.banks,*c.q38Dispatch,*state->memory,m->plan,*state->plan,
  m->identity,c.initialized,PioneerResidentBanks::Pages,r);
 if(okay)state->phase=PioneerQ38RecurrentBlockNativeState::ExecutionReady;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
}
[[maybe_unused]] static bool RunQ38RecurrentBlockNativeExecution(PioneerStoreContext& c,PioneerQ38RecurrentBlockResult& r){
 auto* state=c.q38RecurrentBlockNative;
 if(!state || state->phase!=PioneerQ38RecurrentBlockNativeState::ExecutionReady || !state->execution || state->Unsafe())return false;
 bool okay=c.io.RecurrentBlockBaseScope() && state->execution->Run(c.io,r);
 if(okay)state->phase=PioneerQ38RecurrentBlockNativeState::ExecutionComplete;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
}
[[maybe_unused]] static bool CloseQ38RecurrentBlockNativeExecution(PioneerStoreContext& c,PioneerQ38RecurrentBlockResult& r,PioneerQ38RecurrentBlockMemoryResult& memory){
 auto* state=c.q38RecurrentBlockNative;
 if(!state || state->phase!=PioneerQ38RecurrentBlockNativeState::ExecutionComplete || !state->execution || state->Unsafe())return false;
 state->phase=PioneerQ38RecurrentBlockNativeState::ExecutionClosing;
 bool okay=c.io.RecurrentBlockBaseScope() && state->execution->Close(c.io,r,memory);
 if(okay)state->phase=PioneerQ38RecurrentBlockNativeState::MemoryClosed;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
} // private renamed NODRV only;NO public launch/Work/native token commit
#ifdef PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_READBACK
static bool Q38RecurrentReadbackNativeOverlap(const void* output,size_t bytes,const void* object,size_t size){
 const uintptr_t a=reinterpret_cast<uintptr_t>(output),b=reinterpret_cast<uintptr_t>(object);
 return !output || !object || a>UINTPTR_MAX-bytes || b>UINTPTR_MAX-size || (a<b+size && b<a+bytes);
}
[[maybe_unused]] static bool CloseQ38RecurrentBlockNativeValidation(PioneerStoreContext& c,PioneerQ38RecurrentBlockResult& r,PioneerQ38RecurrentBlockMemoryResult& memory,
 uint32_t* values,unsigned words,PioneerQ38RecurrentBlockReadbackResult& out){
 auto* state=c.q38RecurrentBlockNative;
 if(!state || !c.q38EmbeddingModel || words!=TT_Q38_RECURRENT_BLOCK_READBACK_WORDS
  || state->phase!=PioneerQ38RecurrentBlockNativeState::ExecutionComplete || !state->execution || state->Unsafe())return false;
 const void* objects[]={&c,state,c.q38EmbeddingModel};const size_t sizes[]={sizeof(c),sizeof(*state),sizeof(*c.q38EmbeddingModel)};
 for(unsigned i=0;i<3;i++)if(Q38RecurrentReadbackNativeOverlap(values,words*4,objects[i],sizes[i])
  || Q38RecurrentReadbackNativeOverlap(&out,sizeof(out),objects[i],sizes[i]))return false;
 if(!state->execution->CanCloseForValidation(values,words,out,r,memory))return false;
 state->phase=PioneerQ38RecurrentBlockNativeState::ExecutionClosing;
 bool okay=c.io.RecurrentBlockBaseScope() && state->execution->CloseForValidation(c.io,r,memory,values,words,out);
 if(okay)state->phase=PioneerQ38RecurrentBlockNativeState::MemoryClosed;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
} // trusted kernel output allocation only;no public control or Work grant
#endif
#endif
