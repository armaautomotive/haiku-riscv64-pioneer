/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_GATHER2_SHAPE_H
#define PIONEER_Q38_FFN_GATHER2_SHAPE_H
#include "PioneerQ38FfnInputShape.h"
#include "PioneerQ38FfnGatherShape.h"
// NEW OFFLINE20/68-producer gather ABI. All actual producer/physical/current
// owner proofs MUST precede entry. This shape/tag/summary grants NONE.
enum {TT_Q38_FFN_GATHER2_MARKER=0x32534651,TT_Q38_FFN_GATHER2_FAULT=0x46534651};
static inline bool PioneerQ38FfnGather2Shape(const uint32_t* d)
{
 if(!d || d[0]!=TT_Q38_FFN_GATHER2_MARKER || d[1]!=1 || d[2]!=67108864 || d[3]>=7
  || (d[4]&4095) || d[4]<0x04000000 || d[4]>0xc0000000 || (d[5]!=20 && d[5]!=68)
  || !d[6] || d[6]>=0x7fffffff || d[7] || d[8]!=PioneerQ38SpanRoute(d[3]) || d[9]!=129
  || d[10]!=(d[5]==20?20480U:239744U) || d[11]!=TT_Q38_FFN_SCALE_LINES_OFFSET
  || d[12]!=(d[5]==20?30720U:274560U) || d[13]!=(d[5]==20?10240U:34816U)
  || d[14]!=(d[5]==20?128U:320U) || d[15] || !d[16] || d[42]>=64)return false;
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 for(unsigned i=43;i<64;i++)if(d[i])return false;
 return true;
}
static inline bool PioneerQ38FfnGather2Summary(const uint32_t* s,const uint32_t* d,const uint32_t* before,const uint32_t* after)
{
 if(!s || !before || !after || !PioneerQ38FfnGather2Shape(d) || before[2]!=before[6] || after[2]!=after[6])return false;
 unsigned reads=d[5]+(d[13]+16383)/16384,readFlits=d[5]+d[13]/64,writeFlits=d[14]/64;
 if(s[0]!=reads || s[1]!=readFlits || s[2]!=1 || s[3]!=writeFlits || s[4]!=d[5] || s[5]!=d[13]
  || s[6]!=d[6] || s[7]!=TT_Q38_FFN_GATHER2_MARKER || s[24]!=d[3] || s[25]!=d[4] || s[26]!=d[10]
  || s[27]!=d[11] || s[28]!=d[12] || s[29]!=d[42] || s[30]!=129 || s[31]!=d[5])return false;
 const unsigned delta[]={reads,readFlits,reads+1,reads,1,writeFlits,reads+1,1};
 for(unsigned i=0;i<8;i++)if(s[8+i]!=before[i] || s[16+i]!=after[i] || uint32_t(after[i]-before[i])!=delta[i])return false;
 return true; // DATA ONLY,not producer/current-PC/lifetime authority
}
#endif
