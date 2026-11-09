/* SPDX-License-Identifier: MIT */
// Alternative SrcA-address model inferred from one physical trace, not ISA.
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_TEST_BF16_SRCA_16BIT_BASE 1
#include "../PioneerQ38DenseBf16Matrix.h"
#include "../PioneerQ38DenseBf16Stage.h"
#include "pioneer_q38_native_bfp_machine.h"
#include <cstdio>
struct IO {
 NativeBfpMachine& m;
 unsigned Read(unsigned a){unsigned v;std::memcpy(&v,m.l1.data()+a,4);return v;}
 void Write(unsigned a,unsigned v){std::memcpy(m.l1.data()+a,&v,4);}
 void Fence(){}
};
struct HypothesisMachine:NativeBfpMachine {
 unsigned base;
 explicit HypothesisMachine(unsigned selected):base(selected){
  for(auto& operand:source)for(auto& bank:operand)for(auto& row:bank.values)for(auto& v:row)v=0;
 }
 void Config(unsigned i,unsigned v){NativeBfpMachine::Config(i,i==84?0x00400000|base:v);}
};
static void Test(unsigned base,bool shifted){
 HypothesisMachine card(base);IO io{card};card.expectedWeight=0xa5000;card.alternateWeight=0xa6000;
 card.expectedInput=0xa7000;card.alternateInput=0xa7400;
 for(unsigned i=0;i<256;i+=2){
  const unsigned a=PioneerAttentionToHalf(float(i+1)/256),b=PioneerAttentionToHalf(float(i+2)/256);
  io.Write(0xa0000+i*2,a|(b<<16));
 }
 io.Write(0x82000,PioneerAttentionBits(1.f));
 PioneerQ38DenseBf16ValuesCachedStage<IO> stage(io,0xa0000,0x82000,1,0);
 assert(PioneerQ38DenseBf16Matrix::ValuesCached(card,stage));const auto result=card.Results();
 for(unsigned i=0;i<32;i++){
  const float expected=shifted?((i&15)<4?0.f:float(i-3)/256):float(i+1)/256;
  if(result[i]!=expected)std::fprintf(stderr,"base=%x lane=%u actual=%g expected=%g\n",base,i,result[i],expected);
  assert(result[i]==expected);
 }
}
int main(){Test(0x80,true);Test(0x40,false);
 std::puts("PASS alternative SrcA-base model reproduces shift and candidate repairs synthetic PV; HYPOTHESIS NOT SILICON");}
