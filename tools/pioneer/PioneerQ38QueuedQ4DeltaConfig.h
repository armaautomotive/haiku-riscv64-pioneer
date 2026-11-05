/* SPDX-License-Identifier: MIT */
#include "PioneerQ38QueuedQ4DeltaConfigRole.h"
#ifndef PIONEER_Q38_QUEUED_Q4_DELTA_CONFIG_H
#define PIONEER_Q38_QUEUED_Q4_DELTA_CONFIG_H
#include "PioneerQ38QueuedQ4Block.h"
// Same transaction/immutable-input/final-ACK/readback obligations as E.
// First access to EVERY field in EVERY block still writes ALL FOUR bytes.
// Only subsequent bytes known unchanged from this transaction may be
// omitted. No assumption about initial hardware values or prior jobs.
struct PioneerQ38QueuedQ4DeltaConfig:PioneerQ38QueuedQ4Block {
 template<class E> __attribute__((always_inline))
 bool Config(E& e,unsigned index,unsigned value){
  if(phase!=Recording)return false;
  const unsigned slot=Slot(index);
  if(slot==Fields){phase=Failed;return false;}
  const unsigned changed=(seen&(1U<<slot))?(expected[slot]^value):~0U;
  expected[slot]=value;seen|=1U<<slot;
  if(changed&0x000000ffU)e.Op(0xb3ff0000U|((value&255)<<8)|index);
  if(changed&0x0000ff00U)e.Op(0xb4ff0000U|(((value>>8)&255)<<8)|index);
  if(changed&0x00ff0000U)e.Op(0xb5ff0000U|(((value>>16)&255)<<8)|index);
  if(changed&0xff000000U)e.Op(0xb6ff0000U|((value>>24)<<8)|index);
  return true;
 }
};
#endif
