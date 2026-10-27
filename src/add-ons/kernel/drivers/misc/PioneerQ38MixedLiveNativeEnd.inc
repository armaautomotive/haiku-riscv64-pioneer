/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT
if(c.q38MixedLiveNative || c.io.q38MixedLiveMemoryMapAttempted){
 auto* state=c.q38MixedLiveNative;auto* m=c.q38EmbeddingModel;
 if(!state || !m || !sRuntimeOwner || !c.io.MixedLiveCallScopeMatches() || !state->Closed()
  || !state->MatchesBinding(c,*m,sRuntimeOwner,c.hashes,sRuntimeOwner->team,c.initialized,sReloadGeneration)
  || !c.io.MixedLiveMemoryMappingStable(0)){
  clean=false;RetainQ38DispatchOwner(c,result);return false;
 }
}
#endif
