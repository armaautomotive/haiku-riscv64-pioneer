/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT
if(c.q38MixedLiveNative || c.io.q38MixedLiveMemoryMapAttempted){
#ifdef PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE
 // A closed owner is not permission to touch hardware after its bound
 // capacity/core-mode geometry changes. Final Unmap also checks these,
 // but refusing there would be AFTER bulk/root/clock/fresh-audit IO.
 if(c.bytes!=PioneerResidentBanks::Total || c.runtimeCoreMode!=1){
  clean=false;RetainQ38DispatchOwner(c,result);return false;
 }
#endif
 auto* state=c.q38MixedLiveNative;auto* m=c.q38EmbeddingModel;
 if(!state || !m || !sRuntimeOwner || !c.io.MixedLiveCallScopeMatches() || !state->Closed()
  || !state->MatchesBinding(c,*m,sRuntimeOwner,c.hashes,sRuntimeOwner->team,c.initialized,sReloadGeneration)
  || !c.io.MixedLiveMemoryMappingStable(0)){
  clean=false;RetainQ38DispatchOwner(c,result);return false;
 }
}
#endif
