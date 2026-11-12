/* SPDX-License-Identifier: MIT */
// Actual experimental RV32 leaf under NoC/FPU simulation, never a card run.
#define PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION 1
#define PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT 1
#define PIONEER_BFP8_WORKER_IMAGE_ONLY 1
#include "pioneer_q38_bfp8_worker_leaf_image_test.cpp"
int main(int argc,char** argv){
 assert(argc==2);
 for(unsigned worker:{1U,63U,119U})for(unsigned bands:{1U,3U,8U}){
  WorkerImage image(argv[1],worker,6144,bands,true);
  assert(image.Execute());
  const auto* expected=PioneerNativeBfpBandGolden(6144);assert(expected);
  for(unsigned band=0;band<bands;band++)for(unsigned row=0;row<32;row++){
   const auto value=image.l1[image.Index(W::Output+(band*32+row)*4)];
   if(band&1)assert((value&0x7fffffffU)==0);else assert(value==expected[row]);
  }
  std::printf("PASS experimental RV32 K6144 worker%u bands%u steps%u; NO DEVICE\n",worker,bands,image.steps);
 }
 WorkerImage corrupted(argv[1],1,6144);corrupted.damageFrame=true;
 assert(!corrupted.Execute() && corrupted.stores==32);
 WorkerImage dropped(argv[1],1,6144);dropped.dropAckAt=1;
 assert(!dropped.Execute() && !dropped.stores);
 std::puts("PASS experimental RV32 recurrent frame and FPU-failure controls; NO DEVICE");
}
