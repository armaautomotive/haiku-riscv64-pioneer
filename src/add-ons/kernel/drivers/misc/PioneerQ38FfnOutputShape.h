/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_OUTPUT_SHAPE_H
#define PIONEER_Q38_FFN_OUTPUT_SHAPE_H
#include "PioneerQ38FfnResidentShape.h"
// OFFLINE private DATA contract,not a public destination/launch capability.
// A FUTURE controller must independently prove the complete matrix cohort,
// its gathered payload and the retained/initialized FFN workspace owner.
// This is a NEW row map,NOT the old eight-word matrix task ABI's padding.
// The fixed74200 descriptor overlaps the OLD74080 matrix task table:stages
// MUST be exclusively sequenced or relocated by a NEW composite controller.
enum {TT_Q38_FFN_OUTPUT_MARKER=0x314f4651,TT_Q38_FFN_OUTPUT_FAULT=0x464f4651,
 TT_Q38_FFN_OUTPUT_MAP=0x74400,TT_Q38_FFN_OUTPUT_PACKED=0x80000,
 TT_Q38_FFN_OUTPUT_OFFSET=274880};
static inline bool PioneerQ38FfnOutputShape(const uint32_t* d)
{
 if(!d || d[0]!=TT_Q38_FFN_OUTPUT_MARKER || d[1]!=1 || d[2]!=67108864 || d[3]>=7
  || (d[4]&4095) || d[4]<0x04000000 || d[4]>0xc0000000 || d[5]>2
  || !d[6] || d[6]>=0x7fffffff || !d[7] || d[7]>119 || d[8]!=PioneerQ38SpanRoute(d[3])
  || d[9]!=129 || d[10]!=(d[5]==0?unsigned(TT_Q38_FFN_GATE_OFFSET):d[5]==1?unsigned(TT_Q38_FFN_UP_OFFSET):unsigned(TT_Q38_FFN_OUTPUT_OFFSET))
  || d[11]!=(d[5]==2?5120U:17408U) || d[12]!=d[11]*4 || d[13]!=TT_Q38_FFN_OUTPUT_MAP
  || d[14]!=4 || d[15] || !d[16] || d[42]>=64)return false;
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 for(unsigned i=43;i<64;i++)if(d[i])return false;
 return true;
}
// Entries:firstRow,rows,packedFirst,peerSequence. Every row exactly ONCE,
// in ANY matrix-plan row order;packed input is the CURRENT compact root's
// worker-order output,not fixed1024B slots or a monotonically sorted vector.
static inline bool PioneerQ38FfnOutputMap(const uint32_t* d,const uint32_t map[119][4])
{
 if(!PioneerQ38FfnOutputShape(d) || !map)return false;
 unsigned packed=0;
 for(unsigned w=0;w<119;w++) {
  const uint32_t* p=map[w];
  if(w>=d[7]){for(unsigned i=0;i<4;i++)if(p[i])return false;continue;}
  if((p[0]&15) || !p[1] || p[1]>256 || (p[1]&15) || p[0]>=d[11] || p[1]>d[11]-p[0]
   || p[2]!=packed || !p[3] || p[3]>d[6])return false;
  for(unsigned previous=0;previous<w;previous++)if(p[0]<map[previous][0]+map[previous][1]
   && map[previous][0]<p[0]+p[1])return false;
  packed+=p[1];
 }
 return packed==d[11]; // bounded disjoint intervals+equal total exclude holes
}
static inline bool PioneerQ38FfnOutputSummary(const uint32_t* s,const uint32_t* d,
 const uint32_t* before,const uint32_t* after)
{
 if(!s || !before || !after || !PioneerQ38FfnOutputShape(d) || before[2]!=before[6] || after[2]!=after[6]
  || s[0] || s[1] || s[2]!=d[7] || s[3]!=d[12]/64 || s[4]!=d[11] || s[5]!=d[12]
  || s[6]!=d[6] || s[7]!=TT_Q38_FFN_OUTPUT_MARKER || s[24]!=d[5] || s[25]!=d[3]
  || s[26]!=d[4] || s[27]!=d[10] || s[28]!=d[8] || s[29]!=d[42] || s[30]!=129 || s[31]!=d[7])return false;
 const unsigned delta[]={0,0,d[7],0,d[7],d[12]/64,d[7],d[7]};
 for(unsigned i=0;i<8;i++)if(s[8+i]!=before[i] || s[16+i]!=after[i] || uint32_t(after[i]-before[i])!=delta[i])return false;
 return true;
}
#endif
