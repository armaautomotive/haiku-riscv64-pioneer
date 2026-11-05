/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_Q4_FPU_COMPACT_METADATA_H
#define PIONEER_Q38_Q4_FPU_COMPACT_METADATA_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error Compact Q4 metadata is an offboard FPU prerequisite,not native or Work authority
#endif
#include "PioneerQ38VectorQ4Metadata.h"
// Caller owns the FULL logical FP32 Dst128..255 scope and completed raw
// byte-prefix Dst16 rows0..15;all destination halves must be initialized.
// Eight groups:coef/min padding4+4 rows for each16-row output half.
// Does not initialize/import memory,publish sources,perform matrix work,
// grant a journal scope,or authenticate finite raw bytes.
struct PioneerQ38Q4FpuCompactMetadata {
 enum : unsigned {Base=128,Rows=128};
 static constexpr unsigned Row(unsigned group,unsigned half){return Base+group*16+half*8;}
private:
 template<class E> static void Products(E& e){
  e.Op(0x90000000);e.Op(0x90000110);
  PioneerQ38VectorAffineMul(e,4,0,6);PioneerQ38VectorAffineMul(e,5,1,7);
  // FPU recipe negates the PRODUCT,including minimum0. The older exact
  // K16 recipe's negative-integer-factor zero rule MUST NOT be substituted.
  PioneerSfpuLoad32(e,2,0x80000000);PioneerSfpuXor(e,7,2);
 }
 template<unsigned Half,unsigned First,unsigned Parity,class E> static void StoreGroups(E& e){
  PioneerSfpuMove(e,0,6);PioneerSfpuMove(e,4,7);
  for(unsigned r=1;r<4;r++)PioneerSfpuLoad32(e,r,0);
  for(unsigned r=5;r<8;r++)PioneerSfpuLoad32(e,r,0);
  e.Op(0x8c000000);
  e.Op(0x72030000|Row(First,Half)|Parity);e.Op(0x72430000|(Row(First,Half)+4)|Parity);
  e.Op(0x72130000|Row(First+1,Half)|Parity);e.Op(0x72530000|(Row(First+1,Half)+4)|Parity);
  e.Op(0x72230000|Row(First+2,Half)|Parity);e.Op(0x72630000|(Row(First+2,Half)+4)|Parity);
  e.Op(0x72330000|Row(First+3,Half)|Parity);e.Op(0x72730000|(Row(First+3,Half)+4)|Parity);
 }
 template<unsigned Half,unsigned Parity,class E> static void ParityBlock(E& e){
  e.Op(0x700d0000|Parity);
  PioneerSfpuMove(e,1,0);PioneerSfpuMove(e,2,0);PioneerSfpuMove(e,3,0);
  for(unsigned r=4;r<8;r++)PioneerSfpuLoad32(e,r,0);
  e.Op(0x8c000000);PioneerQ38VectorQ4Metadata::RawBases(e);
  e.Op(0x72430000|Row(7,Half)|Parity);e.Op(0x72530000|(Row(7,Half)+4)|Parity);
  e.Op(0x700d0000|4|Parity);e.Op(0x701d0000|8|Parity);
  PioneerQ38VectorQ4Metadata::Factors<0>(e);Products(e);StoreGroups<Half,0,Parity>(e);
  e.Op(0x700d0000|12|Parity);e.Op(0x701d0000|4|Parity);e.Op(0x702d0000|8|Parity);
  e.Op(0x70430000|Row(7,Half)|Parity);e.Op(0x70530000|(Row(7,Half)+4)|Parity);
  PioneerQ38VectorQ4Metadata::Factors<4>(e);Products(e);StoreGroups<Half,4,Parity>(e);
 }
public:
 template<unsigned Half,class E> static void Decode(E& e){
  static_assert(Half<2,"two original16-output metadata prefixes only");
  ParityBlock<Half,0>(e);ParityBlock<Half,2>(e);e.Drain();
 }
};
#endif
