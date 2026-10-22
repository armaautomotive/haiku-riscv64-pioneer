/* SPDX-License-Identifier: MIT */
// Actual complete native driver source,COMPILE ONLY. New factory/real
// callbacks/mapping/zero/controller/retention/END are instantiated here.
#define PIONEER_Q38_EMBEDDING_NATIVE 1
#include "../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp"
extern "C" bool pioneer_embedding_native_kernel_bind(PioneerStoreContext& c,unsigned token,unsigned position,unsigned bank){
 return BindQ38EmbeddingModel(c,token,position,bank);
}
extern "C" bool pioneer_embedding_native_kernel_initialize(PioneerStoreContext& c,PioneerQ38ProjectionZeroResult& result){
 return InitializeQ38EmbeddingModel(c,result);
}
extern "C" bool pioneer_embedding_native_kernel_run(PioneerStoreContext& c,PioneerQ38EmbeddingResult& result){
 return RunQ38EmbeddingModel(c,result);
}
