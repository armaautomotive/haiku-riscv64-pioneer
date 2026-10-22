/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_GATHER_SHAPE_H
#define PIONEER_Q38_FFN_GATHER_SHAPE_H
#include "PioneerQ38FfnResidentShape.h"
// OFFLINE ROOT0 DATA prerequisite,not public/native ownership authority.
// Native coordinator MUST prove ALL68 producers completed for this exact
// owner/token/layer/nonce BEFORE entry. Fresh scale tags are additional DATA
// coherence,not a substitute for those hardware proofs or private ranges.
enum {TT_Q38_FFN_GATHER_MARKER=0x31534651,TT_Q38_FFN_GATHER_FAULT=0x46534651,
 TT_Q38_FFN_COMPACT_SCALE_OFFSET=274560};
static inline bool PioneerQ38FfnGatherShape(const uint32_t* d)
{
 if(!d || d[0]!=TT_Q38_FFN_GATHER_MARKER || d[1]!=1 || d[2]!=67108864 || d[3]>=7
  || (d[4]&4095) || d[4]<0x04000000 || d[4]>0xc0000000 || d[5]!=68 || !d[6] || d[6]>=0x7fffffff
  || d[7] || d[8]!=PioneerQ38SpanRoute(d[3]) || d[9]!=129 || d[10]!=TT_Q38_FFN_NATIVE_OFFSET
  || d[11]!=TT_Q38_FFN_SCALE_LINES_OFFSET || d[12]!=TT_Q38_FFN_COMPACT_SCALE_OFFSET || d[13]!=34816
  || d[14]!=320 || d[15] || !d[16] || d[42]>=64)return false;
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 for(unsigned i=43;i<64;i++)if(d[i])return false;
 return true;
}
static inline bool PioneerQ38FfnGatherSummary(const uint32_t* s,const uint32_t* d,const uint32_t* before,const uint32_t* after)
{
 if(!s || !before || !after || !PioneerQ38FfnGatherShape(d) || before[2]!=before[6] || after[2]!=after[6]
  || s[0]!=71 || s[1]!=612 || s[2]!=1 || s[3]!=5 || s[4]!=68 || s[5]!=34816 || s[6]!=d[6] || s[7]!=TT_Q38_FFN_GATHER_MARKER
  || s[24]!=d[3] || s[25]!=d[4] || s[26]!=d[10] || s[27]!=d[11] || s[28]!=d[12] || s[29]!=d[42] || s[30]!=129 || s[31]!=68)return false;
 const unsigned delta[]={71,612,72,71,1,5,72,1};
 for(unsigned i=0;i<8;i++)if(s[8+i]!=before[i] || s[16+i]!=after[i] || uint32_t(after[i]-before[i])!=delta[i])return false;
 return true;
}
#endif
