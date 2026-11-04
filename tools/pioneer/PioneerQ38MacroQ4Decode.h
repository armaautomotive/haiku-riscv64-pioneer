/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MACRO_Q4_DECODE_H
#define PIONEER_Q38_MACRO_Q4_DECODE_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error Macro Q4 is a separate shader prerequisite,not a native/Work permission
#endif
#include "PioneerSfpuDivide.h"
#include "PioneerQ38NativeOperand.h"

// Blackhole SFPLOADMACRO: load an ORIGINAL unsigned byte and schedule its
// exact nibble operation on the simple sub-unit in the following cycle.
// Alternate LReg0/1. Each explicit native store follows its load by TWO
// SFPU instructions,so it cannot read before the scheduled operation.
// No macro stores/address modifiers,backdoor template writes,packer,new
// Dst/L1 storage,raw-byte reuse,approximation or changed matrix order.
// ISA:BlackholeA0/.../SFPLOADMACRO.md;WormholeB0/.../SFPCONFIG.md.
struct PioneerQ38MacroQ4Decode {
 static constexpr uint32_t LowTemplate=0x7e004001; // VB4 mask;macro substitutes VC/VD
 static constexpr uint32_t HighTemplate=0x7affc001; // logical >>4;macro substitutes VC/VD
 template<class E> static __attribute__((noinline)) void Setup(E& e) {
  e.Drain();e.Op(0x8a000002);
  PioneerSfpuLoad32(e,0,LowTemplate);e.Op(0x91000000); // explicit template0 from LReg0
  PioneerSfpuLoad32(e,0,HighTemplate);e.Op(0x91000010); // explicit template1
  e.Op(0x91000441);e.Op(0x91000551); // sequences0/1:ONLY simple,delay0,template0/1
  e.Op(0x91000061);e.Op(0x91000071); // sequences2/3 inert
  e.Op(0x91000081); // misc:elapsed CYCLES,not instruction-count waits
  e.Drain();
 }
 template<unsigned Block,bool Upper,class E> static inline void Pair(E& e) {
  static_assert(Block<16 && !(Block&3),"original four-row chunks only");
  constexpr unsigned macro=Upper?(1U<<22):0;
  constexpr unsigned row=128+Block+(Block>=8?8:0);
  e.Op(0x930d0000|macro|Block);
  e.Op(0x931d0000|macro|Block|2);
  e.Op(0x72050000|row);
  e.Op(0x72150000|row|2);
 }
 template<bool Upper,class E> static inline void Chunks(E& e) {
  Pair<0,Upper>(e);Pair<4,Upper>(e);Pair<8,Upper>(e);Pair<12,Upper>(e);
 }
 template<class E> static __attribute__((noinline)) bool Decode(E& e,unsigned panel) {
  if(panel>=16)return false;
  if(!(panel&2))PioneerSfpuLoad32(e,4,15); // affine may clobber4 between panels
  if(panel&2)Chunks<true>(e);else Chunks<false>(e);
  e.Op(0x08002040);e.Op(0x08082044);e.Op(0x08102048);e.Op(0x0818204c);
  return true; // caller STILL owes the original final full drain
 }
 template<class E> static __attribute__((noinline)) void Finish(E& e) {
  e.Drain();
  e.Op(0x91000041);e.Op(0x91000051);e.Op(0x91000061);e.Op(0x91000071);
  e.Op(0x91000081);e.Drain(); // schedules inert;NOT a claim to restore unknown original templates
 }
};
#endif
