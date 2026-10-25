/* SPDX-License-Identifier: MIT */
// CPU-only gate inside the SAME real context's Unsafe(). A caller-supplied
// controlledEmbedding argument never establishes the native source scope.
if(io.q38ResidentLayerMemoryMapAttempted && (!q38ResidentLayerNative || !q38ResidentLayerNative->HasLayerMemory()))return true;
if(q38ResidentLayerNative){
 if(!q38EmbeddingModel || q38ResidentLayerNative->Unsafe()
  || (!controlledEmbedding && !q38ResidentLayerNative->Closed(banks,baseline.baseline.baseline.baseline.dram)))return true;
}
