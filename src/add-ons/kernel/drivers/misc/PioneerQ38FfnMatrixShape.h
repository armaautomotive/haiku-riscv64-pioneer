/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_MATRIX_SHAPE_H
#define PIONEER_Q38_FFN_MATRIX_SHAPE_H
#include "PioneerQ38FfnResidentShape.h"
// NEW OFFLINE private matrix-root ABI. Not a userspace capability,not the
// old host-staging/reuse selector,not authorization to use a workspace.
// Native integration MUST bind the actual canonical model/mutable owner,
// every learned extent and EVERY participant/source/sequence independently.
enum {TT_Q38_FFN_MATRIX_MARKER=0x314d4651,TT_Q38_FFN_MATRIX_FAULT=0x464d4651,
 TT_Q38_FFN_MATRIX_FRAME=0x7f900,TT_Q38_FFN_MATRIX_ROWS=0x7fa00,
 TT_Q38_FFN_MATRIX_RESIDENT_INPUT=3};
static inline bool PioneerQ38FfnMatrixShape(const uint32_t* d,const uint32_t* header,const uint32_t* banks,
 const uint32_t parts[119][8],const uint32_t* first)
{
 if(!d || !header || !banks || !parts || !first || header[0]!=TT_Q38_FFN_MATRIX_MARKER || header[1]!=1
  || header[2]!=1 || (header[3]!=12 && header[3]!=14) || header[7]!=TT_Q38_FFN_MATRIX_RESIDENT_INPUT || header[8]!=1
  || d[0]!=TT_Q38_FFN_MATRIX_MARKER || d[1]!=1 || d[2]!=67108864 || d[3]>=7 || (d[4]&4095)
  || d[4]<0x04000000 || d[4]>0xc0000000 || d[5]>2 || !d[6] || d[6]>=0x7fffffff || d[6]!=header[6]
  || !d[7] || d[7]>119 || d[7]!=header[5] || d[8]!=PioneerQ38SpanRoute(d[3]) || d[8]!=banks[d[3]]
  || d[9]!=129 || d[10]!=(d[5]==0?30848U:d[5]==1?100480U:274880U)
  || d[11]!=(d[5]==2?5120U:17408U) || d[12]!=d[11]*4 || d[13]!=TT_Q38_FFN_MATRIX_ROWS
  || d[14]!=(d[5]==2?17408U:5120U) || d[14]!=header[4] || d[15]!=header[3] || !d[16] || d[42]>=64)return false;
 for(unsigned i=9;i<16;i++)if(header[i])return false; // NEVER a host DMA header
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 for(unsigned i=43;i<64;i++)if(d[i])return false;
 unsigned rows=0;
 for(unsigned w=0;w<119;w++) {
  if(w>=d[7]){if(first[w])return false;continue;}
  unsigned count=parts[w][2]*16;
  if(!parts[w][2] || parts[w][2]>16 || !parts[w][3] || parts[w][3]>d[6] || parts[w][6] || parts[w][7]
   || (first[w]&15) || first[w]>=d[11] || count>d[11]-first[w])return false;
  for(unsigned p=0;p<w;p++)if(first[w]<first[p]+parts[p][2]*16 && first[p]<first[w]+count)return false;
  rows+=count;
 }
 return rows==d[11]; // exact cover with independently owned learned parts
}
static inline bool PioneerQ38FfnMatrixOrdered(const uint32_t parts[119][8],const uint32_t* first,unsigned workers)
{
 if(!parts || !first || !workers || workers>119)return false;
 unsigned row=0;for(unsigned w=0;w<workers;w++){if(first[w]!=row)return false;row+=parts[w][2]*16;}return true;
}
static inline unsigned PioneerQ38FfnMatrixInputReads(unsigned role){return role==2?4:2;}
static inline unsigned PioneerQ38FfnMatrixInputFlits(unsigned role){return role==2?549:162;}
#endif
