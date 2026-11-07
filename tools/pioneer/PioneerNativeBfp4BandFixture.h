/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_BFP4_BAND_FIXTURE_H
#define PIONEER_NATIVE_BFP4_BAND_FIXTURE_H
#include <stdint.h>

// Fixed SYNTHETIC data only for a future isolated BFP4 diagnostic. This is
// not learned weights, a device owner, staging authority or Work inference.
// Independently constructed from integer source n/128 with a tile-specific
// power-of-two scale. Each 16-element face row has max absolute n>=64.
struct PioneerNativeBfp4BandFixture {
 static constexpr unsigned Weight=0x90000, Input=0x124000, Output=0x130000;
 static constexpr unsigned WeightStride=592,InputStride=80;
 static constexpr bool Width(unsigned k){return k==5120 || k==6144 || k==17408;}
 static constexpr unsigned Tiles(unsigned k){return Width(k)?k/32:0;}
 static constexpr unsigned WeightEnd(unsigned k){return Weight-16+Tiles(k)*WeightStride;}
 static constexpr unsigned InputEnd(unsigned k){return Input-16+Tiles(k)*InputStride;}
 static int Numerator(unsigned tile,unsigned row,unsigned column){return int((row*37+column*19+tile*11)%255)-127;}
 static unsigned QuantizedNibble(int n){
  unsigned magnitude=unsigned(n<0?-n:n),q=magnitude/16,remainder=magnitude%16;
  if(remainder>8 || (remainder==8 && (q&1)))q++;
  if(q>7)q=7;
  return q?(q|(n<0?8U:0U)):0U;
 }
 static uint8_t WeightByte(unsigned tile,unsigned index){
  if(index<64)return uint8_t(120+tile%13); // max shared exponent 126 + (tile%13-6)
  if(index>=576)return 0;
  uint8_t value=0;unsigned element=(index-64)*2;
  for(unsigned i=0;i<2;i++){
   unsigned at=element+i,face=at/256,row=(face/2)*16+(at%256)/16,column=(face%2)*16+at%16;
   value|=uint8_t(QuantizedNibble(Numerator(tile,row,column))<<(i*4));
  }
  return value;
 }
 static uint16_t InputBits(unsigned tile,unsigned column){return uint16_t(0x3f00+(column*13+tile*7)%256+((column&3)==2?0x8000:0));}
 static uint32_t Word(unsigned width,unsigned address){
  if(!Width(width) || (address&3))return 0;
  if(address>=Weight-16 && address<WeightEnd(width)){
   unsigned relative=address-(Weight-16),tile=relative/WeightStride,within=relative%WeightStride;
   if(within<16)return 0;
   uint32_t out=0;for(unsigned i=0;i<4;i++)out|=uint32_t(WeightByte(tile,within-16+i))<<(i*8);return out;
  }
  if(address>=Input-16 && address<InputEnd(width)){
   unsigned relative=address-(Input-16),tile=relative/InputStride,within=relative%InputStride;
   if(within<16)return 0;
   unsigned first=(within-16)/2;return uint32_t(InputBits(tile,first))|(uint32_t(InputBits(tile,first+1))<<16);
  }
  return 0;
 }
};
static_assert(PioneerNativeBfp4BandFixture::WeightEnd(17408)<=PioneerNativeBfp4BandFixture::Input-16
 && PioneerNativeBfp4BandFixture::InputEnd(17408)<=PioneerNativeBfp4BandFixture::Output,"synthetic data spans overlap");
#endif
