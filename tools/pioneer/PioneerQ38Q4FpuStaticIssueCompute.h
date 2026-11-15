/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_Q4_FPU_STATIC_ISSUE_COMPUTE_H
#define PIONEER_Q38_Q4_FPU_STATIC_ISSUE_COMPUTE_H
#include "PioneerQ38Q4FpuCompactStreamCompute.h"
// Offline scheduling candidate: fully unroll the repeated per-group Q4
// decode issue sequence. The emitted Tensix/SFPU stream must be identical;
// only scalar RV32 loop/address work may change. No native/Work selection.
struct PioneerQ38Q4FpuStaticIssueCompute:PioneerQ38Q4FpuCompactStreamCompute {
 using B=PioneerQ38Q4FpuBlock;using M=PioneerQ38Q4FpuCompactMetadata;
protected:
 template<bool Upper,class E> static __attribute__((always_inline)) inline void Nibble(E& e){
  if(Upper)PioneerSfpuShift(e,0,-4);else PioneerSfpuAnd(e,0,0,4);
  e.Op(0x90000000);e.Op(0x84010200);e.Op(0x8f000000);
  PioneerSfpuMove(e,3,0);PioneerSfpuShift(e,3,-16);
  PioneerSfpuAnd(e,3,3,5);PioneerSfpuAdd(e,0,6);PioneerSfpuAdd(e,0,3);
  PioneerSfpuAnd(e,0,0,7);
 }
 template<unsigned Face,unsigned Block,unsigned Parity,bool Upper,class E>
 static __attribute__((always_inline)) inline void Chunk(E& e){
  static_assert(Face<4 && (Block==0 || Block==8) && (Parity==0 || Parity==2),"fixed decode lane");
  constexpr unsigned at=Face*16+Block;
  e.Op(0x700d0000|at|Parity);Nibble<Upper>(e);
  e.Op(0x72030000|(64+at)|Parity);
  e.Op(0x700d0000|(at+4)|Parity);Nibble<Upper>(e);
  e.Op(0x72030000|(68+at)|Parity);
 }
 template<unsigned Half,unsigned Parity,bool Upper,class E>
 static __attribute__((always_inline)) inline void Pass(E& e,unsigned group){
  const unsigned row=M::Row(group,Half);
  e.Op(0x70030000|row|Parity);e.Op(0x70430000|(row+4)|Parity);
  PioneerSfpuMove(e,1,0);PioneerSfpuMove(e,2,0);PioneerSfpuMove(e,3,0);
  PioneerSfpuMove(e,5,4);PioneerSfpuMove(e,6,4);PioneerSfpuMove(e,7,4);
  e.Op(0x8c000000);PioneerSfpuMove(e,1,0);PioneerSfpuMove(e,2,4);
  PioneerSfpuLoad32(e,4,15);PioneerSfpuLoad32(e,5,1);PioneerSfpuLoad32(e,6,0x7fff);PioneerSfpuLoad32(e,7,0xffff0000);
  Chunk<Half*2,0,Parity,Upper>(e);Chunk<Half*2,8,Parity,Upper>(e);
  Chunk<Half*2+1,0,Parity,Upper>(e);Chunk<Half*2+1,8,Parity,Upper>(e);
 }
 template<unsigned Face,class E>
 static __attribute__((always_inline)) inline void LoadFace(E& e,unsigned offset,unsigned group){
  e.Config(76,(B::Raw+offset+((group/2)*4+Face)*B::RawStride)/16-1);
  e.Config(84,0x00400000+(Face*16+4)*16);
  e.Op(Face==3?0x420000c1:0x42000081);e.Drain();
 }
 template<unsigned Row,class E>
 static __attribute__((always_inline)) inline void TransposeRow(E& e){
  static_assert(Row<16,"fixed transpose row");
  constexpr unsigned at=Row*4;
  e.Op(0x08002000|(at<<17)|(64+at));
 }
public:
 template<class E> static bool Group(E& e,unsigned buffer,unsigned group){
  if(buffer>=2 || group>=8)return false;
  const unsigned offset=buffer*0x10000;
  e.Op(0x3700000f);B::FloatUnpack(e,16);
  e.Config(1,0x203c8000);e.Config(64,0x10001e);e.Config(65,16);e.Config(72,14);e.Op(0x5e23fc00);e.Drain();
  LoadFace<0>(e,offset,group);LoadFace<1>(e,offset,group);
  LoadFace<2>(e,offset,group);LoadFace<3>(e,offset,group);
  e.Op(0xb2050004);e.Drain();e.Config(1,0x60aa0000);e.Config(124,(B::Input+offset+group*B::InputStride)/16-1);
  e.Op(0x5e407c00);e.Op(0x5160000b);e.Op(0x5460000f);e.Op(0x428000c1);e.Drain();e.Op(0x3700000f);e.Op(0x8a000002);
  if(group&1){
   Pass<0,0,true>(e,group);Pass<0,2,true>(e,group);Pass<1,0,true>(e,group);Pass<1,2,true>(e,group);
  }else{
   Pass<0,0,false>(e,group);Pass<0,2,false>(e,group);Pass<1,0,false>(e,group);Pass<1,2,false>(e,group);
  }
  TransposeRow<0>(e);TransposeRow<1>(e);TransposeRow<2>(e);TransposeRow<3>(e);
  TransposeRow<4>(e);TransposeRow<5>(e);TransposeRow<6>(e);TransposeRow<7>(e);
  TransposeRow<8>(e);TransposeRow<9>(e);TransposeRow<10>(e);TransposeRow<11>(e);
  TransposeRow<12>(e);TransposeRow<13>(e);TransposeRow<14>(e);TransposeRow<15>(e);
  e.Drain();B::MatrixFace<0>(e);B::MatrixFace<1>(e);B::MatrixFace<2>(e);B::MatrixFace<3>(e);e.Drain();return true;
 }
};
#endif
