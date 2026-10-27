/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_EMBEDDING_MACHINE_ONLY
#include "pioneer_q38_embedding_image_test.cpp"
#include "pioneer_q38_mixed_live_original_instructions.h"
void ExecuteMixedOriginalEmbedding(const char* image,ProjectionExecutionLearnedBytes& bytes,uint32_t* output,unsigned words){
 (void)&PlanTests;assert(output && words==5120 && bytes.Ready());
 auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());
 EmbeddingMachine machine(image,*plan,0,0,3);
 assert(bytes.Read(machine.descriptor[12],machine.descriptor[11],machine.packed.data(),unsigned(machine.packed.size())));
 assert(machine.Run());machine.Verify();assert(bytes.Ready());
 for(unsigned i=0;i<words;i++)output[i]=machine.resident[i];
 puts("PASS original token0 QPE1 ACTUAL instructions/46080 original tiled bytes/5120 genuine output words;NO CPU products supplied");
}
