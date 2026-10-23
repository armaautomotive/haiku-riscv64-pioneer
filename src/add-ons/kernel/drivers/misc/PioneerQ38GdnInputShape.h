/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_INPUT_SHAPE_H
#define PIONEER_Q38_GDN_INPUT_SHAPE_H
#include "PioneerQ38ConvShape.h"
// PRIVATE DATA ONLY,not a source/initialized-state/producer/native grant.
// Seven read-only resident inputs -> existing LOCAL half-head GDN layout.
enum {TT_Q38_GDN_INPUT_MARKER=0x31494751,TT_Q38_GDN_INPUT_FAULT=0x46494751,
 TT_Q38_GDN_INPUT_DESCRIPTOR=0x74800,TT_Q38_GDN_INPUT_SUMMARY=0x74900};
static inline bool PioneerQ38GdnInputShape(const uint32_t* d){
 if(!d || d[0]!=TT_Q38_GDN_INPUT_MARKER || d[1]!=1 || d[2]!=67108864 || d[3]>=7
  || (d[4]&4095) || d[4]<0x04000000 || d[4]>0xc0000000 || d[5]>=64 || d[5]%4==3
  || !d[6] || d[6]>=0x7fffffff || d[7]>=96 || d[8]!=PioneerQ38ProjectionRoute(d[3])
  || d[10]!=d[7]+1 || d[9]!=PioneerQ38ConvPeer(d[10]) || d[11]>=7 || d[14]>=7
  || (d[12]&63) || d[12]<0x04000000 || d[12]>0xc3ffffc0 || d[13]!=PioneerQ38ProjectionRoute(d[11])
  || (d[15]&63) || d[15]<0x04000000 || d[15]>0xc3ffffc0 || d[41]!=PioneerQ38ProjectionRoute(d[14])
  || !d[16] || d[40]>=8192)return false;
 unsigned h=d[7]>>1,k=h&15,block=(h>>4)*64;
 if(d[42]!=TT_Q38_CONV_OUTPUT+k*512 || d[43]!=TT_Q38_CONV_OUTPUT+8192+k*512
  || d[44]!=TT_Q38_CONV_OUTPUT+16384+h*512+(d[7]&1)*256
  || d[45]!=PioneerQ38ProjectionResultOffset(10)+block || d[46]!=PioneerQ38ProjectionResultOffset(11)+block
  || d[47]!=(h&15) || d[48]!=512 || d[49]!=512 || d[50]!=256 || d[51]!=64
  || d[52]!=7 || d[53]!=24 || d[54]!=1536 || d[55]!=322)return false;
 if((d[11]==d[3] && d[12]+64>d[4]) || (d[14]==d[3] && d[15]+64>d[4])
  || (d[11]==d[14] && d[12]<d[15]+64 && d[15]<d[12]+64))return false;
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 for(unsigned i=56;i<64;i++)if(d[i])return false;
 return true;
}
static inline bool PioneerQ38GdnInputSummary(const uint32_t* s,const uint32_t* d,const uint32_t* before,const uint32_t* after){
 if(!s || !before || !after || !PioneerQ38GdnInputShape(d) || before[2]!=before[6] || after[2]!=after[6])return false;
 const unsigned first[]={7,24,0,0,322,1536,d[6],TT_Q38_GDN_INPUT_MARKER},delta[]={7,24,7,7,0,0,7,0};
 for(unsigned i=0;i<8;i++)if(s[i]!=first[i] || s[8+i]!=before[i] || s[16+i]!=after[i]
  || uint32_t(after[i]-before[i])!=delta[i])return false;
 const unsigned tail[]={d[7],d[5],d[40],d[10],d[11],d[14],d[42],d[44]};
 for(unsigned i=0;i<8;i++)if(s[24+i]!=tail[i])return false;
 return true; // numerical/receipt DATA,not execution or initialization authority
}
#endif
