/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_RECURRENT_RECIPE_H
#define PIONEER_Q38_MIXED_RECURRENT_RECIPE_H
#include "PioneerQ38DenseProjectionShape.h"
// Complete single-wave recurrent matrix DATA for the new QRP1 controller.
// ZERO sequence placeholders: only the device controller binds live nonces.
// No original-weight, initialized-page, input-producer or native IO grant.
enum {TT_Q38_MIXED_RECURRENT_MATRIX=0x314d5251};
static inline bool PioneerQ38MixedRecurrentGeometry(unsigned layer,unsigned role,unsigned& t,unsigned& k,unsigned& n){
 return layer<64 && layer%4!=3 && PioneerQ38ProjectionGeometry(layer,role,t,k,n);
}
static inline unsigned PioneerQ38MixedRecurrentOffset(unsigned role){
 return role==10?0x100000:role==11?0x101000:role==5?0x102000:role==6?0x108000:
  role==13?0x300000:role==2?0x400000:role==3?0x411000:role==4?0x422000:~0U;
}
static inline bool PioneerQ38MixedRecurrentRecipe(const uint32_t* d,const uint32_t* h,const uint32_t* banks,
 const uint32_t parts[119][8],const uint32_t* first){
 if(!d || !h || !banks || !parts || !first)return false;
 unsigned type=0,k=0,n=0;
 if(!PioneerQ38MixedRecurrentGeometry(d[42],d[43],type,k,n))return false;
 unsigned jobs=n==48?3:119,tile=type==8?576:2304;
 if(d[0]!=TT_Q38_MIXED_RECURRENT_MATRIX || d[1]!=1 || d[2]!=67108864 || d[3]>=7 || (d[4]&4095)
  || d[4]<0x04000000 || d[4]>0xc0000000 || d[5] || !d[6] || d[6]>=0x7fffffff || d[7]!=jobs
  || d[8]!=PioneerQ38ProjectionRoute(d[3]) || d[9]!=129 || d[10]!=PioneerQ38MixedRecurrentOffset(d[43])
  || d[11]!=n || d[12]!=n*4 || d[13]!=TT_Q38_DENSE_MATRIX_ROWS || d[14]!=k || d[15]!=type || !d[16]
  || d[40]>=8192 || d[41]>=248320 || d[44]!=jobs || d[45]!=1
  || h[0]!=TT_Q38_MIXED_RECURRENT_MATRIX || h[1]!=1 || h[2]!=1 || h[3]!=type || h[4]!=k
  || h[5]!=jobs || h[6]!=d[6] || h[7]!=TT_Q38_DENSE_MATRIX_INPUT || h[8]!=1)return false;
 for(unsigned i=9;i<16;i++)if(h[i])return false;
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 for(unsigned i=46;i<64;i++)if(d[i])return false;
 for(unsigned b=0;b<7;b++)if(banks[b]!=PioneerQ38ProjectionRoute(b))return false;
 if(banks[7])return false;
 unsigned rows=0,blocks=k>>(type==8?5:8);
 for(unsigned w=0;w<119;w++){
  const auto* p=parts[w];
  if(w>=jobs){if(first[w])return false;for(unsigned i=0;i<8;i++)if(p[i])return false;continue;}
  if(!p[2] || p[2]>16 || p[3] || !p[5] || p[6] || p[7] || p[1]>=7 || p[4]!=banks[p[1]]
   || p[0]<0x04000000 || p[0]>=0xc4000000 || (p[0]&63) || p[2]*blocks*tile>0xc4000000-p[0]
   || (first[w]&15) || first[w]>=n || p[2]*16>n-first[w]
   || (p[1]==d[3] && p[0]<d[4]+67108864 && d[4]<p[0]+p[2]*blocks*tile))return false;
  for(unsigned old=0;old<w;old++){
   const auto* a=parts[old];
   if((first[w]<first[old]+a[2]*16 && first[old]<first[w]+p[2]*16)
    || (p[1]==a[1] && p[0]<a[0]+a[2]*blocks*tile && a[0]<p[0]+p[2]*blocks*tile))return false;
  }
  rows+=p[2]*16;
 }
 return rows==n;
}
#endif
