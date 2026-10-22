/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_RESIDENT_SHAPE_H
#define PIONEER_Q38_FFN_RESIDENT_SHAPE_H
#include "PioneerQ38SpanShape.h"
// OFFLINE private DATA descriptor ONLY,NOT an ioctl/address/ownership or
// initialization grant. Future native owner must derive the workspace from
// its own reserved mutable pages and independently seal EVERY participant.
// The auxiliary68 scale lines occupy4352B of the SAME64MiB workspace,after
// the preceding295360B layout. Each line binds its scale to nonce,block,
// layer,position and ALL four owner epochs. They NEVER race on shared scales.
// Root must gather/compact them on-card AFTER ALL68 producers complete.
enum {TT_Q38_FFN_RESIDENT_MARKER=0x31524651,TT_Q38_FFN_RESIDENT_FAULT=0x46524651,
 TT_Q38_FFN_RESIDENT_DESCRIPTOR=0x74200,TT_Q38_FFN_RESIDENT_SUMMARY=0x74300,
 TT_Q38_FFN_GATE_OFFSET=30848,TT_Q38_FFN_UP_OFFSET=100480,
 TT_Q38_FFN_NATIVE_OFFSET=239744,TT_Q38_FFN_SCALE_LINES_OFFSET=295360,
 TT_Q38_FFN_SCALE_LINES_BYTES=4352,TT_Q38_FFN_RESIDENT_BYTES=299712};
static inline uint32_t PioneerQ38FfnResidentPeer(unsigned block)
{
 if(block>=68)return ~uint32_t(0);
 const unsigned columns[]={1,2,3,4,5,6,7,10,11,12,13,14};
 unsigned column=0,row=block+1;while(row>=10){row-=10;column++;}
 return columns[column]|((row+2)<<6);
}
static inline bool PioneerQ38FfnResidentScale(const uint32_t* line,const uint32_t* d,unsigned block)
{
 if(!line || !d || block>=68)return false;
 uint32_t magnitude=line[0]&0x7fffffff;
 if((magnitude && (magnitude<0x2f800000 || magnitude>0x47800000)) || line[0]==0x80000000
  || line[1]!=TT_Q38_FFN_RESIDENT_MARKER || line[2]!=d[6] || line[3]!=block || line[4]!=d[42]
  || line[5]!=d[40] || line[6]!=d[41] || line[15])return false;
 for(unsigned i=0;i<8;i++)if(line[7+i]!=d[32+i])return false;
 return true; // DATA freshness relation only,not proof that a producer ran
}
static inline bool PioneerQ38FfnResidentShape(const uint32_t* d)
{
 if(!d || d[0]!=TT_Q38_FFN_RESIDENT_MARKER || d[1]!=1 || d[2]!=67108864 || d[3]>=7
  || (d[4]&4095) || d[4]<0x04000000 || d[4]>0xc0000000 || d[5]>=68
  || !d[6] || d[6]>=0x7fffffff || d[7]!=d[5]+1 || d[8]!=PioneerQ38SpanRoute(d[3])
  || d[9]!=PioneerQ38FfnResidentPeer(d[5]) || d[10]!=TT_Q38_FFN_GATE_OFFSET+d[5]*1024
  || d[11]!=TT_Q38_FFN_UP_OFFSET+d[5]*1024 || d[12]!=TT_Q38_FFN_NATIVE_OFFSET+d[5]*512
  || d[13]!=TT_Q38_FFN_SCALE_LINES_OFFSET+d[5]*64 || d[14]!=256 || d[15] || !d[16] || d[42]>=64)return false;
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false; // four owner epochs
 for(unsigned i=43;i<64;i++)if(d[i])return false;
 return true;
}
// CPU DATA observer only,not completion authority. Independent controller
// must additionally prove actual parked PCs,all sources,owner and NoC state.
static inline bool PioneerQ38FfnResidentSummary(const uint32_t* s,const uint32_t* d,
 const uint32_t* before,const uint32_t* after)
{
 if(!s || !before || !after || !PioneerQ38FfnResidentShape(d) || before[2]!=before[6] || after[2]!=after[6]
  || s[0]!=2 || s[1]!=32 || s[2]!=2 || s[3]!=9 || s[4]!=256 || s[5]!=576 || s[6]!=d[6]
  || s[7]!=TT_Q38_FFN_RESIDENT_MARKER || s[24]!=d[5] || s[25]!=d[3] || s[26]!=d[4]
  || s[27]!=d[8] || s[28]!=d[9] || s[29]!=d[42] || s[30]!=d[12] || s[31]!=d[13])return false;
 const unsigned delta[]={2,32,4,2,2,9,4,2};
 for(unsigned i=0;i<8;i++)if(s[8+i]!=before[i] || s[16+i]!=after[i] || uint32_t(after[i]-before[i])!=delta[i])return false;
 return true;
}
#endif
