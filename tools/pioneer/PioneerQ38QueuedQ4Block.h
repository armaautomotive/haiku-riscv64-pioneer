/* SPDX-License-Identifier: MIT */
#include "PioneerQ38QueuedQ4BlockRole.h"
#ifndef PIONEER_Q38_QUEUED_Q4_BLOCK_H
#define PIONEER_Q38_QUEUED_Q4_BLOCK_H
#include "PioneerQ38ExactQ4TwoProduct.h"
// NEW offboard block-queue experiment. No lifetime,installation or speed grant.
// Exclusive T0/Config0,immutable block inputs+escape planes and normal/fault
// ownership are CALLER obligations. GPU-read operands stay immutable from
// Begin through End;CPU-private stack/sum-cache words are not GPU operands.
// Backend waits remain:only repeated RISCV ACK polling is removed inside a
// block. The existing bounded real Drain is REQUIRED before Verify/CPU reuse.
struct PioneerQ38QueuedQ4Block {
 enum {EscapeBase=0x9c610,EscapeStride=80,EscapeBytes=1280,WaitAll=0xa2ff9816U,Fields=10};
 unsigned expected[Fields]={},seen=0;
 enum Phase{Idle,Recording,Failed};Phase phase=Idle;
 static __attribute__((always_inline)) inline unsigned Index(unsigned slot){
  switch(slot){case 0:return 1;case 1:return 50;case 2:return 64;
   case 3:return 65;case 4:return 72;case 5:return 73;case 6:return 76;
   case 7:return 84;case 8:return 86;case 9:return 124;default:return ~0U;}
 }
 __attribute__((always_inline)) bool Begin(){if(phase!=Idle || seen)return false;phase=Recording;return true;}
 static __attribute__((always_inline)) inline unsigned Slot(unsigned index){
  switch(index){case 1:return 0;case 50:return 1;case 64:return 2;
   case 65:return 3;case 72:return 4;case 73:return 5;case 76:return 6;
   case 84:return 7;case 86:return 8;case 124:return 9;default:return Fields;}
 }
 template<class E> __attribute__((always_inline))
 bool Config(E& e,unsigned index,unsigned value){
  if(phase!=Recording)return false;
  const unsigned slot=Slot(index);
  if(slot==Fields){phase=Failed;return false;}
  expected[slot]=value;seen|=1U<<slot;
  // RMWCIB0..3:full byte masks,one immutable word assembled completely
  // before any following consumer. No GPR borrowing or writes to global,
  // reset,PRNG,prefetch or thread configuration. Config0 selected by owner.
  e.Op(0xb3ff0000U|((value&255)<<8)|index);
  e.Op(0xb4ff0000U|(((value>>8)&255)<<8)|index);
  e.Op(0xb5ff0000U|(((value>>16)&255)<<8)|index);
  e.Op(0xb6ff0000U|((value>>24)<<8)|index);
  return true;
 }
 template<class E> static __attribute__((always_inline)) void Wait(E& e){
  // Blackhole:ALL9 blocked classes;wait on unpack0/1,FPU,SFPU,CFG.
  // Unlike B5-only ACK fencing,this also blocks RMWCIB and following math.
  // No claim of a local timeout if a coprocessor stops completing.
  e.Op(WaitAll);
 }
 template<class IO> __attribute__((always_inline)) bool Verify(IO& io){
  if(phase!=Recording || !seen)return false;
  phase=Failed; // readback uncertainty cannot be retried or hidden
  for(unsigned slot=0;slot<Fields;slot++)if(seen&(1U<<slot)){
   unsigned observed=0;
   if(!io.ReadConfig(Index(slot),observed) || observed!=expected[slot])return false;
  }
  seen=0;phase=Idle;return true;
 }
 // Exact original two-product arithmetic/UNPACK/bank release sequence.
 // All16 escape planes were prepared BEFORE queueing this block;there are
 // no RISCV L1 stores here which could overwrite an in-flight unpack.
 template<class E> static __attribute__((always_inline)) bool Dot(E& e){
  e.SealData(); // preserve the original CPU-store fence,including sum-cache writes
  e.Op(0x3700000f);e.Op(0x10180000);
  e.Op(0x5e23fc00);e.Op(0x5e41fc00);e.Op(0x5160000b);e.Op(0x5460000f);
  e.Config(64,0x01000017);e.Config(72,0x27);e.Config(50,0x181);
  e.Config(84,0x00400040);e.Config(76,0x90000/16-1);
  e.Op(0x42000081);e.Drain();
  e.Config(64,0x0100001f);e.Config(72,15);e.Config(50,0x17f);
  e.Config(84,0x00400140);e.Config(76,0x90200/16-1);
  e.Op(0x420000c1);e.Op(0x428000c1);e.Drain();
  e.Op(0x26004020);
  e.Op(0x37040201);e.Op(0x37040201);e.Op(0x37000008);
  e.Op(0x26c04020);e.Drain();return true;
 }
 template<class E> struct PanelEmitter {
  E& e;explicit PanelEmitter(E& value):e(value){}
  __attribute__((always_inline)) void Op(unsigned word){
   if(word==0x10180000){e.Op(0x10080004);e.Op(0x10080005);}else e.Op(word);
  }
  __attribute__((always_inline)) void Config(unsigned i,unsigned v){e.Config(i,v);}
  __attribute__((always_inline)) void Drain(){e.Drain();}
  __attribute__((always_inline)) void SealData(){e.SealData();}
 };
};
#endif
