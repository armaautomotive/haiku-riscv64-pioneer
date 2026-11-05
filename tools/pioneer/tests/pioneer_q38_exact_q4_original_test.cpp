/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38ExactQ4Original.h"
#include "../PioneerQ38Q4EscapeTiles.h"
#include <array>
#include <cassert>
#include <cstring>
#include <cstdio>
struct OriginalStage {
 unsigned rawBase,reads=0,primaryWrites=0,maskWrites=0,fences=0;
 std::array<uint8_t,2304> raw{};std::array<unsigned,32> primary{};std::array<unsigned,8> mask{};
 unsigned OriginalWord(unsigned a){assert(!(a&3) && a>=rawBase+256 && a+4<=rawBase+2304);reads++;unsigned v;memcpy(&v,raw.data()+a-rawBase,4);return v;}
 void Primary(unsigned i,unsigned v){assert(i<32);primary[i]=v;primaryWrites++;}
 void MaskWord(unsigned i,unsigned v){assert(i<8);mask[i]=v;maskWrites++;}
 void SealData(){fences++;}
};
int main(){
 using P=PioneerQ38ExactQ4Original;unsigned fours=0,activationCases=0,panels=0;
 for(unsigned q=0;q<65536;q++)for(unsigned shift:{0U,4U}){
  unsigned raw=0,expected=0,mask=0;
  for(unsigned lane=0;lane<4;lane++){
   const unsigned value=(q>>(lane*4))&15;raw|=(value<<shift)<<(lane*8);
   const int z=int(value==15?14:value)-7;expected|=(z<0?unsigned(-z)|8:unsigned(z))<<(lane*4);mask|=unsigned(value==15)<<lane;
  }
  raw|=shift?0x0f0f0f0f:0xf0f0f0f0;const auto result=P::EncodeFour(raw,shift);
  assert(result.primary==expected && result.escape==mask);fours++;
 }
 for(unsigned value=0;value<65536;value++){
  unsigned got=0xdeadbeef;const bool valid=PioneerInt8OperandValid(uint16_t(value));assert(P::Activation(value,got)==valid);
  if(valid){const int x=int(value&1023)*(value&0x8000?-1:1);const float f=float(x);unsigned bits;memcpy(&bits,&f,4);assert(!(bits&65535) && got==bits>>16);}
  else assert(got==0xdeadbeef);activationCases++;
 }
 for(unsigned base:{0x90000U,0x98000U,0x98900U,0x9a000U,0x9a900U})for(unsigned seed=0;seed<16;seed++){
  OriginalStage e;e.rawBase=base;
  for(unsigned i=0;i<2304;i++)e.raw[i]=uint8_t(seed*29+i*71+(i>>3));
  std::array<uint8_t,2816> encoded{};assert(PioneerQ38Q4EscapeTiles::Encode(e.raw.data(),2304,encoded.data(),2816));
  for(unsigned p=0;p<16;p++){
   assert(P::Panel(e,base,p));assert(!memcmp(e.primary.data(),encoded.data()+256+p*128,128) && !memcmp(e.mask.data(),encoded.data()+2304+p*32,32));panels++;
  }
  assert(e.reads==16*64 && e.primaryWrites==16*32 && e.maskWrites==16*8 && e.fences==16);
 }
 for(unsigned base:{0U,0x90001U,0x98001U,0x99200U,0x9ba80U,~0U}){
  OriginalStage e;e.rawBase=base;assert(!P::Panel(e,base,0) && !e.reads && !e.primaryWrites && !e.maskWrites && !e.fences);
 }
 {OriginalStage e;e.rawBase=0x98000;assert(!P::Panel(e,e.rawBase,16) && !e.reads && !e.primaryWrites && !e.maskWrites);}
 printf("PASS_EXACT_Q4_ORIGINAL_DATA fourLaneCombinations%u allNativeOperandPatterns%u panels%u fiveOriginalRawBases;"
  "exactSM3_plusEscape_notRequantization BF16_integerIncludingMinus128;invalidGeometryInert;NO_KERNEL_MODEL_HARDWARE_OR_TPS\n",fours,activationCases,panels);
}
