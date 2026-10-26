/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DENSE_PROJECTION_SHAPE_H
#define PIONEER_Q38_DENSE_PROJECTION_SHAPE_H
#include "PioneerQ38ProjectionGatherShape.h"
// NEW full dense matrix DATA. Not a QPS1/QPS2/GDN result-layout alias.
enum {TT_Q38_DENSE_MATRIX_MARKER=0x314d4451,TT_Q38_DENSE_MATRIX_FAULT=0x464d4451,
 TT_Q38_DENSE_MATRIX_FRAME=0x7f900,TT_Q38_DENSE_MATRIX_ROWS=0x7fa00,TT_Q38_DENSE_MATRIX_INPUT=4,
 TT_Q38_DENSE_MATRIX_SHADOW=0x7fc00,TT_Q38_DENSE_ROWS_SHADOW=0x7fd00,
 TT_Q38_DENSE_QG=0x180000,TT_Q38_DENSE_K=0x18c000,TT_Q38_DENSE_V=0x18d000,
 TT_Q38_DENSE_ATTN=0x190000,TT_Q38_DENSE_O=0x196000,TT_Q38_DENSE_END=0x19b000};
static inline bool PioneerQ38DenseProjectionGeometry(unsigned layer,unsigned role,unsigned& type,unsigned& k,unsigned& n){
 if(layer>=64 || layer%4!=3 || (role!=5 && role!=6 && role!=7 && role!=10))return false;
 type=role==10?14:8;k=role==10?6144:5120;n=role==5?12288:role==10?5120:1024;return true;
}
static inline uint32_t PioneerQ38DenseProjectionOffset(unsigned layer,unsigned role){
 unsigned t=0,k=0,n=0;if(!PioneerQ38DenseProjectionGeometry(layer,role,t,k,n))return ~uint32_t(0);
 return role==5?TT_Q38_DENSE_QG:role==6?TT_Q38_DENSE_K:role==7?TT_Q38_DENSE_V:TT_Q38_DENSE_O;
}
static inline unsigned PioneerQ38DenseProjectionJobCount(unsigned rows){return rows==12288 || rows==5120?119:rows==1024?64:0;}
static inline bool PioneerQ38DenseProjectionRecipe(const uint32_t* d,const uint32_t* h,const uint32_t* banks,
 const uint32_t parts[119][8],const uint32_t* first,bool runtime){
 if(!d || !h || !banks || !parts || !first)return false;
 unsigned type=0,k=0,n=0;if(!PioneerQ38DenseProjectionGeometry(d[42],d[43],type,k,n))return false;
 unsigned jobs=PioneerQ38DenseProjectionJobCount(n);
 if(d[0]!=TT_Q38_DENSE_MATRIX_MARKER || d[1]!=1 || d[2]!=67108864 || d[3]>=7 || (d[4]&4095)
  || d[4]<0x04000000 || d[4]>0xc0000000 || d[5] || !d[6] || d[6]>=0x7fffffff || d[7]!=jobs
  || d[8]!=PioneerQ38ProjectionRoute(d[3]) || d[9]!=129 || d[10]!=PioneerQ38DenseProjectionOffset(d[42],d[43])
  || d[11]!=n || d[12]!=n*4 || d[13]!=TT_Q38_DENSE_MATRIX_ROWS || d[14]!=k || d[15]!=type || !d[16]
  || d[40]>=8192 || d[41]>=248320 || d[44]!=jobs || d[45]!=1
  || h[0]!=TT_Q38_DENSE_MATRIX_MARKER || h[1]!=1 || h[2]!=1 || h[3]!=type || h[4]!=k
  || h[5]!=jobs || h[6]!=d[6] || h[7]!=TT_Q38_DENSE_MATRIX_INPUT || h[8]!=1)return false;
 for(unsigned i=9;i<16;i++)if(h[i])return false;
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 for(unsigned i=46;i<64;i++)if(d[i])return false;
 for(unsigned b=0;b<7;b++)if(banks[b]!=PioneerQ38ProjectionRoute(b))return false;
 if(banks[7])return false;
 unsigned rows=0,blocks=k>>(type==8?5:8),tile=type==8?576:3392;
 for(unsigned w=0;w<119;w++){
  const auto* p=parts[w];
  if(w>=jobs){if(first[w])return false;for(unsigned i=0;i<8;i++)if(p[i])return false;continue;}
  if(!p[2] || p[2]>16 || (runtime?(!p[3] || p[3]>d[6]):p[3]!=0) || !p[5] || p[6] || p[7]
   || p[1]>=7 || p[4]!=banks[p[1]] || p[0]<0x04000000 || p[0]>=0xc4000000 || (p[0]&63)
   || p[2]*blocks*tile>0xc4000000-p[0] || (first[w]&15) || first[w]>=n || p[2]*16>n-first[w]
   || (p[1]==d[3] && p[0]<d[4]+67108864 && d[4]<p[0]+p[2]*blocks*tile))return false;
  for(unsigned old=0;old<w;old++){
   const auto* a=parts[old];
   if((first[w]<first[old]+a[2]*16 && first[old]<first[w]+p[2]*16)
    || (p[1]==a[1] && p[0]<a[0]+a[2]*blocks*tile && a[0]<p[0]+p[2]*blocks*tile))return false;
  }
  rows+=p[2]*16;
 }
 return rows==n; // geometry,not canonical weight/source/input/sequence ownership
}
static inline bool PioneerQ38DenseProjectionMatrixShape(const uint32_t* d,const uint32_t* h,const uint32_t* banks,
 const uint32_t parts[119][8],const uint32_t* first){return PioneerQ38DenseProjectionRecipe(d,h,banks,parts,first,true);}
static inline bool PioneerQ38DenseProjectionMatrixOrdered(const uint32_t parts[119][8],const uint32_t* first,unsigned workers){
 if(!parts || !first || !workers || workers>119)return false;
 unsigned row=first[0];for(unsigned w=0;w<workers;w++){if(first[w]!=row)return false;row+=parts[w][2]*16;}return true;
}
static inline unsigned PioneerQ38DenseProjectionInputReads(const uint32_t* d){return (d[14]*2+16383)/16384+1;}
static inline unsigned PioneerQ38DenseProjectionInputFlits(const uint32_t* d){return (d[14]*2+(((d[14]>>(d[15]==8?5:8))*4+63)&~63U))/64;}
static_assert(TT_Q38_DENSE_QG+12288*4==TT_Q38_DENSE_K && TT_Q38_DENSE_K+1024*4==TT_Q38_DENSE_V
 && TT_Q38_DENSE_V+1024*4<=TT_Q38_DENSE_ATTN && TT_Q38_DENSE_ATTN+6144*4==TT_Q38_DENSE_O
 && TT_Q38_DENSE_O+5120*4==TT_Q38_DENSE_END && TT_Q38_DENSE_END<2097152,
 "NEW disjoint dense DATA inside2MiB;not old zero-init/native ownership grant");
#endif
