/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#include "../PioneerQ38DenseBf16Matrix.h"
#include "../PioneerQ38DenseBf16Stage.h"
#include "pioneer_q38_native_bfp_machine.h"
#include <cstdio>

struct L1IO {
 NativeBfpMachine& m;unsigned fences=0,reads=0;
 unsigned Read(unsigned a){assert(a%4==0 && a+4<=m.l1.size());reads++;unsigned v;memcpy(&v,m.l1.data()+a,4);return v;}
 void Write(unsigned a,unsigned v){assert(a%4==0 && a+4<=m.l1.size());memcpy(m.l1.data()+a,&v,4);}
 void Fence(){fences++;}
};
struct FailedFourStage {
 PioneerQ38DenseBf16ValuesStage<L1IO> stage;
 FailedFourStage(L1IO& io):stage(io,0xa0000,0x82000,0x90000,0x91000,1,0){}
 bool Select(unsigned part){return part!=2 && stage.Select(part);}
 bool Tile(unsigned tile){return stage.Tile(tile);}
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
 unsigned splitCases=0;
 for(unsigned half=0;half<65536;half++){
  if((half&0x7c00)==0x7c00)continue;
  const float original=PioneerAttentionHalf(uint16_t(half));unsigned high=0,low=0;
  assert(PioneerQ38DenseBf16ValuesStage<L1IO>::Split(PioneerAttentionBits(original),false,high));
  assert(PioneerQ38DenseBf16ValuesStage<L1IO>::Split(PioneerAttentionBits(original),true,low));
  assert(PioneerAttentionFloat(high<<16)+PioneerAttentionFloat(low<<16)==original);
  splitCases++;
 }
 assert(splitCases==63488);std::printf("PASS %u finite FP16 values reconstructed by two BF16 components\n",splitCases);
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
 NativeBfpMachine interrupted;L1IO interruptedIO{interrupted};
 interruptedIO.Write(0xa0000,0);interruptedIO.Write(0x82000,0);
 FailedFourStage incomplete(interruptedIO);
 assert(!PioneerQ38DenseBf16Matrix::ValuesFour(interrupted,incomplete,0x90000,0x91000));
 assert(interrupted.matrixOps==32 && interrupted.Clean() && interrupted.outputReads==0);
 NativeBfpMachine sequenceCard;L1IO sequenceIO{sequenceCard};
 sequenceIO.Write(0xa0000,0);sequenceIO.Write(0x82000,0);
 PioneerQ38DenseBf16ValuesStage<L1IO> sequence(sequenceIO,0xa0000,0x82000,0x90000,0x91000,1,0);
 assert(!sequence.Select(1) && sequence.Select(0) && !sequence.Select(1));
 assert(sequence.Tile(0) && sequence.Select(1) && sequence.Tile(0));
 assert(!sequence.Select(3) && sequence.Select(2) && sequence.Tile(0) && sequence.Select(3) && sequence.Tile(0));
 assert(!sequence.Select(4) && sequenceIO.fences==4);
 std::puts("four-product stage ordering and third-part refusal pass");
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
  // 16 packed K words per row plus 32 FP32 query words per K32 tile.
  // Every query is converted once, not once per repeated vector row.
  assert(io.reads==8*(count*16+32));
  // The last staged tile must duplicate precisely the same BF16 bit pattern
  // in every vector row, including both halves and every column.
  for(unsigned half=0;half<2;half++)for(unsigned row=0;row<8;row++)for(unsigned pair=0;pair<8;pair++){
   const unsigned dimension=7*32+half*16+pair*2;unsigned lo=0,hi=0;
   assert(PioneerQ38Bf16Input::Lane(io.Read(query+dimension*4),lo));
   assert(PioneerQ38Bf16Input::Lane(io.Read(query+(dimension+1)*4),hi));
   assert(io.Read(vector+((half*8+row)*16+pair*2)*2)==(lo|(hi<<16)));
  }
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
 NativeBfpMachine invalidQuery;L1IO badQueryIO{invalidQuery};
 for(unsigned i=0;i<16;i++)badQueryIO.Write(0xa0000+i*4,0);
 for(unsigned i=0;i<32;i++)badQueryIO.Write(0x85000+i*4,0);
 badQueryIO.Write(0x85000,0x7fc00000);
 PioneerQ38DenseBf16Stage<L1IO> badQuery(badQueryIO,0xa0000,0x85000,0x90000,0x91000,1);
 assert(!PioneerQ38DenseBf16Matrix::Scores(invalidQuery,badQuery,0x90000,0x91000));
 assert(invalidQuery.matrixOps==0 && invalidQuery.outputReads==0 && badQueryIO.fences==0);
 // PV uses a different layout: transpose token-major FP16 values into
 // output-dimension rows, with probabilities as the BF16 vector.
 for(unsigned count: {1U,17U,32U})for(unsigned block: {0U,3U,7U}){
  NativeBfpMachine card;L1IO valueIO{card};
  const unsigned v=0xa0000,p=0x82000,matrix=0x90000,vector=0x91000;
  for(unsigned t=0;t<count;t++){
   float probability=PioneerAttentionHalf(PioneerAttentionToHalf(float(t+1)/600.0f));
   valueIO.Write(p+t*4,PioneerAttentionBits(probability));
   for(unsigned d=0;d<256;d+=2){
    unsigned lo=0x3000+((t*83+d*31)%0x1000),hi=0x3000+((t*83+(d+1)*31)%0x1000);
    if((t+d)%3==0)lo|=0x8000;if((t+d)%5==0)hi|=0x8000;
    valueIO.Write(v+t*512+d*2,lo|(hi<<16));
   }
  }
  PioneerQ38DenseBf16ValuesStage<L1IO> actual(valueIO,v,p,matrix,vector,count,block);
  assert(PioneerQ38DenseBf16Matrix::Values(card,actual,matrix,vector));
  const auto result=card.Results();double squared=0,referenceSquared=0,maxAbsolute=0;
  for(unsigned row=0;row<32;row++){
   const unsigned d=block*32+row;double reference=0,absolute=0;
   for(unsigned t=0;t<count;t++){
    unsigned pair=valueIO.Read(v+t*512+(d&~1U)*2);
    const uint16_t half=uint16_t(d&1?pair>>16:pair);
    double product=double(PioneerAttentionHalf(half))*PioneerAttentionFloat(valueIO.Read(p+t*4));
    reference+=product;absolute+=std::abs(product);
   }
   const double error=std::abs(result[row]-reference);
   maxAbsolute=std::max(maxAbsolute,error);squared+=error*error;referenceSquared+=reference*reference;
   assert(error<=0.01*absolute+0.000001);
  }
  assert(valueIO.fences==1 && card.matrixOps==16);
  std::printf("pv rows=%u block=%u simulated_max_abs_error=%.9g relative_l2=%.9g\n",count,block,maxAbsolute,std::sqrt(squared/referenceSquared));
  card.expectedWeight=0xa5000;card.alternateWeight=0xa6000;
  card.expectedInput=0xa7000;card.alternateInput=0xa7400;
  PioneerQ38DenseBf16ValuesCachedStage<L1IO> cached(valueIO,v,p,count,block);
  const unsigned beforeCached=valueIO.reads;
  assert(PioneerQ38DenseBf16Matrix::ValuesCached(card,cached));
  assert(valueIO.reads-beforeCached==17*count); // 16 packed V words + one probability per token
  const auto precise=card.Results();double preciseError=0,preciseNorm=0;
  for(unsigned row=0;row<32;row++){
   const unsigned d=block*32+row;double reference=0,absolute=0;
   for(unsigned t=0;t<count;t++){
    unsigned pair=valueIO.Read(v+t*512+(d&~1U)*2);
    const uint16_t half=uint16_t(d&1?pair>>16:pair);
    double product=double(PioneerAttentionHalf(half))*PioneerAttentionFloat(valueIO.Read(p+t*4));
    reference+=product;absolute+=std::abs(product);
   }
   const double error=std::abs(precise[row]-reference);
   preciseError+=error*error;preciseNorm+=reference*reference;
   assert(error<=0.00001*absolute+0.00000001);
  }
  assert(card.matrixOps==80 && valueIO.fences==2 && !cached.Prepare());
  // Compare every packed operand word with the independent original
  // four-pass converter, including padded tokens and both BF16 components.
  for(unsigned part=0;part<4;part++){
   PioneerQ38DenseBf16ValuesStage<L1IO> reference(valueIO,v,p,matrix,vector,count,block,part);
   assert(reference.Tile(0));
   for(unsigned i=0;i<2048;i+=4)assert(valueIO.Read(matrix+i)==valueIO.Read(cached.Matrix(part)+i));
   for(unsigned i=0;i<512;i+=4)assert(valueIO.Read(vector+i)==valueIO.Read(cached.Vector(part)+i));
  }
  std::printf("cached four-product pv rows=%u block=%u relative_l2=%.9g\n",count,block,std::sqrt(preciseError/preciseNorm));
 }
 for(unsigned badKind: {0U,1U,2U}){
  NativeBfpMachine card;L1IO valueIO{card};
  valueIO.Write(0xa0000,badKind==0?0x00007c00:badKind==2?0x7c000000:0x00000000);
  valueIO.Write(0x82000,badKind==1?0x7f800000:0);
  PioneerQ38DenseBf16ValuesStage<L1IO> invalidPV(valueIO,0xa0000,0x82000,0x90000,0x91000,1,0);
  assert(!PioneerQ38DenseBf16Matrix::Values(card,invalidPV,0x90000,0x91000));
  assert(card.matrixOps==0 && valueIO.fences==0);
 }
 NativeBfpMachine invalidBlock;L1IO blockIO{invalidBlock};
 PioneerQ38DenseBf16ValuesStage<L1IO> blockStage(blockIO,0xa0000,0x82000,0x90000,0x91000,1,8);
 assert(!blockStage.Tile(0) && blockIO.fences==0);
 PioneerQ38DenseBf16ValuesCachedStage<L1IO> overlapping(blockIO,0xa5000,0x82000,1,0);
 assert(!overlapping.Prepare() && blockIO.fences==0);
 NativeBfpMachine cachedInvalid;L1IO cachedInvalidIO{cachedInvalid};
 cachedInvalidIO.Write(0xa0000,0x00007c00);cachedInvalidIO.Write(0x82000,0);
 PioneerQ38DenseBf16ValuesCachedStage<L1IO> invalidCachedStage(cachedInvalidIO,0xa0000,0x82000,1,0);
 assert(!PioneerQ38DenseBf16Matrix::ValuesCached(cachedInvalid,invalidCachedStage));
 assert(cachedInvalid.matrixOps==0 && cachedInvalid.outputReads==0 && cachedInvalidIO.fences==0);
 NativeBfpMachine cachedInvalidHigh;L1IO cachedInvalidHighIO{cachedInvalidHigh};
 cachedInvalidHighIO.Write(0xa0000,0x7c000000);cachedInvalidHighIO.Write(0x82000,0);
 PioneerQ38DenseBf16ValuesCachedStage<L1IO> invalidCachedHigh(cachedInvalidHighIO,0xa0000,0x82000,1,0);
 assert(!PioneerQ38DenseBf16Matrix::ValuesCached(cachedInvalidHigh,invalidCachedHigh));
 assert(cachedInvalidHigh.matrixOps==0 && cachedInvalidHigh.outputReads==0 && cachedInvalidHighIO.fences==0);
 std::puts("nonfinite V/probability and invalid PV block refused");
}
