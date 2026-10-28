/* SPDX-License-Identifier: MIT */
// Actual NEW emitted arithmetic from reset;raw unit inputs only,not a
// completed model producer or a replacement whole-model/native qualifier.
#define PIONEER_Q38_RMS_MACHINE_ONLY
#include "pioneer_q38_learned_rms_image_test.cpp"
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RmsPlanTests;
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 unsigned positive=0,refusals=0,stack=0,maxSteps=0;
 for(unsigned trial=0;trial<8;trial++){
  LearnedRmsMachine m(argv[1],*canonical,0,0,0);
  for(unsigned i=0;i<5120;i++){
   float x=trial==0?PioneerAttentionFloat(0x47816cda):trial==1?67108864.f:trial==2?(i?67108864.f:PioneerAttentionFloat(0x26800000))
    :trial==3?(i?0.f:67108864.f):trial==4?-0.f:trial==5?PioneerAttentionFloat(0x26800000):trial==6?(i&1?67108864.f:-67108864.f):float(int(i%127)-63);
   float w=trial==2?PioneerAttentionFloat(0x26800000):trial==3?65504.f:1.f;
   m.input[i]=m.resident[i]=PioneerAttentionBits(x);m.weight[i]=PioneerAttentionBits(w);
  }
  assert(!m.steps && m.Run());m.Verify();positive++;stack=std::max(stack,0x70000-m.minStack);maxSteps=std::max(maxSteps,m.steps);
 }
 for(unsigned index:{0U,5119U})for(uint32_t bits:{1U,0x267fffffU,0x4c800001U,0x7f800000U,0x7fc00000U}){
  LearnedRmsMachine m(argv[1],*canonical,0,0,0);m.input[index]=m.resident[index]=bits;
  assert(!m.Run());m.NoSink();assert(m.readPackets==4 && !m.outputWrites);refusals++;
 }
 for(uint32_t bits:{0x47800000U,0x47816cdaU,0x7f800000U,0x7fc00000U}){
  LearnedRmsMachine m(argv[1],*canonical,0,0,0);m.weight[5119]=bits;
  assert(!m.Run());m.NoSink();assert(m.readPackets==4 && !m.outputWrites);refusals++;
 }
 printf("PASS ACTUAL bounded FP32 RMS numeric image/fromRESET/positives%u/domain refusals%u/all5120 bitexact independent double+sqrtf/raw backup exact/unchanged65504 weight domain/NO GDDR write on refusal/stack%u/maxInstructions%u;NOT residual/controller/model/native/physical/Work/TPS proof\n",positive,refusals,stack,maxSteps);
}
