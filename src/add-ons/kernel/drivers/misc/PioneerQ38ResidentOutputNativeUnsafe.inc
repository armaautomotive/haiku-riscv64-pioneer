/* SPDX-License-Identifier: MIT */
// CPU-only gate inside the SAME real context's Unsafe(). A caller-supplied
// controlledEmbedding argument never establishes the native source scope.
if(q38ResidentOutputNative){
 if(!q38EmbeddingModel || q38ResidentOutputNative->Unsafe()
  || (!controlledEmbedding && !q38ResidentOutputNative->Closed(banks,baseline.baseline.baseline.baseline.dram)))return true;
}
