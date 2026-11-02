/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_WIDE_DELTA_INSTRUCTION_MODEL_H
#define PIONEER_Q38_WIDE_DELTA_INSTRUCTION_MODEL_H
#define PIONEER_Q38_DELTA_VECTOR_MODEL_ONLY
#include "pioneer_q38_delta_vector_test.cpp"
// TEST ONLY instruction decoder. Inherits the unchanged Dst lane mapping,
// defined-row checks, stores, load-immediates and strict MAD latency model.
// New instructions decode their own operand bits;no expected model product
// is supplied to the processor. Models only enabled32-lane direct forms.
struct WideDeltaUnit:DeltaVectorMachine {
 unsigned splitAnds=0,wideMads=0;
 void Op(uint32_t op) {
  const unsigned code=op>>24;
  if(code!=0x7e && code!=0x84 && code!=0x85 && code!=0x7c){DeltaVectorMachine::Op(op);return;}
  if(latency)throw OrderingFault{};
  assert(enabled);ops++;drained=false;
  if(code==0x7e){
   const unsigned b=(op>>12)&15,c=(op>>8)&15,d=(op>>4)&15;
   assert(b<8 && c<8 && d<8 && (op&15)==1 && !(op&0x00ff0000));
   for(unsigned lane=0;lane<32;lane++)lr[d][lane]=lr[b][lane]&lr[c][lane];
   splitAnds++;return;
  }
  if(code==0x7c){
   const unsigned c=(op>>8)&15,d=(op>>4)&15;
   assert(c<8 && d<8 && !(op&0x00fff000) && !(op&15));
   for(unsigned lane=0;lane<32;lane++)lr[d][lane]=lr[c][lane];return;
  }
  const unsigned a=(op>>16)&15,b=(op>>12)&15,c=(op>>8)&15,d=(op>>4)&15,mod=op&15;
  assert((a<8 || (code==0x85 && a==10)) && b<8 && c<8 && d<8 && !(op&0x00f00000) && !(mod&~2U));
  for(unsigned lane=0;lane<32;lane++){
   const uint32_t av=a==10?0x3f800000U:lr[a][lane],cv=lr[c][lane]^((mod&2)?0x80000000U:0);
   lr[d][lane]=PioneerBlackholeFmaBits(av,lr[b][lane],cv);
  }
  if(code==0x84)wideMads++;
  latency=1;
 }
};
#endif
