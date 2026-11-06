/* SPDX-License-Identifier: MIT */
#include "PioneerQ38Q4FactoredTile.h" // exact separate-role checks
#ifndef PIONEER_Q38_Q4_FACTORED_COMPUTE_H
#define PIONEER_Q38_Q4_FACTORED_COMPUTE_H
#include "PioneerQ38Q4FpuCompactMetadata.h"
// Complete arithmetic producer,NOT a native owner or model-quality grant.
// Begin once/job;LoadMetadata once per completed K256 buffer;Group adds one
// genuine 32-K contribution. No scalar learned/input/result reads or host
// products. Caller owes original finite bytes,producer visibility,private
// code/L1/Dst ownership and full-K loader/release/return lifetime.
struct PioneerQ38Q4FactoredCompute {
 using F=PioneerQ38Q4FactoredTile;using M=PioneerQ38Q4FpuCompactMetadata;
 enum : unsigned {Metadata=0x94000,Zero=0x97000,Accumulator=80};
 template<class IO> static void Constants(IO& io){
  F::Constants(io);
  for(unsigned a=Zero-64;a<Zero+1024;a+=4)io.Write(a,0);
 }
private:
 template<class E> static void FloatUnpack(E& e){
  e.Op(0xb2050000);e.Drain();e.Config(0,0x42b6);e.Config(1,0x20200000);
  e.Config(64,0x100010);e.Config(65,16);e.Config(66,1);e.Config(67,0);
  e.Config(70,8);e.Config(71,0);e.Config(72,0);e.Config(73,0x13);
  e.Config(86,0x100010);e.Op(0x5e23fc00);e.Op(0x5120000a);e.Op(0x5420000f);e.Drain();
 }
public:
 template<class E> static void Begin(E& e){
  F::Begin(e);e.Op(0x3700000f);e.Op(0x10180000);e.Drain();FloatUnpack(e);
  e.Config(76,Zero/16-1);
  // Unpack defines EVERY column of BOTH physical FP32 halves before any
  // partial SFPU stores. Matrix scratch is independently invalidated.
  e.Config(84,0x00400000+(Accumulator+4)*16);e.Op(0x42000091);e.Drain();
  for(unsigned row=128;row<256;row+=16){
   e.Config(84,0x00400000+(row+4)*16);e.Op(0x42000091);e.Drain();
  }
 }
 template<class E> static bool LoadMetadata(E& e,unsigned buffer){
  if(buffer>=2)return false;
  for(unsigned half=0;half<2;half++){
   e.Op(0x3700000f);FloatUnpack(e);
   e.Config(1,0xa03c8000);e.Config(64,0x10001e);e.Config(72,14);
   e.Config(84,0x00400040);e.Config(76,(Metadata+buffer*0x10000+half*320)/16-1);
   e.Op(0x42000081);e.Drain();e.Op(0xb2050004);e.Op(0x8a000002);e.Drain();
   if(!M::DecodeDynamic(e,half))return false;
  }
  return true;
 }
 template<class E> static bool Group(E& e,unsigned buffer,unsigned group){
  if(buffer>=2 || group>=8)return false; // ZERO IO
  if(!F::PackCodes(e,group,buffer) || !F::Matrix<true>(e,F::Ones,group,buffer))return false;
  // Broadcast MVMUL only guarantees the FIRST output row. SFPU must not
  // load its undefined padding rows. LReg15 is the architectural 2*lane
  // constant;enable flags then select lanes0..7 by signed(2*lane-16)<0.
  e.Op(0x8a001002);PioneerSfpuMove(e,7,15);PioneerSfpuAddI(e,7,-16);e.Op(0x7b000700);
  e.Op(0x70130020); // genuine on-card ones dot -> sum in r1,not scalar IO
  if(!F::Matrix<true>(e,F::Native,group,buffer))return false; // r1 preserved
  for(unsigned half=0;half<2;half++)for(unsigned parity=0;parity<=2;parity+=2){
   const unsigned row=M::Row(group,half),acc=Accumulator+half*8;
   e.Op(0x70230000|row|parity);e.Op(0x70330000|(row+4)|parity);
   e.Op(0x70030000|(half?40:32)|parity);e.Op(0x70430000|acc|parity);
   PioneerQ38Q4FactoredBfp8::GroupAffine(e);
   e.Op(0x72430000|acc|parity);
  }
  e.Op(0x8a000002);e.Drain();return true;
 }
};
#endif
