/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_Q4_FPU_HALF_STATIC_ISSUE_COMPUTE_H
#define PIONEER_Q38_Q4_FPU_HALF_STATIC_ISSUE_COMPUTE_H
typedef __UINT32_TYPE__ uint32_t;
#include "PioneerQ38Q4FpuStaticIssueCompute.h"

// Bounded-size scheduling experiment: retain static address construction for
// each decode pass, but share its code between the two halves. This changes
// only scalar RV32 issue work; the emitted Tensix stream is intended to remain
// byte-for-byte identical. Offline-only until measured and separately gated.
struct PioneerQ38Q4FpuHalfStaticIssueCompute:PioneerQ38Q4FpuStaticIssueCompute {
 using B=PioneerQ38Q4FpuBlock;using M=PioneerQ38Q4FpuCompactMetadata;
private:
 template<bool Upper,class E>
 static __attribute__((noinline)) void PassHalf(E& e,unsigned group,unsigned half,unsigned parity){
  const unsigned row=M::Row(group,half);
  e.Op(0x70030000|row|parity);e.Op(0x70430000|(row+4)|parity);
  PioneerSfpuMove(e,1,0);PioneerSfpuMove(e,2,0);PioneerSfpuMove(e,3,0);
  PioneerSfpuMove(e,5,4);PioneerSfpuMove(e,6,4);PioneerSfpuMove(e,7,4);
  e.Op(0x8c000000);PioneerSfpuMove(e,1,0);PioneerSfpuMove(e,2,4);
  PioneerSfpuLoad32(e,4,15);PioneerSfpuLoad32(e,5,1);PioneerSfpuLoad32(e,6,0x7fff);PioneerSfpuLoad32(e,7,0xffff0000);
  for(unsigned face=half*2;face<half*2+2;face++)
   for(unsigned block=0;block<16;block+=8){
    const unsigned at=face*16+block;
    e.Op(0x700d0000|at|parity);Nibble<Upper>(e);
    e.Op(0x72030000|(64+at)|parity);
    e.Op(0x700d0000|(at+4)|parity);Nibble<Upper>(e);
    e.Op(0x72030000|(68+at)|parity);
   }
 }
public:
 template<class E> static bool Group(E& e,unsigned buffer,unsigned group){
  if(buffer>=2 || group>=8)return false;
  const unsigned offset=buffer*0x10000;
  e.Op(0x3700000f);B::FloatUnpack(e,16);
  e.Config(1,0x203c8000);e.Config(64,0x10001e);e.Config(65,16);e.Config(72,14);e.Op(0x5e23fc00);e.Drain();
  LoadFace<0>(e,offset,group);LoadFace<1>(e,offset,group);LoadFace<2>(e,offset,group);LoadFace<3>(e,offset,group);
  e.Op(0xb2050004);e.Drain();e.Config(1,0x60aa0000);e.Config(124,(B::Input+offset+group*B::InputStride)/16-1);
  e.Op(0x5e407c00);e.Op(0x5160000b);e.Op(0x5460000f);e.Op(0x428000c1);e.Drain();e.Op(0x3700000f);e.Op(0x8a000002);
  for(unsigned half=0;half<2;half++){
   if(group&1){PassHalf<true>(e,group,half,0);PassHalf<true>(e,group,half,2);}
   else{PassHalf<false>(e,group,half,0);PassHalf<false>(e,group,half,2);}
  }
  TransposeRow<0>(e);TransposeRow<1>(e);TransposeRow<2>(e);TransposeRow<3>(e);
  TransposeRow<4>(e);TransposeRow<5>(e);TransposeRow<6>(e);TransposeRow<7>(e);
  TransposeRow<8>(e);TransposeRow<9>(e);TransposeRow<10>(e);TransposeRow<11>(e);
  TransposeRow<12>(e);TransposeRow<13>(e);TransposeRow<14>(e);TransposeRow<15>(e);
  e.Drain();B::MatrixFace<0>(e);B::MatrixFace<1>(e);B::MatrixFace<2>(e);B::MatrixFace<3>(e);e.Drain();return true;
 }
};
#endif
