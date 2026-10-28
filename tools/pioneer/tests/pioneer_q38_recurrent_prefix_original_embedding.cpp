/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_EMBEDDING_MACHINE_ONLY
#include "pioneer_q38_embedding_image_test.cpp"
#include "pioneer_q38_recurrent_prefix_original_instructions.h"
void ExecutePrefixOriginalEmbedding(const char* image,RecurrentBlockRawLearnedBytes& bytes,uint32_t* output,unsigned words){
 (void)&PlanTests;assert(output && words==5120 && bytes.Ready());
 auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());
 EmbeddingMachine machine(image,*plan,0,0,0);
 for(unsigned at=0;at<machine.packed.size();at+=16384){
  unsigned count=std::min(16384U,unsigned(machine.packed.size())-at);
  assert(bytes.Read(machine.descriptor[12],machine.descriptor[11]+at,machine.packed.data()+at,count));
 }
 assert(machine.Run());machine.Verify();assert(bytes.Ready());
 for(unsigned i=0;i<words;i++)output[i]=machine.resident[i];
 puts("PASS prefix original token0 QPE1 ACTUAL instructions/46080 original tiled bytes/5120 output words;NO CPU products supplied");
}
