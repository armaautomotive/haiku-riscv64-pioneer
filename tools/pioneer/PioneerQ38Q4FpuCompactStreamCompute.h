/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_Q4_FPU_COMPACT_STREAM_COMPUTE_H
#define PIONEER_Q38_Q4_FPU_COMPACT_STREAM_COMPUTE_H
#include "PioneerQ38Q4FpuCompactBlock.h"
// Same compact math,NEW full-K lifecycle. Begin ONCE per job;LoadMetadata
// ONCE per completed K256 block;Group preserves the accumulator32/40.
// Buffer/finite-provenance/NoC/Dst128..255 ownership remain caller duties.
struct PioneerQ38Q4FpuCompactStreamCompute {
 using B=PioneerQ38Q4FpuBlock;using C=PioneerQ38Q4FpuCompactBlock;using M=PioneerQ38Q4FpuCompactMetadata;
private:
 template<class E> static bool Import(E& e,unsigned buffer,unsigned half){
  B::FloatUnpack(e,16);
  e.Config(1,0xa03c8000);e.Config(64,0x10001e);e.Config(72,14);
  e.Config(84,0x00400040);e.Config(76,(C::Metadata+buffer*0x10000+half*C::MetadataStride)/16-1);
  e.Op(0x42000081);e.Drain();e.Op(0xb2050004);e.Op(0x3700000f);e.Op(0x8a000002);e.Drain();
  return M::DecodeDynamic(e,half);
 }
public:
 template<class E> static void Begin(E& e){
  B::Init(e);B::FloatUnpack(e,16);e.Config(76,B::Zero/16-1);
  for(unsigned row=144;row<256;row+=16){e.Config(84,0x00400000+(row+4)*16);e.Op(0x42000091);e.Drain();}
 }
 template<class E> static bool LoadMetadata(E& e,unsigned buffer){
  if(buffer>=2)return false; // ZERO IO
  for(unsigned half=0;half<2;half++)if(!Import(e,buffer,half))return false;
  return true;
 }
 template<class E> static bool Group(E& e,unsigned buffer,unsigned group){
  if(buffer>=2 || group>=8)return false; // ZERO IO
  const unsigned offset=buffer*0x10000;
  e.Op(0x3700000f);B::FloatUnpack(e,16);
  e.Config(1,0x203c8000);e.Config(64,0x10001e);e.Config(65,16);e.Config(72,14);e.Op(0x5e23fc00);e.Drain();
  for(unsigned face=0;face<4;face++){
   e.Config(76,(B::Raw+offset+((group/2)*4+face)*B::RawStride)/16-1);
   e.Config(84,0x00400000+(face*16+4)*16);e.Op(face==3?0x420000c1:0x42000081);e.Drain();
  }
  e.Op(0xb2050004);e.Drain();e.Config(1,0x60aa0000);e.Config(124,(B::Input+offset+group*B::InputStride)/16-1);
  e.Op(0x5e407c00);e.Op(0x5160000b);e.Op(0x5460000f);e.Op(0x428000c1);e.Drain();e.Op(0x3700000f);e.Op(0x8a000002);
  if(group&1)C::Decode<true>(e,group);else C::Decode<false>(e,group);
  for(unsigned row=0;row<64;row+=4)e.Op(0x08002000|(row<<17)|(64+row));
  e.Drain();B::MatrixFace<0>(e);B::MatrixFace<1>(e);B::MatrixFace<2>(e);B::MatrixFace<3>(e);e.Drain();return true;
 }
};
#endif
