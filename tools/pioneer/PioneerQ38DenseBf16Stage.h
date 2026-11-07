/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DENSE_BF16_STAGE_H
#define PIONEER_Q38_DENSE_BF16_STAGE_H
#if !defined(PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT) || PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT != 1
#error Experimental dense BF16 input staging only
#endif
#include "PioneerQ38Bf16Input.h"
#include "PioneerAttentionMath.h"

// Converts the existing partition's token-major FP16 K chunk and one FP32
// private query to BF16 matrix faces. The query is repeated for the eight
// output rows consumed by the native non-broadcast matvec. The IO backend is
// on-card L1 access, not a host inference or CPU-validation callback.
// This initial scalar converter is for qualification, NOT a speed claim;
// it may be a bottleneck. Source immutability and all L1 ranges, including
// the 16-byte operand headers, remain the exclusive caller's responsibility.
template<class IO> class PioneerQ38DenseBf16Stage {
 IO& io;unsigned keys,query,matrix,vector,count,next;
public:
 static bool EncodeHalf(unsigned half,unsigned& out){
  if((half&0x7c00)==0x7c00)return false;
  return PioneerQ38Bf16Input::Lane(PioneerAttentionBits(PioneerAttentionHalf(uint16_t(half))),out);
 }
 PioneerQ38DenseBf16Stage(IO& access,unsigned keyAddress,unsigned queryAddress,unsigned matrixAddress,unsigned vectorAddress,unsigned rows)
  :io(access),keys(keyAddress),query(queryAddress),matrix(matrixAddress),vector(vectorAddress),count(rows),next(0){}
 bool Tile(unsigned tile){
  if(tile!=next || tile>=8 || !count || count>32 || (keys&3) || (query&3) || (matrix&15) || (vector&15))return false;
  for(unsigned i=1;i<=4;i++){io.Write(matrix-i*4,0);io.Write(vector-i*4,0);}
  for(unsigned face=0;face<4;face++)for(unsigned row=0;row<16;row++)for(unsigned pair=0;pair<8;pair++){
   const unsigned r=(face/2)*16+row,dimension=tile*32+(face%2)*16+pair*2;
   unsigned lo=0,hi=0;
   if(r<count){
    const unsigned halfs=io.Read(keys+r*512+dimension*2);
    if(!EncodeHalf(halfs&65535,lo) || !EncodeHalf(halfs>>16,hi))return false;
   }
   io.Write(matrix+(face*256+row*16+pair*2)*2,lo|(hi<<16));
  }
  for(unsigned half=0;half<2;half++)for(unsigned row=0;row<8;row++)for(unsigned pair=0;pair<8;pair++){
   unsigned lo=0,hi=0,dimension=tile*32+half*16+pair*2;
   if(!PioneerQ38Bf16Input::Lane(io.Read(query+dimension*4),lo)
     || !PioneerQ38Bf16Input::Lane(io.Read(query+(dimension+1)*4),hi))return false;
   io.Write(vector+((half*8+row)*16+pair*2)*2,lo|(hi<<16));
  }
  io.Fence();next++;return true;
 }
};

// One PV chunk: 32 output dimensions (selected by block) by up to 32
// token-major FP16 value vectors. Probabilities are live FP32 words already
// rounded to FP16 by the existing attention path. Transpose while packing;
// pad missing tokens with zero. This is a single partial PV, not a merge
// across chunks or heads. All inputs stay on the card; no CPU products.
template<class IO> class PioneerQ38DenseBf16ValuesStage {
 IO& io;unsigned values,probabilities,matrix,vector,count,block,part,done;
public:
 // For binary16-origin inputs, high+low reconstructs the original exactly.
 // This is input-format preparation, not a host numerical fallback.
 static bool Split(uint32_t original,bool low,unsigned& result){
  unsigned high=0;
  if(!PioneerQ38Bf16Input::Lane(original,high))return false;
  if(!low){result=high;return true;}
  const float residual=PioneerAttentionFloat(original)-PioneerAttentionFloat(high<<16);
  return PioneerQ38Bf16Input::Lane(PioneerAttentionBits(residual),result);
 }
 PioneerQ38DenseBf16ValuesStage(IO& access,unsigned v,unsigned p,unsigned m,unsigned x,unsigned rows,unsigned dimensionBlock,unsigned productPart=0)
  :io(access),values(v),probabilities(p),matrix(m),vector(x),count(rows),block(dimensionBlock),part(productPart),done(0){}
 bool Tile(unsigned tile){
  if(done || tile || !count || count>32 || block>=8 || part>=4 || (values&3) || (probabilities&3) || (matrix&15) || (vector&15))return false;
  for(unsigned i=1;i<=4;i++){io.Write(matrix-i*4,0);io.Write(vector-i*4,0);}
  for(unsigned face=0;face<4;face++)for(unsigned row=0;row<16;row++)for(unsigned pair=0;pair<8;pair++){
   const unsigned dimension=block*32+(face/2)*16+row,token=(face%2)*16+pair*2;
   unsigned output[2]={0,0};
   for(unsigned j=0;j<2;j++)if(token+j<count){
    const unsigned halves=io.Read(values+(token+j)*512+(dimension&~1U)*2);
    const unsigned half=dimension&1?halves>>16:halves&65535;
    if((half&0x7c00)==0x7c00 || !Split(PioneerAttentionBits(PioneerAttentionHalf(uint16_t(half))),part&1,output[j]))return false;
   }
   io.Write(matrix+(face*256+row*16+pair*2)*2,output[0]|(output[1]<<16));
  }
  for(unsigned half=0;half<2;half++)for(unsigned row=0;row<8;row++)for(unsigned pair=0;pair<8;pair++){
   unsigned lo=0,hi=0,token=half*16+pair*2;
   if(token<count && !Split(io.Read(probabilities+token*4),part&2,lo))return false;
   if(token+1<count && !Split(io.Read(probabilities+(token+1)*4),part&2,hi))return false;
   io.Write(vector+((half*8+row)*16+pair*2)*2,lo|(hi<<16));
  }
  io.Fence();done=1;return true;
 }
};
#endif
