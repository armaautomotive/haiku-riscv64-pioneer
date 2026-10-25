/* SPDX-License-Identifier: MIT */
// FIRST EndQ38DispatchOwnership gate. Even if the old producer has CLOSED,
// the separate new lifetime owes actual STOP/holds/full restoration. No
// automatic cleanup, mapping release, clock IDLE or cookie free on failure.
if(c.q38ResidentLayerNative){
 auto* state=c.q38ResidentLayerNative;auto* model=c.q38EmbeddingModel;
 if(!model || !sRuntimeOwner || !c.io.ResidentLayerCallScopeMatches() || sRuntimeOwner->context!=&c || sRuntimeOwner->magic!=0x50313030
  || !state->MatchesBinding(c,*model,sRuntimeOwner,c.hashes,sRuntimeOwner->team,c.initialized,sReloadGeneration)
  || !state->Closed(c.banks,c.baseline.baseline.baseline.baseline.dram) || !state->ClosedLayerBinding()
  || !c.io.ResidentLayerMemoryMappingStable(0)){
  clean=false;RetainQ38DispatchOwner(c,result);return false;
 }
}
