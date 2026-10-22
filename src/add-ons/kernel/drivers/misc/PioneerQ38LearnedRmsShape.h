/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_LEARNED_RMS_SHAPE_H
#define PIONEER_Q38_LEARNED_RMS_SHAPE_H
#include "PioneerQ38ProjectionInputShape.h"
// QRN1 DATA only,not a producer/source or launch capability. Preserve raw
//5120 input for residual,then put learned-normalized input at old op5 base.
enum{TT_Q38_RMS_MARKER=0x314e5251,TT_Q38_RMS_FAULT=0x464e5251,TT_Q38_RMS_BACKUP=98304};
static inline bool PioneerQ38LearnedRmsShape(const uint32_t* d){
 if(!d || d[0]!=TT_Q38_RMS_MARKER || d[1]!=1 || d[2]!=67108864 || d[3]>=7 || (d[4]&4095)
  || d[4]<0x04000000 || d[4]>0xc0000000 || d[5]>64 || !d[6] || d[6]>=0x7fffffff
  || (d[5]<64?d[7]>1:d[7]!=2) || d[8]!=PioneerQ38ProjectionRoute(d[3]) || d[9]!=129
  || d[10]>=7 || (d[11]&4095) || d[11]<0x04000000 || d[11]>0xc3ffb000
  || d[12]!=PioneerQ38ProjectionRoute(d[10]) || d[13]!=20480 || d[14]!=5120 || d[15] || !d[16]
  || d[40]>=8192 || d[41] || d[42]!=TT_Q38_RMS_BACKUP || d[43]!=20480 || d[44]!=0x358637bd
  || d[45]!=5120 || d[46]!=4 || d[47]!=4 || d[48]!=640 || d[49]!=640)return false;
 if(d[3]==d[10] && d[11]+20480>d[4])return false; // learned F32 always precedes canonical runtime envelope
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 for(unsigned i=50;i<64;i++)if(d[i])return false;
 return true;
}
static inline bool PioneerQ38LearnedRmsSummary(const uint32_t* s,const uint32_t* d,const uint32_t* before,const uint32_t* after){
 if(!s || !before || !after || !PioneerQ38LearnedRmsShape(d) || before[2]!=before[6] || after[2]!=after[6])return false;
 const uint32_t first[]={4,640,4,640,5120,20480,d[6],TT_Q38_RMS_MARKER};
 for(unsigned i=0;i<8;i++)if(s[i]!=first[i])return false;
 const unsigned delta[]={4,640,8,4,4,640,8,4};
 for(unsigned i=0;i<8;i++)if(s[8+i]!=before[i] || s[16+i]!=after[i] || uint32_t(after[i]-before[i])!=delta[i])return false;
 return s[24]==d[3] && s[25]==d[4] && s[26]==d[5] && s[27]==d[7] && s[28]==d[10] && s[29]==d[11]
  && s[30]==d[40] && s[31]==TT_Q38_RMS_BACKUP;
}
static_assert(17408*4<=TT_Q38_RMS_BACKUP && TT_Q38_RMS_BACKUP+20480<=TT_Q38_PROJECTION_NATIVE_OFFSET,"raw input/residual/native regions disjoint");
#endif
