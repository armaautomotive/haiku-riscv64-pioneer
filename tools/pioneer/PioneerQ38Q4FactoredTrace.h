/* SPDX-License-Identifier: MIT */
// DIAGNOSTIC observations only. Never included by an inference/driver role.
#if !defined(PIONEER_Q38_Q4_FACTORED_TRACE_DIAGNOSTIC) || PIONEER_Q38_Q4_FACTORED_TRACE_DIAGNOSTIC!=1 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error Separate factored Q4 TRACE diagnostic only;never Work/native/reference
#endif
#if defined(PIONEER_ATTENTION_FREESTANDING) && (!defined(PIONEER_Q38_FACTORED_Q4_TRACE_BUILD) || PIONEER_Q38_FACTORED_Q4_TRACE_BUILD!=1 || !defined(__riscv) || __riscv_xlen!=32)
#error Freestanding TRACE requires its distinct RV32 BUILD role
#endif
#ifndef PIONEER_Q38_Q4_FACTORED_TRACE_H
#define PIONEER_Q38_Q4_FACTORED_TRACE_H
#include "PioneerQ38Q4FactoredCompute.h"

// SAME arithmetic issue sequence as Compute::Group, with scalar diagnostic
// observations ONLY at completed PACK/FPU/SFPU boundaries. The issue-stream
// correspondence test is required. Reads can hide a timing bug: a passing
// trace is NOT a fix or a throughput result. No native owner is granted here.
// Fixed K5120/32-output job; preserve compressed weights and both buffers.
// Result words: exact final32;20 block hashes;4 stage hashes;8 first-group
// words. Hashes locate failures, NEVER replace exact final-output acceptance.
class PioneerQ38Q4FactoredTrace {
 using C=PioneerQ38Q4FactoredCompute;using F=PioneerQ38Q4FactoredTile;
 using M=PioneerQ38Q4FpuCompactMetadata;
 static constexpr unsigned Seed=2166136261U;
 unsigned blockHashes[20]={},stageHashes[4]={Seed,Seed,Seed,Seed},first[8]={};
 // Word-sized state matches the fixed word-journaled diagnostic stack.
 unsigned completedBlocks=0,nextGroup=0,blockHash=Seed,published=0;
 // Word-wise diagnostic fingerprint;no RV32 multiply/runtime capability.
 static unsigned Mix(unsigned h,unsigned v){h+=v;h+=h<<10;return h^(h>>6);}
 void Record(unsigned stage,unsigned v){stageHashes[stage]=Mix(stageHashes[stage],v);blockHash=Mix(Mix(blockHash,stage),v);}
 template<class IO> static unsigned Dst(IO& io,unsigned row,unsigned column){return io.Read(0xffbd8000+row*64+column*4);}
 template<class IO> void Packed(IO& io){
  for(unsigned a=F::Native-16;a<F::Native+F::TileBytes;a+=4)Record(0,io.Read(a));
  if(!completedBlocks && !nextGroup){first[0]=io.Read(F::Native+64);first[1]=io.Read(F::Native+80);}
 }
 template<class IO> void Ones(IO& io){
  const unsigned sum=Dst(io,32,0);Record(1,sum);
  if(!completedBlocks && !nextGroup)first[2]=sum;
 }
 template<class IO> void DotAndMetadata(IO& io,unsigned group){
  for(unsigned row=0;row<32;row++){
   const unsigned dot=Dst(io,row<16?32:40,row%16);Record(1,dot);
   if(!completedBlocks && !nextGroup && !row)first[3]=dot;
  }
  for(unsigned row=0;row<32;row++){
   const unsigned a=M::Row(group,row/16),coef=Dst(io,a,row%16),minimum=Dst(io,a+4,row%16);
   Record(2,coef);Record(2,minimum);
   if(!completedBlocks && !nextGroup && !row){first[4]=coef;first[5]=minimum;first[6]=Dst(io,C::Accumulator,0);}
  }
 }
 template<class IO> void Affine(IO& io){
  for(unsigned row=0;row<32;row++){
   const unsigned value=Dst(io,C::Accumulator+(row/16)*8,row%16);Record(3,value);
   if(!completedBlocks && !nextGroup && !row)first[7]=value;
  }
 }
public:
 template<class E,class IO> bool Group(E& e,IO& io,unsigned block,unsigned buffer,unsigned group){
  if(published || block>=20 || block!=completedBlocks || buffer!=(block&1) || group>=8 || group!=nextGroup)return false;
  if(!F::PackCodes(e,group,buffer))return false;
  Packed(io);
  if(!F::Matrix<true>(e,F::Ones,group,buffer))return false;
  Ones(io);
  e.Op(0x8a001002);PioneerSfpuMove(e,7,15);PioneerSfpuAddI(e,7,-16);e.Op(0x7b000700);
  e.Op(0x70130020);
  if(!F::Matrix<true>(e,F::Native,group,buffer))return false;
  DotAndMetadata(io,group);
  for(unsigned half=0;half<2;half++)for(unsigned parity=0;parity<=2;parity+=2){
   const unsigned row=M::Row(group,half),acc=C::Accumulator+half*8;
   e.Op(0x70230000|row|parity);e.Op(0x70330000|(row+4)|parity);
   e.Op(0x70030000|(half?40:32)|parity);e.Op(0x70430000|acc|parity);
   PioneerQ38Q4FactoredBfp8::GroupAffine(e);
   e.Op(0x72430000|acc|parity);
  }
  e.Op(0x8a000002);e.Drain();Affine(io);nextGroup++;return true;
 }
 bool EndBlock(unsigned block){
  if(published || block>=20 || block!=completedBlocks || nextGroup!=8)return false;
  blockHashes[completedBlocks++]=blockHash;blockHash=Seed;nextGroup=0;return true;
 }
 template<class IO> bool Publish(IO& io){
  if(published || completedBlocks!=20 || nextGroup)return false;
  published=1;
  for(unsigned row=0;row<32;row++)io.Write(0x98000+row*4,Dst(io,C::Accumulator+(row/16)*8,row%16));
  for(unsigned i=0;i<20;i++)io.Write(0x98080+i*4,blockHashes[i]);
  for(unsigned i=0;i<4;i++)io.Write(0x980d0+i*4,stageHashes[i]);
  for(unsigned i=0;i<8;i++)io.Write(0x980e0+i*4,first[i]);
  return true;
 }
};
#endif
