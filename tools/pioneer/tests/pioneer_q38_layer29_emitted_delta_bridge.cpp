/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_LAYER29_EMITTED_DELTA_DIAGNOSTIC) || PIONEER_Q38_LAYER29_EMITTED_DELTA_DIAGNOSTIC != 1 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(__FAST_MATH__)
#error Separate HOST emitted arithmetic diagnostic only; no CPU oracle/native/Work
#endif
#define PIONEER_Q38_DELTA_VECTOR_PROBE_IMAGE
#define PIONEER_Q38_DELTA_VECTOR_POST_SCALE
#define PIONEER_Q38_DELTA_VECTOR_IMAGE_MACHINE_ONLY
#include "pioneer_q38_delta_vector_image_test.cpp"
#include "pioneer_q38_layer29_emitted_delta_bridge.h"
#include <cfenv>

bool Q38Layer29EmittedDelta(const char* image,const float* q,const float* k,const float* value,
 float decay,float beta,float* state,float* output,uint64_t* steps,uint64_t* ops){
 if(!image || !q || !k || !value || !state || !output || !steps || !ops || std::fegetround()!=FE_TONEAREST)return false;
 std::array<uint32_t,16384> next={};std::array<uint32_t,128> result={};
 uint64_t totalSteps=0,totalOps=0;
 for(unsigned half=0;half<2;half++){
  DeltaVectorImage machine(image,0);
  for(unsigned key=0;key<128;key++){
   machine.memory[uint32_t(L::Query)+key*4]=Bits(q[key]);
   machine.memory[uint32_t(L::Key)+key*4]=Bits(k[key]);
   for(unsigned v=0;v<64;v++)machine.memory[0x90000+(key*64+v)*4]=Bits(state[key*128+half*64+v]);
  }
  for(unsigned v=0;v<64;v++)machine.memory[uint32_t(L::Value)+v*4]=Bits(value[half*64+v]);
  machine.memory[uint32_t(L::Decay)]=Bits(decay);machine.memory[uint32_t(L::Beta)]=Bits(beta);
  if(!machine.RunVector())return false;
  if(machine.imports!=9 || machine.nextWrites!=8192 || machine.resultWrites!=64
   || machine.csrReads!=8256 || !machine.unit.drained || machine.pendingWords)return false;
  for(unsigned key=0;key<128;key++){
   if(machine.memory.at(uint32_t(L::Query)+key*4)!=Bits(q[key]) || machine.memory.at(uint32_t(L::Key)+key*4)!=Bits(k[key]))return false;
   for(unsigned v=0;v<64;v++){
    if(machine.memory.at(0x90000+(key*64+v)*4)!=Bits(state[key*128+half*64+v]))return false;
    next[key*128+half*64+v]=machine.memory.at(0xa0000+(key*64+v)*4);
   }
  }
  for(unsigned v=0;v<64;v++){
   if(machine.memory.at(uint32_t(L::Value)+v*4)!=Bits(value[half*64+v]))return false;
   result[half*64+v]=machine.memory.at(uint32_t(L::Output)+v*4);
  }
  totalSteps+=machine.steps;totalOps+=machine.unit.ops;
 }
 // Commit only after BOTH independently initialized local image executions.
 // This copies diagnostic numbers, never a held model's state or ownership.
 for(unsigned i=0;i<16384;i++)state[i]=Float(next[i]);
 for(unsigned i=0;i<128;i++)output[i]=Float(result[i]);
 *steps=totalSteps;*ops=totalOps;return true;
}
