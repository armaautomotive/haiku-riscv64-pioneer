/* SPDX-License-Identifier: MIT */
#include <cassert>
#define PIONEER_Q38_Q4_FACTORED_BFP8_OFFBOARD 1
#include "../PioneerQ38Q4FactoredBfp8.h"
#define PIONEER_Q38_Q4_FPU_DECODE_MODEL_ONLY
#include "pioneer_q38_q4_fpu_decode_test.cpp"
#include <cfloat>
using Factored=PioneerQ38Q4FactoredBfp8;
static void ExecuteCodes(const Q4FpuWords& words,uint32_t (&registers)[8][32]){
 // Independent,STRICT integer instruction observer. No Dst/packer/FPU
 // emulation and no widening of the existing Q4FpuMachine instruction set.
 for(unsigned w:words.words){
  if(w==0x71720078){for(unsigned lane=0;lane<32;lane++)registers[7][lane]=0x78;continue;}
  assert(w==0x7a003001 || w==0x7afff001 || w==0x7e000701);
  for(unsigned lane=0;lane<32;lane++){
   if(w==0x7a003001)registers[0][lane]<<=3;
   else if(w==0x7afff001)registers[0][lane]>>=1;
   else registers[0][lane]&=registers[7][lane];
  }
 }
}
static uint32_t Bits(float f){return PioneerFmaBits(f);}
static float Float(uint32_t u){float f;memcpy(&f,&u,4);return f;}
int main(){
 // Existing RNE tie controls stay distinct from the hardware ties-away
 // shortcut;this candidate does NOT substitute that shortcut anywhere.
 assert(Round(0x3f808000)==0x3f800000 && Round(0xbf808000)==0xbf800000);
 Q4FpuWords init,codes[2],affine;
 Factored::InitCodes(init);Factored::Codes<false>(codes[0]);Factored::Codes<true>(codes[1]);Factored::GroupAffine(affine);
 assert(init.words==std::vector<uint32_t>{0x71720078});
 assert(codes[0].words==(std::vector<uint32_t>{0x7a003001,0x7e000701}));
 assert(codes[1].words==(std::vector<uint32_t>{0x7afff001,0x7e000701}));
 assert(affine.words.size()==7);
 uint64_t codeCases=0,affineCases=0,boundCases=0;unsigned timingFaults=0,zeroCases=0;
 uint32_t regs[8][32];for(unsigned r=0;r<8;r++)for(unsigned lane=0;lane<32;lane++)regs[r][lane]=0xa1230000+r*32+lane;
 ExecuteCodes(init,regs);
 for(unsigned packed=0;packed<256;packed++)for(unsigned upper=0;upper<2;upper++){
  uint32_t saved[8][32];memcpy(saved,regs,sizeof(saved));
  for(unsigned lane=0;lane<32;lane++)regs[0][lane]=(packed+lane)&255;
  ExecuteCodes(codes[upper],regs);
  for(unsigned lane=0;lane<32;lane++){
   const unsigned q=(((packed+lane)&255)>>(upper*4))&15;
   assert(regs[0][lane]==q*8 && !(regs[0][lane]&0x80));
   // Independent documented BFP8_B value,NOT a tile/UNPACK test.
   const double decoded=double(regs[0][lane])/64*std::ldexp(1.,int(Factored::SharedExponent)-127);
   assert(decoded==q);
   for(unsigned r=1;r<8;r++)assert(regs[r][lane]==saved[r][lane]);codeCases++;
  }
 }
 uint32_t seed=0x27b8f032;auto random=[&](){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;};
 for(unsigned trial=0;trial<4096;trial++){
  Q4FpuMachine m;uint32_t expected[32],original[8][32];long double exact[32],bounds[32];
  for(unsigned lane=0;lane<32;lane++){
   const float d=PioneerHalfFloat(uint16_t((random()%0x7c00)|(random()&0x8000)));
   const float dm=PioneerHalfFloat(uint16_t((random()%0x7c00)|(random()&0x8000)));
   const unsigned scale=random()&63,minimum=random()&63;
   volatile float coefficient=d*float(scale),negativeProduct=dm*float(minimum);
   const float negativeMinimum=Float(Bits(negativeProduct)^0x80000000);
   const int exponent=int(random()%37)-20;int sum=0,dot=0;
   for(unsigned k=0;k<32;k++){
    const int x=int(random()%511)-255;const unsigned q=random()&15;
    sum+=x;dot+=x*int(q);
   }
   // BF16 values (integer[-255,255]*power2) give exact qdot and sum
   // in this fixture. A real FPU/producer must establish those values.
   const float qdot=std::ldexp(float(dot),exponent-7),xsum=std::ldexp(float(sum),exponent-7);
   float old=std::ldexp(float(int(random()%4097)-2048),exponent-4);
   if(trial<4)old=Float((trial&1)?0x80000000:0);
   if(trial<2){coefficient=Float((lane&1)?0x80000000:0);}
   m.lanes[0][lane]=Bits(qdot);m.lanes[1][lane]=Bits(xsum);m.lanes[2][lane]=Bits(coefficient);
   m.lanes[3][lane]=Bits(negativeMinimum);m.lanes[4][lane]=Bits(old);m.lanes[7][lane]=Factored::CodeMask;
   volatile float a=coefficient*qdot,b=negativeMinimum*xsum,c=a+b,result=old+c;
   expected[lane]=Bits(result);
   exact[lane]=static_cast<long double>(old)+static_cast<long double>(coefficient)*qdot+static_cast<long double>(negativeMinimum)*xsum;
   bounds[lane]=(std::fabs(static_cast<long double>(old))+std::fabs(static_cast<long double>(coefficient)*qdot)
       +std::fabs(static_cast<long double>(negativeMinimum)*xsum))*8*std::ldexp(1.L,-24);
  }
  memcpy(original,m.lanes,sizeof(original));m.Execute(affine);assert(m.okay);
  for(unsigned lane=0;lane<32;lane++){
   assert(m.lanes[4][lane]==expected[lane]);
   for(unsigned r:{0U,1U,2U,3U,7U})assert(m.lanes[r][lane]==original[r][lane]);
   assert(std::fabs(static_cast<long double>(Float(m.lanes[4][lane]))-exact[lane])<=bounds[lane]);
   affineCases++;boundCases++;
  }
  // A missing latency word MUST fail the unchanged strict SFPU observer.
  if(trial<16)for(unsigned removed:{2U,4U,6U}){
   Q4FpuMachine bad;memcpy(bad.lanes,original,sizeof(original));
   Q4FpuWords mutant=affine;mutant.words.erase(mutant.words.begin()+removed);bad.Execute(mutant);
   // Final NOP's caller-facing dependency is observed by its next read.
   if(removed==6)for(unsigned lane=0;lane<32;lane++)(void)bad.Read(4,lane);
   assert(!bad.okay);timingFaults++;
  }
 }
 for(unsigned signs=0;signs<32;signs++){
  Q4FpuMachine z;uint32_t expectedZero[32];
  for(unsigned lane=0;lane<32;lane++){
   for(unsigned r=0;r<5;r++)z.lanes[r][lane]=(signs&(1U<<r))?0x80000000:0;
   z.lanes[7][lane]=Factored::CodeMask;
   volatile float a=Float(z.lanes[2][lane])*Float(z.lanes[0][lane]);
   volatile float b=Float(z.lanes[3][lane])*Float(z.lanes[1][lane]);
   volatile float c=a+b,result=Float(z.lanes[4][lane])+c;expectedZero[lane]=Bits(result);
  }
  z.Execute(affine);assert(z.okay);
  for(unsigned lane=0;lane<32;lane++){assert(z.lanes[4][lane]==expectedZero[lane]);zeroCases++;}
 }
 assert(codeCases==16384 && affineCases==131072 && boundCases==131072 && timingFaults==48 && zeroCases==1024);
 printf("PASS_FACTORED_Q4_BFP8 codes%llu exactOriginalNibblesAndDocumentedBfpValues affine%llu bitExactNEW_separateFP32Contract boundChecks%llu zeroSigns%u missingLatencyFaults%u;2codeOps_vs10oldWeightDecode_EXCLUDES_PACK_MATRIX_METADATA_LIFETIME;OFFBOARD_NOT_FPU_NATIVE_QUALITY_OR_TPS\n",
  (unsigned long long)codeCases,(unsigned long long)affineCases,(unsigned long long)boundCases,zeroCases,timingFaults);
}
