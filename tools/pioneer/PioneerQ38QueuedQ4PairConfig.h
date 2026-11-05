/* SPDX-License-Identifier: MIT */
#include "PioneerQ38QueuedQ4PairConfigRole.h"
#ifndef PIONEER_Q38_QUEUED_Q4_PAIR_CONFIG_H
#define PIONEER_Q38_QUEUED_Q4_PAIR_CONFIG_H
#include "PioneerQ38QueuedQ4Block.h"
// First field access in EVERY block writes all4 bytes. Later calls ALWAYS
// write the low2 bytes;one branch emits both high bytes only if changed.
// No assumed initial configuration,GPR borrowing,wait/readback removal.
struct PioneerQ38QueuedQ4PairConfig:PioneerQ38QueuedQ4Block {
 template<class E> __attribute__((always_inline))
 bool Config(E& e,unsigned index,unsigned value){
  if(phase!=Recording)return false;
  const unsigned slot=Slot(index);
  if(slot==Fields){phase=Failed;return false;}
  const unsigned changed=(seen&(1U<<slot))?(expected[slot]^value):~0U;
  expected[slot]=value;seen|=1U<<slot;
  e.Op(0xb3ff0000U|((value&255)<<8)|index);
  e.Op(0xb4ff0000U|(((value>>8)&255)<<8)|index);
  if(changed&0xffff0000U){
   e.Op(0xb5ff0000U|(((value>>16)&255)<<8)|index);
   e.Op(0xb6ff0000U|((value>>24)<<8)|index);
  }
  return true;
 }
};
#endif
