/* SPDX-License-Identifier: MIT */
// Actual QCB1 instructions/packet endpoints,NO host convolution/SiLU helper.
#define PIONEER_Q38_CONV_BATCH_MACHINE_ONLY
#include "pioneer_q38_conv_batch_image_test.cpp"
#include "pioneer_q38_conv_batch_precision_probe_bridge.h"
PioneerQ38ConvPrecisionProbeStats ExecuteConvBatchPrecisionProbe(const char* image,const uint32_t* input,
 const uint32_t* weights,uint32_t* output,uint32_t* next){
 assert(image && input && weights && output && next);(void)&Plans;(void)&Sequence;(void)&BatchPlans;
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 std::array<unsigned,640> coverage={};PioneerQ38ConvPrecisionProbeStats result={};
 for(unsigned worker=0;worker<120;worker++){
  ConvBatchMachine machine(image,*canonical,0,0,worker,0,0);
  for(unsigned j=0;j<machine.count;j++){
   auto& group=machine.groups[j];unsigned first=machine.table[j*64+7];
   assert(first%16==0 && first<10240 && !coverage[first/16]++);
   for(unsigned c=0;c<16;c++){
    group.input[c]=input[first+c];
    for(unsigned tap=0;tap<4;tap++)group.weight[c*4+tap]=weights[(first+c)*4+tap];
    for(unsigned tap=0;tap<3;tap++)group.previous[c*3+tap]=0; // original token0 history
   }
  }
  assert(machine.RunBatch());machine.VerifyBatch();
  for(unsigned j=0;j<machine.count;j++){
   const auto& group=machine.groups[j];unsigned first=machine.table[j*64+7];
   for(unsigned c=0;c<16;c++){
    output[first+c]=group.output[c];for(unsigned tap=0;tap<3;tap++)next[(first+c)*3+tap]=group.next[c*3+tap];
   }
   result.groups++;
  }
  result.workers++;result.maxStack=std::max(result.maxStack,0x70000-machine.minStack);result.maxSteps=std::max(result.maxSteps,machine.steps);
 }
 for(auto count:coverage)assert(count==1);
 assert(result.workers==120 && result.groups==640);return result;
}
