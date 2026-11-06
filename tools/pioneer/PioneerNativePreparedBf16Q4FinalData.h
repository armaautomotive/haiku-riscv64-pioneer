/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_PREPARED_BF16_Q4_FINAL_DATA_H
#define PIONEER_NATIVE_PREPARED_BF16_Q4_FINAL_DATA_H
#include "PioneerNativeCompactQ4StreamLayout.h"
// Exact terminal DATA prediction for the diagnostic journal. Integer source
// permutation ONLY, never host inference or acceptance of arbitrary scratch.
struct PioneerNativePreparedBf16Q4FinalData {
 static unsigned Spread(unsigned v){v&=65535;v=(v|(v<<8))&0x00ff00ff;v=(v|(v<<4))&0x0f0f0f0f;v=(v|(v<<2))&0x33333333;return (v|(v<<1))&0x55555555;}
 template<class Data> static bool Known(unsigned sample,unsigned k,unsigned a,uint32_t& out){
  if(sample>=9 || !PioneerNativeCompactQ4StreamLayout::Width(k) || (a&3))return false;
  const unsigned original=k/256*1152,last=k/256-1;
  if(a==0x87ff0){out=0x32425151;return true;}
  if(a>=0x88800 && a<0x88880){out=0x11111111;return true;}
  if(a>=0x8a000 && a<0x8a200 && (a-0x8a000)%128<64){
   const unsigned face=(a-0x8a000)/128,word=(a%128)/4;
   const unsigned bits=Data::Word(sample,original+last*1408+1152+(face/2)*128+(14+face%2)*8+word/2);
   out=Spread(bits>>(word%2*16));return true;
  }
  for(unsigned b=0;b<2;b++){
   const unsigned block=k/256-2+b,off=b*0x10000;
   if(a>=0x90000+off && a<0x91200+off){out=Data::Word(sample,original+block*1408+(a-0x90000-off)/4);return true;}
   if(a>=0x8c000+off && a<0x8c400+off){out=Data::Word(sample,original+block*1408+1152+(a-0x8c000-off)/4);return true;}
   if(a>=0x96000+off && a<0x96200+off){out=Data::Word(sample,original+k/256*1408+block*128+(a-0x96000-off)/4);return true;}
  }
  return false;
 }
};
#endif
