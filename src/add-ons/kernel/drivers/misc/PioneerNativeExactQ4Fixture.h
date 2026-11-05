/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_EXACT_Q4_FIXTURE_H
#define PIONEER_NATIVE_EXACT_Q4_FIXTURE_H
#include <stdint.h>
// Fixed SYNTHETIC operand DATA only. No learned tensor,CPU dot or user input.
// Original integer q0..15;lossless primary z=min(q,14)-7 +escape mask.
struct PioneerNativeExactQ4Fixture {
 static unsigned Quant(unsigned k,unsigned n){return (k*17+n*3+7)&15;}
 static int Input(unsigned k){return k==0?-128:int((k*37+11)&255)-128;}
 static uint16_t InputBits(unsigned k) {
  const int x=Input(k);if(!x)return 0;const unsigned mag=x<0?unsigned(-x):unsigned(x);
  unsigned top=0;while((mag>>(top+1))!=0)top++;
  return uint16_t((x<0?0x8000:0)|((127+top)<<7)|((mag<<(7-top))&127));
 }
 static unsigned WeightBytes(unsigned bits){return bits==4?800:0;}
 static uint8_t WeightByte(unsigned bits,unsigned at) {
  if(bits!=4)return 0;
  if(at<128) {
   unsigned byte=0;for(unsigned half=0;half<2;half++) {
    const unsigned i=at*2+half,q=Quant(i/16,i%16);const int z=int(q==15?14:q)-7;
    byte|=(z<0?unsigned(-z)|8:unsigned(z))<<(half*4);
   }
   return uint8_t(byte);
  }
  if(at>=768 && at<800) {
   const unsigned first=(at-768)*8;unsigned byte=0;
   for(unsigned i=0;i<8;i++)byte|=unsigned(Quant((first+i)/16,(first+i)%16)==15)<<i;
   return uint8_t(byte);
  }
  return 0; // private constant7/escape scratch,headers,padding before start
 }
 static uint32_t WeightWord(unsigned bits,unsigned word) {
  uint32_t value=0;for(unsigned i=0;i<4;i++)value|=uint32_t(WeightByte(bits,word*4+i))<<(8*i);return value;
 }
 static uint32_t InputWord(unsigned word) {
  const unsigned k=(word*2)%16;return uint32_t(InputBits(k))|(uint32_t(InputBits(k+1))<<16);
 }
};
#endif
