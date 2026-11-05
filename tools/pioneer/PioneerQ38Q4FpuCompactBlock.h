/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_Q4_FPU_COMPACT_BLOCK_H
#define PIONEER_Q38_Q4_FPU_COMPACT_BLOCK_H
#include "PioneerQ38Q4FpuBlock.h"
#include "PioneerQ38Q4FpuCompactMetadata.h"
// Separate OFFBOARD K256 pipeline. Reuses the ORIGINAL quant/input layout,
// four-phase FPU arithmetic and bank handoffs,not its expanded metadata.
// Caller supplies finite ORIGINAL prefix bytes at Metadata+half*320 with
// private zero64B headers;zero buffer/input/raw ownership stays explicit.
// Logical FP32 Dst128..255 is a NEW scope,NOT an existing native lease grant.
struct PioneerQ38Q4FpuCompactBlock {
 using B=PioneerQ38Q4FpuBlock;using M=PioneerQ38Q4FpuCompactMetadata;
 enum : unsigned {Metadata=B::Metadata,MetadataStride=320};
private:
 template<unsigned Half,class E> static void Import(E& e){
  B::FloatUnpack(e,16);
  e.Config(1,0xa03c8000);e.Config(64,0x10001e);e.Config(72,14);
  e.Config(84,0x00400040);e.Config(76,(Metadata+Half*MetadataStride)/16-1);
  e.Op(0x42000081);e.Drain();
  e.Op(0xb2050004);e.Op(0x3700000f);e.Op(0x8a000002);e.Drain();
  M::Decode<Half>(e);
 }
 template<bool Upper,class E> static __attribute__((noinline)) void Decode(E& e,unsigned group){
  for(unsigned half=0;half<2;half++)for(unsigned parity=0;parity<=2;parity+=2){
   const unsigned row=M::Row(group,half);
   e.Op(0x70030000|row|parity);e.Op(0x70430000|(row+4)|parity);
   PioneerSfpuMove(e,1,0);PioneerSfpuMove(e,2,0);PioneerSfpuMove(e,3,0);
   PioneerSfpuMove(e,5,4);PioneerSfpuMove(e,6,4);PioneerSfpuMove(e,7,4);
   e.Op(0x8c000000);PioneerSfpuMove(e,1,0);PioneerSfpuMove(e,2,4);
   PioneerSfpuLoad32(e,4,15);PioneerSfpuLoad32(e,5,1);PioneerSfpuLoad32(e,6,0x7fff);PioneerSfpuLoad32(e,7,0xffff0000);
   for(unsigned face=half*2;face<half*2+2;face++)for(unsigned block=0;block<16;block+=4){
    e.Op(0x700d0000|(face*16+block)|parity);
    PioneerQ38Q4FpuDecode<E,Upper>(e);
    e.Op(0x72030000|(64+face*16+block)|parity);
   }
  }
 }
public:
 template<class E> static void Run(E& e){
  B::Init(e);B::FloatUnpack(e,16);e.Config(76,B::Zero/16-1);
  // B::Init defines128..143;define BOTH physical halves of the remaining
  // metadata scope once per job,not per group or partial store.
  for(unsigned row=144;row<256;row+=16){e.Config(84,0x00400000+(row+4)*16);e.Op(0x42000091);e.Drain();}
  Import<0>(e);Import<1>(e);
  for(unsigned group=0;group<8;group++){
   e.Op(0x3700000f);B::FloatUnpack(e,16);
   e.Config(1,0x203c8000);e.Config(64,0x10001e);e.Config(65,16);e.Config(72,14);e.Op(0x5e23fc00);e.Drain();
   for(unsigned face=0;face<4;face++){
    e.Config(76,(B::Raw+((group/2)*4+face)*B::RawStride)/16-1);
    e.Config(84,0x00400000+(face*16+4)*16);e.Op(face==3?0x420000c1:0x42000081);e.Drain();
   }
   e.Op(0xb2050004);e.Drain();e.Config(1,0x60aa0000);e.Config(124,(B::Input+group*B::InputStride)/16-1);
   e.Op(0x5e407c00);e.Op(0x5160000b);e.Op(0x5460000f);e.Op(0x428000c1);e.Drain();e.Op(0x3700000f);e.Op(0x8a000002);
   if(group&1)Decode<true>(e,group);else Decode<false>(e,group);
   for(unsigned row=0;row<64;row+=4)e.Op(0x08002000|(row<<17)|(64+row));
   e.Drain();B::MatrixFace<0>(e);B::MatrixFace<1>(e);B::MatrixFace<2>(e);B::MatrixFace<3>(e);e.Drain();
  }
 }
};
#endif
