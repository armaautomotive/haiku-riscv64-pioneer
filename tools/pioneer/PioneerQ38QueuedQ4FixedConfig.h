/* SPDX-License-Identifier: MIT */
#include "PioneerQ38QueuedQ4FixedConfigRole.h"
#ifndef PIONEER_Q38_QUEUED_Q4_FIXED_CONFIG_H
#define PIONEER_Q38_QUEUED_Q4_FIXED_CONFIG_H
#include "PioneerQ38QueuedQ4Block.h"
// SPECIALIZED to the fixed16-panel Dot+register-only-Affine block.
// Begin writes EVERY byte of its six fields,then binds that program's
// final expected configuration ONCE. Config writes both low bytes after
// checking the immutable high half;it does not update a per-call cache.
// This is NOT a general arbitrary-command transaction or a mapping grant.
// Caller must run the entire fixed sequence,then real Drain and Verify.
struct PioneerQ38QueuedQ4FixedConfig:PioneerQ38QueuedQ4Block {
 template<class E> __attribute__((always_inline))
 bool Begin(E& e,unsigned firstInput,unsigned firstPrimary,unsigned finalInput,unsigned finalEscape){
  if(phase!=Idle || seen)return false;
  if(((firstInput|firstPrimary|finalInput|finalEscape)&0xffff0000U) || firstInput>finalInput){phase=Failed;return false;}
  if(!PioneerQ38QueuedQ4Block::Begin())return false;
  // Completed metadata/immutable planes precede this call. No unpack or
  // other config consumer may be inserted between these seeds and Dot.
  PioneerQ38QueuedQ4Block::Config(e,50,0x181);
  PioneerQ38QueuedQ4Block::Config(e,64,0x01000017);
  PioneerQ38QueuedQ4Block::Config(e,72,0x27);
  PioneerQ38QueuedQ4Block::Config(e,76,firstPrimary);
  PioneerQ38QueuedQ4Block::Config(e,84,0x00400040);
  PioneerQ38QueuedQ4Block::Config(e,124,firstInput);
  expected[1]=0x17f;expected[2]=0x0100001f;expected[4]=15;
  expected[6]=finalEscape;expected[7]=0x00400140;expected[9]=finalInput;
  return true;
 }
 template<class E> __attribute__((always_inline))
 bool Config(E& e,unsigned index,unsigned value){
  if(phase!=Recording)return false;
  unsigned high=0;
  switch(index){
   case 50:case 72:case 76:case 124:break;
   case 64:high=0x100;break;
   case 84:high=0x40;break;
   default:phase=Failed;return false;
  }
  if((value>>16)!=high){phase=Failed;return false;}
  e.Op(0xb3ff0000U|((value&255)<<8)|index);
  e.Op(0xb4ff0000U|(((value>>8)&255)<<8)|index);
  return true;
 }
};
#endif
