/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_Q4_FPU_PAIR_ISSUE_COMPUTE_H
#define PIONEER_Q38_Q4_FPU_PAIR_ISSUE_COMPUTE_H
#include "PioneerQ38Q4FpuCompactStreamCompute.h"
// Experimental issue scheduling ONLY. The expanded Tensix word stream,
// arithmetic, metadata, drains and memory scope must equal the old stream.
// Pair two adjacent chunks so RV32 amortizes address/loop construction.
// No replay slots, new formats, host products or relaxed completion checks.
struct PioneerQ38Q4FpuPairIssueCompute:PioneerQ38Q4FpuCompactStreamCompute {
private:
 template<bool Upper,class E> static __attribute__((always_inline)) inline void Nibble(E& e){
  // Exactly the original ten words. Spell the six-word round here so -Os
  // does not outline it into a call for each half of the paired loop.
  if(Upper)PioneerSfpuShift(e,0,-4);else PioneerSfpuAnd(e,0,0,4);
  e.Op(0x90000000);e.Op(0x84010200);e.Op(0x8f000000);
  PioneerSfpuMove(e,3,0);PioneerSfpuShift(e,3,-16);
  PioneerSfpuAnd(e,3,3,5);PioneerSfpuAdd(e,0,6);PioneerSfpuAdd(e,0,3);
  PioneerSfpuAnd(e,0,0,7);
 }
 template<bool Upper,class E> static __attribute__((noinline)) void Decode(E& e,unsigned group){
  for(unsigned half=0;half<2;half++)for(unsigned parity=0;parity<=2;parity+=2){
   const unsigned row=M::Row(group,half);
   e.Op(0x70030000|row|parity);e.Op(0x70430000|(row+4)|parity);
   PioneerSfpuMove(e,1,0);PioneerSfpuMove(e,2,0);PioneerSfpuMove(e,3,0);
   PioneerSfpuMove(e,5,4);PioneerSfpuMove(e,6,4);PioneerSfpuMove(e,7,4);
   e.Op(0x8c000000);PioneerSfpuMove(e,1,0);PioneerSfpuMove(e,2,4);
   PioneerSfpuLoad32(e,4,15);PioneerSfpuLoad32(e,5,1);PioneerSfpuLoad32(e,6,0x7fff);PioneerSfpuLoad32(e,7,0xffff0000);
   for(unsigned face=half*2;face<half*2+2;face++)for(unsigned block=0;block<16;block+=8){
    const unsigned at=face*16+block;
    e.Op(0x700d0000|at|parity);
    Nibble<Upper>(e);
    e.Op(0x72030000|(64+at)|parity);
    e.Op(0x700d0000|(at+4)|parity);
    Nibble<Upper>(e);
    e.Op(0x72030000|(68+at)|parity);
   }
  }
 }
public:
 template<class E> static bool Group(E& e,unsigned buffer,unsigned group){
  if(buffer>=2 || group>=8)return false; // ZERO IO, unchanged admission.
  const unsigned offset=buffer*0x10000;
  e.Op(0x3700000f);B::FloatUnpack(e,16);
  e.Config(1,0x203c8000);e.Config(64,0x10001e);e.Config(65,16);e.Config(72,14);e.Op(0x5e23fc00);e.Drain();
  for(unsigned face=0;face<4;face++){
   e.Config(76,(B::Raw+offset+((group/2)*4+face)*B::RawStride)/16-1);
   e.Config(84,0x00400000+(face*16+4)*16);e.Op(face==3?0x420000c1:0x42000081);e.Drain();
  }
  e.Op(0xb2050004);e.Drain();e.Config(1,0x60aa0000);e.Config(124,(B::Input+offset+group*B::InputStride)/16-1);
  e.Op(0x5e407c00);e.Op(0x5160000b);e.Op(0x5460000f);e.Op(0x428000c1);e.Drain();e.Op(0x3700000f);e.Op(0x8a000002);
  if(group&1)Decode<true>(e,group);else Decode<false>(e,group);
  for(unsigned row=0;row<64;row+=4)e.Op(0x08002000|(row<<17)|(64+row));
  e.Drain();B::MatrixFace<0>(e);B::MatrixFace<1>(e);B::MatrixFace<2>(e);B::MatrixFace<3>(e);e.Drain();return true;
 }
};
#endif
