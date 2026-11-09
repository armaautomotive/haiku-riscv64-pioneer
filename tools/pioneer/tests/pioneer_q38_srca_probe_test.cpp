/* SPDX-License-Identifier: MIT */
// This tests the actual probe control flow in the alternative model. The
// existing model rejects 0x40 rather than modeling it as a shifted output.
#ifndef PIONEER_TEST_BF16_SRCA_16BIT_BASE
#error This synthetic test requires the alternative SrcA-address hypothesis
#endif
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#include "../PioneerQ38SrcaProbe.h"
#include "pioneer_q38_native_bfp_machine.h"
#include <cstdio>

struct IO {
 NativeBfpMachine& machine;
 unsigned Read(unsigned a){unsigned v;std::memcpy(&v,machine.l1.data()+a,4);return v;}
 void Write(unsigned a,unsigned v){std::memcpy(machine.l1.data()+a,&v,4);}
 void Fence(){}
 unsigned Result(unsigned lane){
  assert(machine.valid[lane<16?32:40] && !machine.mathPending);
  return NativeBfpMachine::Bits(machine.destination[lane<16?32:40][lane&15]);
 }
};
int main(){
 NativeBfpMachine card;IO io{card};
 for(auto& operand:card.source)for(auto& bank:operand)for(auto& row:bank.values)for(auto& v:row)v=0;
 card.expectedWeight=0xa5000;card.alternateWeight=0xa6000;
 card.expectedInput=0xa7000;card.alternateInput=0xa7400;
 for(unsigned i=0;i<256;i+=2){
  const unsigned lo=PioneerAttentionToHalf(float(i+1)/256),hi=PioneerAttentionToHalf(float(i+2)/256);
  io.Write(PioneerQ38SrcaProbe::Values+i*2,lo|(hi<<16));
 }
 io.Write(PioneerQ38SrcaProbe::Probabilities,PioneerAttentionBits(1.f));
 assert(PioneerQ38SrcaProbe::Run(io,card));
 for(unsigned pass=0;pass<2;pass++)for(unsigned i=0;i<32;i++){
  const float expected=pass?float(i+1)/256:((i&15)<4?0.f:float(i-3)/256);
  const float actual=PioneerAttentionFloat(io.Read(PioneerQ38SrcaProbe::Output+(pass*32+i)*4));
  if(actual!=expected)std::fprintf(stderr,"pass=%u lane=%u actual=%g expected=%g\n",pass,i,actual,expected);
  assert(actual==expected);
 }
 std::puts("PASS two-base probe control flow in synthetic SrcA model; NOT A SILICON RESULT");
}
