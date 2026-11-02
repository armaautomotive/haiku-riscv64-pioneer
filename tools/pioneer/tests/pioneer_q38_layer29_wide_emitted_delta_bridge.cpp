/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_LAYER29_WIDE_EMITTED_DIAGNOSTIC) || PIONEER_Q38_LAYER29_WIDE_EMITTED_DIAGNOSTIC != 1 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(__FAST_MATH__) || defined(PIONEER_Q38_WIDE_DELTA_EXPECTED_OBSERVER_TEST) || defined(PIONEER_Q38_LAYER29_EMITTED_DELTA_DIAGNOSTIC) || defined(PIONEER_Q38_LAYER29_WIDE_REDUCTION_DIAGNOSTIC)
#error Separate HOST emitted wide-delta diagnostic only;no CPU oracle/native/Work or old-image alias
#endif
#define PIONEER_Q38_WIDE_DELTA_INSTRUCTION_TEST 1
#include "pioneer_q38_wide_delta_local_machine.h"
#include "pioneer_q38_layer29_wide_emitted_delta_bridge.h"
#include <cfenv>
bool Q38Layer29WideEmittedDelta(const char* image,const float* q,const float* k,const float* value,
 float decay,float beta,float* state,float* output,uint64_t* steps,uint64_t* ops)
{
 if(!image || !q || !k || !value || !state || !output || !steps || !ops || std::fegetround()!=FE_TONEAREST)return false;
 std::array<uint32_t,16384> next={};std::array<uint32_t,128> result={};uint64_t totalSteps=0,totalOps=0;
 for(unsigned half=0;half<2;half++){
  WideDeltaLocalMachine m(image,0);
  for(unsigned key=0;key<128;key++){
   m.memory[W::Query+key*4]=Bits(q[key]);m.memory[W::Key+key*4]=Bits(k[key]);
   for(unsigned v=0;v<64;v++)m.memory[W::Prior+(key*64+v)*4]=Bits(state[key*128+half*64+v]);
  }
  for(unsigned v=0;v<64;v++)m.memory[W::Value+v*4]=Bits(value[half*64+v]);
  m.memory[W::Decay]=Bits(decay);m.memory[W::Beta]=Bits(beta);
  if(!m.RunWide() || m.imports!=13 || m.deltaWrites!=64 || m.nextWrites!=8192 || m.resultWrites!=64
   || m.csrReads!=8320 || !m.unit.drained || m.pendingWords)return false;
  for(unsigned key=0;key<128;key++){
   if(m.memory.at(W::Query+key*4)!=Bits(q[key]) || m.memory.at(W::Key+key*4)!=Bits(k[key]))return false;
   for(unsigned v=0;v<64;v++){
    if(m.memory.at(W::Prior+(key*64+v)*4)!=Bits(state[key*128+half*64+v]))return false;
    next[key*128+half*64+v]=m.memory.at(W::Next+(key*64+v)*4);
   }
  }
  for(unsigned v=0;v<64;v++){
   if(m.memory.at(W::Value+v*4)!=Bits(value[half*64+v]))return false;
   result[half*64+v]=m.memory.at(W::Output+v*4);
  }
  totalSteps+=m.steps;totalOps+=m.unit.ops;
 }
 // Copy diagnostic numbers ONLY after both complete independent engines.
 // The expected numerical observer is NOT compiled into this TU.
 for(unsigned i=0;i<16384;i++)state[i]=Float(next[i]);
 for(unsigned i=0;i<128;i++)output[i]=Float(result[i]);
 *steps=totalSteps;*ops=totalOps;return true;
}
