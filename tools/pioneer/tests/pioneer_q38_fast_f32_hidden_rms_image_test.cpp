/* SPDX-License-Identifier: MIT */
// Real emitted RV32 instructions, synthetic inputs and simulated transport.
#define PIONEER_Q38_RMS_MACHINE_ONLY
#include "pioneer_q38_learned_rms_image_test.cpp"
#include <algorithm>
#include <cmath>
int main(int argc,char** argv){
 (void)&RmsPlanTests;assert(argc==3);auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 unsigned long long oldSteps=0,newSteps=0,changed=0;double worstRelative=0;
 for(unsigned trial=0;trial<6;trial++){
  LearnedRmsMachine old(argv[1],*canonical,0,0,0,trial),fast(argv[2],*canonical,0,0,0,trial);
  assert(old.Run() && fast.Run());old.Verify();assert(old.input==fast.input && old.weight==fast.weight && fast.backup==fast.input);
  assert(fast.packets==8 && fast.readPackets==4 && fast.writePackets==4 && fast.outputWrites==5120);
  double err=0,norm=0;
  for(unsigned i=0;i<5120;i++){
   const double a=PioneerAttentionFloat(old.resident[i]),b=PioneerAttentionFloat(fast.resident[i]);
   assert(std::isfinite(a) && std::isfinite(b));err+=(a-b)*(a-b);norm+=a*a;changed+=old.resident[i]!=fast.resident[i];
  }
  const double relative=std::sqrt(err/(norm+1e-300));assert(relative<0.00002);
  worstRelative=std::max(worstRelative,relative);oldSteps+=old.steps;newSteps+=fast.steps;
 }
 assert(newSteps<oldSteps);
 unsigned refused=0;
 for(unsigned index:{0U,5119U})for(uint32_t bits:{1U,0x267fffffU,0x4c800001U,0x7f800000U,0x7fc00000U}){
  LearnedRmsMachine fast(argv[2],*canonical,0,0,0);fast.input[index]=fast.resident[index]=bits;
  assert(!fast.Run() && fast.readPackets==4 && !fast.outputWrites);fast.NoSink();refused++;
 }
 for(uint32_t bits:{0x47800000U,0x7f800000U,0x7fc00000U}){
  LearnedRmsMachine fast(argv[2],*canonical,0,0,0);fast.weight[5119]=bits;
  assert(!fast.Run() && fast.readPackets==4 && !fast.outputWrites);fast.NoSink();refused++;
 }
 printf("PASS emitted RV32 synthetic RMS: 6 rows, %u refusals, precise instructions%llu fast instructions%llu changed_words%llu worst_relative_l2%.9g; "
  "SIMULATED ONLY, NO physical timing, real-model quality or tokens/sec\n",refused,oldSteps,newSteps,changed,worstRelative);
}
