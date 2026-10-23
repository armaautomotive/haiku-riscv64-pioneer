/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_CONV_SHAPE_H
#define PIONEER_Q38_CONV_SHAPE_H
#include "PioneerQ38ProjectionInputShape.h"
#include "PioneerQ38ProjectionResultLayout.h"
// QCV1 OFFLINE descriptor DATA ONLY. No public endpoint/native owner grant.
// Each16channel job consumes genuine QPS1 QKV,ORIGINAL F32 Conv4 weights
// and immutable previous3sample state. Outputs/next-state are DISJOINT
// workspace scratch,not the canonical current state or a publication.
enum {TT_Q38_CONV_MARKER=0x31564351,TT_Q38_CONV_FAULT=0x46564351,
 TT_Q38_CONV_CHANNELS=10240,TT_Q38_CONV_GROUP=16,TT_Q38_CONV_JOBS=640,
 TT_Q38_CONV_OUTPUT=0x120000,TT_Q38_CONV_NEXT=0x130000};
static inline uint32_t PioneerQ38ConvPeer(unsigned worker){
 if(worker>=120)return ~uint32_t(0);
 const unsigned columns[]={1,2,3,4,5,6,7,10,11,12,13,14};
 unsigned column=0,row=worker;while(row>=10){row-=10;column++;}
 return columns[column]|((row+2)<<6); // no unqualified MULHU/divide
}
static inline bool PioneerQ38ConvShape(const uint32_t* d){
 if(!d || d[0]!=TT_Q38_CONV_MARKER || d[1]!=1 || d[2]!=67108864 || d[3]>=7
  || (d[4]&4095) || d[4]<0x04000000 || d[4]>0xc0000000 || d[5]>=64 || d[5]%4==3
  || !d[6] || d[6]>=0x7fffffff || d[7]>=10240 || (d[7]&15)
  || d[8]!=PioneerQ38ProjectionRoute(d[3]) || d[10]>=120 || d[9]!=PioneerQ38ConvPeer(d[10])
  || d[11]>=7 || (d[12]&255) || d[12]<0x04000000 || d[12]>0xc3ffff00
  || d[13]!=PioneerQ38ProjectionRoute(d[11]) || d[14]>=7 || (d[15]&63)
  || d[15]<0x04000000 || d[15]>0xc3ffff40 || !d[16] || d[40]>=8192
  || d[41]!=PioneerQ38ProjectionRoute(d[14]) || d[42]!=TT_Q38_CONV_NEXT+d[7]*12
  || d[43]!=TT_Q38_CONV_OUTPUT+d[7]*4 || d[44]!=16
  || d[45]!=PioneerQ38ProjectionResultOffset(6)+d[7]*4 || d[46]!=192 || d[47]!=256
  || d[48]!=3 || d[49]!=2 || d[50]!=8 || d[51]!=4)return false;
 if((d[11]==d[3] && d[12]+256>d[4]) || (d[14]==d[3] && d[15]+192>d[4]))return false;
 // Original weights and previous state cannot alias even in one bank.
 if(d[11]==d[14] && d[12]<d[15]+192 && d[15]<d[12]+256)return false;
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 for(unsigned i=52;i<64;i++)if(d[i])return false;
 return true;
}
static inline bool PioneerQ38ConvSummary(const uint32_t* s,const uint32_t* d,const uint32_t* before,const uint32_t* after){
 if(!s || !before || !after || !PioneerQ38ConvShape(d) || before[2]!=before[6] || after[2]!=after[6])return false;
 const unsigned first[]={3,8,2,4,16,256,d[6],TT_Q38_CONV_MARKER},delta[]={3,8,5,3,2,4,5,2};
 for(unsigned i=0;i<8;i++)if(s[i]!=first[i] || s[8+i]!=before[i] || s[16+i]!=after[i]
  || uint32_t(after[i]-before[i])!=delta[i])return false;
 const unsigned tail[]={d[7],d[5],d[40],d[10],d[11],d[14],d[42],d[43]};
 for(unsigned i=0;i<8;i++)if(s[24+i]!=tail[i])return false;
 return true; // observer DATA,not execution/initialization/producer proof
}
static_assert(TT_Q38_PROJECTION_RESULT_BASE+TT_Q38_PROJECTION_RESULT_BYTES<=TT_Q38_CONV_OUTPUT
 && TT_Q38_CONV_OUTPUT+10240*4<=TT_Q38_CONV_NEXT && TT_Q38_CONV_NEXT+30720*4<=2097152,
 "QPS1/conv/current-private-next scratch disjoint inside existing2MiB prefix");
#endif
