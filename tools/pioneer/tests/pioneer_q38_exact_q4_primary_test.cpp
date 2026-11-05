/* SPDX-License-Identifier: MIT */
// Independent OFFBOARD direct-emitter observer;NOT a native image or timing.
#include "pioneer_q38_native_bfp_machine.h"
#include "../PioneerQ38ExactQ4Primary.h"
#include "../PioneerQ38Q4EscapeTiles.h"
#include <cstdio>
#ifdef NDEBUG
#error Prototype assertions may not be disabled
#endif
struct ExactPrimaryMachine:NativeBfpMachine {
 void StagePrimary(const uint8_t* primary,const std::array<int,16>& input) {
  assert(Clean());memcpy(l1.data()+0x90000,primary,128);
  for(unsigned r=0;r<8;r++)for(unsigned k=0;k<16;k++) {
   assert(input[k]>=-128 && input[k]<=127);
   const unsigned bits=Bits(float(input[k])),at=0x91000+(r*16+k)*2;
   assert(!(bits&65535));l1[at]=uint8_t(bits>>16);l1[at+1]=uint8_t(bits>>24);
  }
 }
 void PrimaryUnpack(unsigned which) {
  assert(which<2 && thread[0]==0 && thread[5]==4);
  const unsigned base=64+which*48;
  assert(config[base]==(which?0x00800015U:0x01000017U));
  assert(config[base+1]==0x00010001 && config[base+2]==1 && !config[base+3]);
  assert(config[base+8]==(which?0x25U:0x27U));
  assert(config[base+9]==(which?0x000f000fU:0x000f010fU));
  assert(config[base+10]==0x17fff && !config[base+11] && !config[base+28]);
  assert(config[50]==0x181 && config[84]==0x00400040 && !config[49] && !config[61]);
  assert(xStart[which]==0 && xEnd[which]==(which?127U:255U));
  const unsigned address=(config[base+12]+1)*16;
  assert(address==(which?0x91000U:0x90000U) && !(address&15));
  auto& s=source[which][unpackBank[which]];assert(!s.pending && !s.owner);
  const unsigned count=xEnd[which]+1;
  for(unsigned i=0;i<count;i++) {
   float value;
   if(which) {
    const unsigned at=address+2*i;
    value=Float(uint32_t(l1[at]|(unsigned(l1[at+1])<<8))<<16);
   } else {
    const unsigned q=(l1[address+i/2]>>((i&1)*4))&15,magnitude=q&7;
    assert(q!=8);
    // Numeric ISA model,NOT CPU codec. No exponent bytes are consumed.
    value=float(std::ldexp((q&8?-1.L:1.L)*magnitude,int(config[50]&255)-129));
   }
   s.values[i/16][i%16]=value;
  }
  s.pending=true;s.due=cycle+delay[which];unpacks[which]++;
 }
 void Op(unsigned op) {
  if(op==0x420000c1 || op==0x428000c1) {cycle++;Advance();PrimaryUnpack((op>>23)&1);}
  else NativeBfpMachine::Op(op);
 }
 std::array<int,16> PrimaryResults() {
  if(!Clean())throw Unknown;
  std::array<int,16> out{};
  for(unsigned n=0;n<16;n++) {
   for(unsigned row=32;row<40;row++)assert(valid[row] && std::isfinite(destination[row][n])
    && destination[row][n]==destination[32][n]);
   const float value=destination[32][n];assert(value==float(int(value)));out[n]=int(value);
  }
  outputReads+=16;return out;
 }
};
#ifndef PIONEER_Q38_EXACT_Q4_MACHINE_ONLY
int main() {
 using P=PioneerQ38ExactQ4Primary;using E=PioneerQ38Q4EscapeTiles;
 unsigned jobs=0,dots=0;
 for(unsigned schedule=0;schedule<4;schedule++) {
  ExactPrimaryMachine machine;machine.delay[0]=schedule==1?1:schedule==2?100003:79;
  machine.delay[1]=schedule==1?100003:schedule==2?1:7;assert(P::Setup(machine));
  for(unsigned seed=0;seed<256;seed++) {
   std::array<uint8_t,2304> original{};std::array<uint8_t,2816> encoded{};
   for(unsigned p=0;p<16;p++)for(unsigned i=0;i<256;i++) {
    const unsigned q=(seed+(p*16+i/16)*17+(i%16)*3)&15;
    original[E::OriginalAt(p,i)]|=uint8_t(q<<(p&2?4:0));
   }
   assert(E::Encode(original.data(),2304,encoded.data(),2816));
   for(unsigned p=0;p<16;p++) {
    std::array<int,16> input{},expected{},correction{};int sum=0;
    for(unsigned k=0;k<16;k++) {
     input[k]=int((seed*7+k*13+p*29)&255)-128;sum+=input[k];
     for(unsigned n=0;n<16;n++) {
      const unsigned q=(seed+(p*16+k)*17+n*3)&15;
      expected[n]+=int(q==15?14:q)*input[k]-7*input[k];
      correction[n]+=int(q==15)*input[k];
     }
    }
    machine.StagePrimary(encoded.data()+256+p*128,input);assert(P::Run(machine));const auto got=machine.PrimaryResults();
    assert(got==expected && machine.Clean());
    for(unsigned n=0;n<16;n++) {
     int originalDot=0;
     for(unsigned k=0;k<16;k++)originalDot+=int((seed+(p*16+k)*17+n*3)&15)*input[k];
     assert(got[n]+correction[n]+7*sum==originalDot);dots++;
    }
    jobs++;
   }
  }
  assert(machine.matrixOps==4096 && machine.unpacks[0]==4096 && machine.unpacks[1]==4096
   && machine.visits[0]==4096 && !machine.visits[1] && !machine.visits[2] && !machine.visits[3]);
 }
 // A missing producer/final completion is NOT silently accepted.
 for(unsigned producer=1;producer<=2;producer++) {
  ExactPrimaryMachine machine;assert(P::Setup(machine));machine.dropProducer=producer;
  std::array<uint8_t,128> weight{};std::array<int,16> input{};machine.StagePrimary(weight.data(),input);
  bool refused=false;try {P::Run(machine);}catch(uint32_t marker){refused=marker==machine.Unknown;}
  assert(refused && !machine.matrixOps && !machine.outputReads);
 }
 {
  ExactPrimaryMachine machine;assert(P::Setup(machine));machine.omitFlip=true;
  std::array<uint8_t,128> weight{};std::array<int,16> input{};machine.StagePrimary(weight.data(),input);
  assert(P::Run(machine) && !machine.Clean());bool refused=false;
  try{(void)machine.PrimaryResults();}catch(uint32_t marker){refused=marker==machine.Unknown;}
  assert(refused && !machine.outputReads);
 }
 {
  struct MissingFinalDrain:ExactPrimaryMachine {
   unsigned calls=0;void Drain(){if(++calls!=6)ExactPrimaryMachine::Drain();}
  } machine;
  assert(P::Setup(machine));std::array<uint8_t,128> weight{};std::array<int,16> input{};machine.StagePrimary(weight.data(),input);
  assert(P::Run(machine) && machine.mathPending);bool refused=false;
  try{(void)machine.PrimaryResults();}catch(uint32_t marker){refused=marker==machine.Unknown;}
  assert(refused && !machine.outputReads);
 }
 // A general BF16 value with8 significant bits is OUTSIDE this integer
 // contract:phase0 alone loses a term. Prevent a software pass being read
 // as permission to truncate the old arbitrary-BF16 path's fidelity.
 {
  ExactPrimaryMachine machine;assert(P::Setup(machine));
  std::array<uint8_t,128> weight;weight.fill(0x77);std::array<int,16> input{};machine.StagePrimary(weight.data(),input);
  for(unsigned r=0;r<8;r++){machine.l1[0x91000+r*32]=0xff;machine.l1[0x91001+r*32]=0x3f;}
  assert(P::Run(machine) && machine.Clean());
  assert(machine.destination[32][0]!=7*machine.Float(0x3fff0000));
 }
 printf("PASS_OFFBOARD_EXACT_Q4_PRIMARY jobs%u integerDots%u fourProducerSchedules dualBanks;"
  "ONE_MATRIX_PHASE_PER_K16N16_INTEGER_INPUT_PANEL;NOT_FULL_Q4_KERNEL_NATIVE_IMAGE_OR_HARDWARE_TPS\n",jobs,dots);
}
#endif
