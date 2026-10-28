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
#endif
