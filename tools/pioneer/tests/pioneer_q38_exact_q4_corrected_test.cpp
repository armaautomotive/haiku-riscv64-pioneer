/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_exact_q4_corrected_machine.h"
#include <cstdio>
int main() {
 using P=PioneerQ38ExactQ4Corrected;using E=PioneerQ38Q4EscapeTiles;unsigned jobs=0,dots=0;
 for(unsigned mask=0;mask<=65535;mask++) {
  unsigned expected=0;for(unsigned n=0;n<16;n++)expected|=((mask>>n)&1)<<(2*n);
  assert(P::Spread(mask)==expected);
 }
 for(unsigned schedule=0;schedule<4;schedule++) {
  ExactCorrectedMachine machine;machine.delay[0]=schedule==1?1:schedule==2?100003:79;
  machine.delay[1]=schedule==1?100003:schedule==2?1:7;assert(P::Setup(machine));
  for(unsigned seed=0;seed<256;seed++) {
   std::array<uint8_t,2304> original{};std::array<uint8_t,2816> encoded{};
   for(unsigned p=0;p<16;p++)for(unsigned i=0;i<256;i++) {
    const unsigned q=(seed+(p*16+i/16)*17+(i%16)*3)&15;
    original[E::OriginalAt(p,i)]|=uint8_t(q<<(p&2?4:0));
   }
   assert(E::Encode(original.data(),2304,encoded.data(),2816));
   for(unsigned p=0;p<16;p++) {
    std::array<int,16> input{},expected{};
    for(unsigned k=0;k<16;k++){input[k]=int((seed*7+k*13+p*29)&255)-128;
     for(unsigned n=0;n<16;n++)expected[n]+=int((seed+(p*16+k)*17+n*3)&15)*input[k];}
    machine.StageCorrected(encoded.data()+256+p*128,encoded.data()+2304+p*32,input);
    assert(P::Run(machine) && machine.Clean() && machine.CorrectedResults()==expected);jobs++;dots+=16;
   }
  }
  assert(machine.matrixOps==4096*3 && machine.unpacks[0]==4096*3 && machine.unpacks[1]==4096
   && machine.maskReads==4096*8 && machine.escapeWrites==4096*16 && machine.sevenWrites==32
   && machine.visits[0]==4096*3 && !machine.visits[1] && !machine.visits[2] && !machine.visits[3]);
 }
 unsigned extremes=0,escapeBits=0,refused=0;
 {
  ExactCorrectedMachine machine;assert(P::Setup(machine));
  for(unsigned q=0;q<16;q++)for(int x:{-128,-127,-1,0,1,127}) {
   const int z=int(q==15?14:q)-7;const unsigned sm=z<0?unsigned(-z)|8:unsigned(z);
   std::array<uint8_t,128> primary;primary.fill(uint8_t(sm|(sm<<4)));std::array<uint8_t,32> mask;mask.fill(q==15?255:0);
   std::array<int,16> input,expected;input.fill(x);expected.fill(int(q)*x*16);
   machine.StageCorrected(primary.data(),mask.data(),input);assert(P::Run(machine) && machine.CorrectedResults()==expected);extremes++;
  }
  for(unsigned bit=0;bit<256;bit++) {
   std::array<uint8_t,128> primary;primary.fill(0x77);std::array<uint8_t,32> mask{};mask[bit/8]=uint8_t(1U<<(bit&7));
   std::array<int,16> input{},expected{};int sum=0;
   for(unsigned k=0;k<16;k++){input[k]=int((k*17+bit*7)&255)-128;sum+=input[k];}
   expected.fill(14*sum);expected[bit%16]+=input[bit/16];
   machine.StageCorrected(primary.data(),mask.data(),input);assert(P::Run(machine) && machine.CorrectedResults()==expected);escapeBits++;
  }
 }
 for(unsigned row:{16U,32U}) {
  struct OmittedComponent:ExactCorrectedMachine {
   unsigned omit=0;void Op(unsigned op){const unsigned old=dropPhase;
    if((op>>24)==0x26 && (rwcA&0x38)==omit)dropPhase=0;
    ExactCorrectedMachine::Op(op);dropPhase=old;
   }
  } machine;
  machine.omit=row;assert(P::Setup(machine));std::array<uint8_t,128> primary;primary.fill(0x77);
  std::array<uint8_t,32> mask;mask.fill(255);std::array<int,16> input;input.fill(1);
  machine.StageCorrected(primary.data(),mask.data(),input);assert(P::Run(machine));
  const auto got=machine.CorrectedResults();for(int value:got)assert(value!=240);refused++;
 }
 {
  struct MissingFence:ExactCorrectedMachine{void SealData(){}} machine;
  assert(P::Setup(machine));std::array<uint8_t,128> primary{};std::array<uint8_t,32> mask{};std::array<int,16> input{};
  machine.StageCorrected(primary.data(),mask.data(),input);bool unknown=false;
  try{P::Run(machine);}catch(uint32_t marker){unknown=marker==machine.Unknown;}
  assert(unknown && !machine.matrixOps && !machine.outputReads);refused++;
 }
 printf("PASS_OFFBOARD_CORRECTED_Q4 jobs%u exactOriginalIntegerDots%u masks65536;"
  "extremeCases%u singleEscapeBits%u correctionOrFenceFaults%u;"
  "THREE_MATRIX_PRODUCTS_INCLUDE_PRIMARY_SEVEN_ESCAPE;NOT_FULLK_AFFINE_NATIVE_HARDWARE_OR_TPS\n",jobs,dots,extremes,escapeBits,refused);
}
