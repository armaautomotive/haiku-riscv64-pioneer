/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PROJECTION_MATRIX_SHAPE_H
#define PIONEER_Q38_PROJECTION_MATRIX_SHAPE_H
#include "PioneerQ38ProjectionGatherShape.h"
// NEW per-wave ROOT DATA,not an execution capability. The full canonical
// kernel Jobs owner MUST bind every learned part and ALL waves plus actual
// producer/source/ledger/sequence/code owners BEFORE this helper may run.
// This shape validates one wave's bounded/disjoint rows;it does NOT prove
// complete multi-wave cover or authenticate weights from a caller manifest.
enum {TT_Q38_PROJECTION_MATRIX_MARKER=0x314d5051,TT_Q38_PROJECTION_MATRIX_FAULT=0x464d5051,
 TT_Q38_PROJECTION_MATRIX_FRAME=0x7f900,TT_Q38_PROJECTION_MATRIX_ROWS=0x7fa00,
 TT_Q38_PROJECTION_MATRIX_INPUT=4,TT_Q38_PROJECTION_OUTPUT_OFFSET=1048576,
 TT_Q38_PROJECTION_MATRIX_SHADOW=0x7fc00,TT_Q38_PROJECTION_ROWS_SHADOW=0x7fd00};
static inline unsigned PioneerQ38ProjectionJobCount(unsigned rows){
 // Exact fixed public model shapes,independently checked against the
 // canonical heap scheduler. No generic division in device instructions.
 return rows==48?3:rows==1024?64:rows==248320?973:
  rows==5120 || rows==6144 || rows==10240 || rows==12288 || rows==17408?119:0;
}
static inline bool PioneerQ38ProjectionMatrixShape(const uint32_t* d,const uint32_t* h,const uint32_t* banks,
 const uint32_t parts[119][8],const uint32_t* first){
 if(!d || !h || !banks || !parts || !first)return false;
 unsigned type=0,k=0,n=0;
 if(!PioneerQ38ProjectionGeometry(d[42],d[43],type,k,n))return false;
 unsigned jobs=PioneerQ38ProjectionJobCount(n),waves=jobs==973?9:1;
 if(d[0]!=TT_Q38_PROJECTION_MATRIX_MARKER || d[1]!=1 || d[2]!=67108864 || d[3]>=7
  || (d[4]&4095) || d[4]<0x04000000 || d[4]>0xc0000000 || d[5]>=waves
  || !d[6] || d[6]>=0x7fffffff || d[7]!=(jobs-d[5]*119<119?jobs-d[5]*119:119)
  || d[8]!=PioneerQ38ProjectionRoute(d[3]) || d[9]!=129 || d[10]!=TT_Q38_PROJECTION_OUTPUT_OFFSET
  || d[11]!=n || !d[12] || d[12]>119*1024 || (d[12]&63) || d[13]!=TT_Q38_PROJECTION_MATRIX_ROWS
  || d[14]!=k || d[15]!=type || !d[16] || d[44]!=jobs || d[45]!=waves
  || h[0]!=TT_Q38_PROJECTION_MATRIX_MARKER || h[1]!=1 || h[2]!=1 || h[3]!=type || h[4]!=k
  || h[5]!=d[7] || h[6]!=d[6] || h[7]!=TT_Q38_PROJECTION_MATRIX_INPUT || h[8]!=1)return false;
 for(unsigned i=9;i<16;i++)if(h[i])return false;
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 for(unsigned i=46;i<64;i++)if(d[i])return false;
 for(unsigned b=0;b<7;b++)if(banks[b]!=PioneerQ38ProjectionRoute(b))return false;
 if(banks[7])return false;
 unsigned rows=0,blocks=k>>(type==8?5:8),tile=type==8?576:type==12?2304:3392;
 for(unsigned w=0;w<119;w++){
  const uint32_t* p=parts[w];
  if(w>=d[7]){if(first[w])return false;for(unsigned i=0;i<8;i++)if(p[i])return false;continue;}
  if(!p[2] || p[2]>16 || !p[3] || p[3]>d[6] || !p[5] || p[6] || p[7] || p[1]>=7
   || p[4]!=banks[p[1]] || p[0]<0x04000000 || p[0]>=0xc4000000 || (p[0]&63)
   || p[2]*blocks*tile>0xc4000000-p[0] || (first[w]&15) || first[w]>=n || p[2]*16>n-first[w])return false;
  for(unsigned previous=0;previous<w;previous++){
   if(first[w]<first[previous]+parts[previous][2]*16 && first[previous]<first[w]+p[2]*16)return false;
   if(p[1]==parts[previous][1] && p[0]<parts[previous][0]+parts[previous][2]*blocks*tile
    && parts[previous][0]<p[0]+p[2]*blocks*tile)return false;
  }
  rows+=p[2]*16;
 }
 return rows*4==d[12];
}
static inline bool PioneerQ38ProjectionMatrixOrdered(const uint32_t parts[119][8],const uint32_t* first,unsigned workers){
 if(!parts || !first || !workers || workers>119)return false;
 unsigned row=first[0];for(unsigned w=0;w<workers;w++){if(first[w]!=row)return false;row+=parts[w][2]*16;}return true;
}
static inline unsigned PioneerQ38ProjectionMatrixInputReads(const uint32_t* d){return (d[14]*2+16383)/16384+1;}
static inline unsigned PioneerQ38ProjectionMatrixInputFlits(const uint32_t* d){
 unsigned blocks=d[14]>>(d[15]==8?5:8);return (d[14]*2+((blocks*4+63)&~63U))/64;
}
#endif
