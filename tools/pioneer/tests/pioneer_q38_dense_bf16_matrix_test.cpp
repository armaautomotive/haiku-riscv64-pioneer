/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#include "../PioneerQ38DenseBf16Matrix.h"
#include "../PioneerQ38DenseBf16Stage.h"
#include "pioneer_q38_native_bfp_machine.h"
#include <cstdio>

struct L1IO {
 NativeBfpMachine& m;unsigned fences=0;
 unsigned Read(unsigned a){assert(a%4==0 && a+4<=m.l1.size());unsigned v;memcpy(&v,m.l1.data()+a,4);return v;}
 void Write(unsigned a,unsigned v){assert(a%4==0 && a+4<=m.l1.size());memcpy(m.l1.data()+a,&v,4);}
 void Fence(){fences++;}
};

struct TileStage {
 NativeBfpMachine& machine;
 uint16_t keys[32][256],query[256];
 unsigned calls=0,fail=8;
 bool Tile(unsigned tile) {
  assert(tile==calls++);if(tile==fail)return false;
  for(unsigned face=0;face<4;face++)for(unsigned row=0;row<16;row++)for(unsigned col=0;col<16;col++) {
   const unsigned offset=face*256+row*16+col;
   const uint16_t v=keys[(face/2)*16+row][tile*32+(face%2)*16+col];
   machine.l1[0x90000+offset*2]=uint8_t(v);
   machine.l1[0x90001+offset*2]=uint8_t(v>>8);
  }
  for(unsigned half=0;half<2;half++)for(unsigned row=0;row<8;row++)for(unsigned col=0;col<16;col++) {
   const unsigned offset=(half*8+row)*16+col;
   const uint16_t v=query[tile*32+half*16+col];
   machine.l1[0x91000+offset*2]=uint8_t(v);
   machine.l1[0x91001+offset*2]=uint8_t(v>>8);
  }
  return true;
 }
};
static float Value(uint16_t v){return NativeBfpMachine::Float(uint32_t(v)<<16);}
static uint16_t Sample(unsigned n){
 // Finite BF16 with changing signs/exponents/fractions; includes zero.
 if(n%23==0)return 0;
 return uint16_t(((n%7==0)?0x8000:0) | ((124+(n%6))<<7) | ((n*29+3)&127));
}
int main(){
 for(unsigned count: {1U,17U,32U}) {
  NativeBfpMachine machine;TileStage stage{machine,{},{}};
  for(unsigned i=0;i<256;i++)stage.query[i]=Sample(i*17+5);
  for(unsigned row=0;row<32;row++)for(unsigned i=0;i<256;i++)
   stage.keys[row][i]=row<count?Sample(row*307+i*13+7):0;
  assert(PioneerQ38DenseBf16Matrix::Scores(machine,stage,0x90000,0x91000));
  assert(stage.calls==8 && machine.matrixOps==128 && machine.Clean());
  const auto result=machine.Results();
  float max_error=0;
  for(unsigned row=0;row<32;row++){
   double reference=0,absolute=0;
   for(unsigned i=0;i<256;i++){
    const double product=double(Value(stage.keys[row][i]))*Value(stage.query[i]);
    reference+=product;absolute+=std::abs(product);
   }
   const float error=float(std::abs(result[row]-reference));
   max_error=std::max(max_error,error);
   assert(error<=0.00003*absolute+0.000001);
   if(row>=count)assert(result[row]==0);
  }
  std::printf("rows=%u simulated_max_abs_error=%.9g matrix_ops=%u\n",count,max_error,machine.matrixOps);
 }
 NativeBfpMachine machine;TileStage stage{machine,{},{}};stage.fail=3;
 assert(!PioneerQ38DenseBf16Matrix::Scores(machine,stage,0x90000,0x91000));
 assert(stage.calls==4 && machine.matrixOps==48 && machine.Clean());
 std::puts("stage failure returns without publishing results");
 NativeBfpMachine bad;TileStage noio{bad,{},{}};
 assert(!PioneerQ38DenseBf16Matrix::Scores(bad,noio,0x90000,0x90800));
 assert(!PioneerQ38DenseBf16Matrix::Scores(bad,noio,0x90001,0x91000));
 assert(noio.calls==0 && bad.drains==0 && bad.matrixOps==0);
 // The real staging code reads the existing token-major binary16 KV and
 // float32 private query layout; the independent reference stays binary16.
 for(unsigned count: {1U,17U,32U}){
  NativeBfpMachine card;L1IO io{card};
  const unsigned keys=0xa0000,query=0x85000,matrix=0x90000,vector=0x91000;
  for(unsigned i=0;i<256;i++){
   const uint16_t q=uint16_t(0x3000+((i*37)%0x1000));
   io.Write(query+i*4,PioneerAttentionBits(PioneerAttentionHalf(q)));
  }
  for(unsigned r=0;r<count;r++)for(unsigned i=0;i<256;i+=2){
   unsigned lo=0x3000+((r*73+i*29)%0x1000),hi=0x3000+((r*73+(i+1)*29)%0x1000);
   if((r+i)%3==0)lo|=0x8000;if((r+i)%5==0)hi|=0x8000;
   io.Write(keys+r*512+i*2,lo|(hi<<16));
  }
  PioneerQ38DenseBf16Stage<L1IO> actual(io,keys,query,matrix,vector,count);
  assert(PioneerQ38DenseBf16Matrix::Scores(card,actual,matrix,vector));
  const auto result=card.Results();double squared=0,referenceSquared=0,maxAbsolute=0;
  for(unsigned r=0;r<count;r++){
   double reference=0,absolute=0;
   for(unsigned i=0;i<256;i++){
    unsigned pair=io.Read(keys+r*512+(i&~1U)*2);
    uint16_t half=uint16_t(i&1?pair>>16:pair);
    double product=double(PioneerAttentionHalf(half))*PioneerAttentionFloat(io.Read(query+i*4));
    reference+=product;absolute+=std::abs(product);
   }
   const double error=std::abs(result[r]-reference);
   maxAbsolute=std::max(maxAbsolute,error);squared+=error*error;referenceSquared+=reference*reference;
   assert(error<=0.01*absolute+0.000001);
  }
  for(unsigned r=count;r<32;r++)assert(result[r]==0);
  assert(io.fences==8 && card.matrixOps==128);
  std::printf("fp16-layout rows=%u simulated_max_abs_error=%.9g relative_l2=%.9g\n",count,maxAbsolute,std::sqrt(squared/referenceSquared));
 }
 NativeBfpMachine invalid;L1IO io{invalid};
 io.Write(0xa0000,0x00007c00); // nonfinite half K
 PioneerQ38DenseBf16Stage<L1IO> invalidStage(io,0xa0000,0x85000,0x90000,0x91000,1);
 assert(!PioneerQ38DenseBf16Matrix::Scores(invalid,invalidStage,0x90000,0x91000));
 assert(invalid.matrixOps==0 && io.fences==0);
 std::puts("nonfinite K refused before any matrix multiply");
}
