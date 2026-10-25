/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_LAYER_SHAPE_H
#define PIONEER_Q38_RESIDENT_LAYER_SHAPE_H
#include "PioneerQ38ResidentOutputCohortShape.h"
#include "PioneerQ38FfnResidualGraphShape.h"
// NEW complete recurrent-layer schedule DATA,not an owner/producer/IO grant.
// Existing68 has neither this footprint nor its residual/FFN capability.
enum {TT_Q38_RESIDENT_LAYER_MARKER=0x31524c51,TT_Q38_RESIDENT_LAYER_FAULT=0x46524c51,
 TT_Q38_RESIDENT_LAYER_WORDS=3520,TT_Q38_RESIDENT_LAYER_MANIFEST=0x14e000,
 TT_Q38_RESIDENT_LAYER_SHADOW=0x152000,TT_Q38_RESIDENT_LAYER_NORM_ROM=0x158000,
 TT_Q38_RESIDENT_LAYER_RECEIPT=0x155800,TT_Q38_RESIDENT_LAYER_RECEIPT_WORDS=256,
 TT_Q38_RESIDENT_LAYER_RECEIPT_MARKER=0x32504c51,
 TT_Q38_RESIDENT_LAYER_GDN_SAVED=0x155c00,TT_Q38_RESIDENT_LAYER_GDN_SAVED_WORDS=68*32,
 TT_Q38_RESIDENT_LAYER_FFN_ROM=0x15c000,TT_Q38_RESIDENT_LAYER_ROM_END=0x17e000,
 TT_Q38_RESIDENT_LAYER_NORM_MARKER=0x314e4c51};
static inline bool PioneerQ38ResidentLayerShape(const uint32_t* l,const uint32_t* m,const uint32_t* q){
 if(!l || !m || !q || !PioneerQ38ResidentCohortShape(m) || !PioneerQ38ResidentOutputCohortShape(q,m)
  || m[5]>=0x7ffffff0 || l[0]!=TT_Q38_RESIDENT_LAYER_MARKER || l[1]!=1 || l[2]!=TT_Q38_RESIDENT_LAYER_WORDS
  || l[3]!=m[3] || l[4]!=m[4] || l[5]!=m[5] || l[6]!=q[7]+1 || l[7]!=q[7]+2
  || l[8]!=m[7] || l[9]!=m[8] || l[10]!=m[9] || l[11]!=q[11]
  || l[12]>=7 || (l[13]&4095) || l[13]<0x04000000 || l[13]>0xc3ffb000
  || l[14]!=PioneerQ38ProjectionRoute(l[12]) || l[15]!=20480
  || (l[12]==l[8] && l[13]+20480>l[9])
  || l[40]!=TT_Q38_PROJECTION_OUTPUT_OFFSET || l[41]!=98304 || l[42]!=5120
  || l[43]!=TT_Q38_RESIDENT_OUTPUT_COHORT_MARKER || l[44]!=q[7] || l[45]!=0x358637bd
  || l[46]!=TT_Q38_FFN_RESIDUAL_GRAPH_MARKER || l[47]!=TT_Q38_RESIDENT_LAYER_NORM_MARKER)return false;
 for(unsigned i=16;i<40;i++)if(l[i]!=m[i])return false;
 for(unsigned i=48;i<64;i++)if(l[i])return false;
 const auto* f=l+64;
 if(!PioneerQ38FfnResidualGraphManifest(f,m+TT_Q38_COHORT_SOURCE_WORD+16)
  || f[3]!=l[8] || f[4]!=l[9] || f[5]!=1 || f[6]!=l[7]
  || f[40]!=l[4] || f[41]!=l[11] || f[42]!=l[3])return false;
 for(unsigned i=16;i<40;i++)if(f[i]!=l[i])return false;
 for(unsigned w=0;w<119;w++)if(f[64+w])return false;
 return true; // zero placeholders are REPLACED by actual private peer sequences ON CARD
}
static inline bool PioneerQ38ResidentLayerNormShape(const uint32_t* d){
 if(!d || d[0]!=TT_Q38_RESIDENT_LAYER_NORM_MARKER || d[1]!=1 || d[2]!=67108864 || d[3]>=7
  || (d[4]&4095) || d[4]<0x04000000 || d[4]>0xc0000000 || d[5] || !d[6] || d[6]>=0x7fffffff
  || d[7]!=1 || d[8]!=PioneerQ38ProjectionRoute(d[3]) || d[9]!=129 || d[10]>=7
  || (d[11]&4095) || d[11]<0x04000000 || d[11]>0xc3ffb000 || d[12]!=PioneerQ38ProjectionRoute(d[10])
  || d[13]!=20480 || d[14]!=5120 || d[15]!=TT_Q38_PROJECTION_OUTPUT_OFFSET || !d[16]
  || d[40]>=64 || d[40]%4==3 || d[41]>=248320 || d[42]!=98304 || d[43]!=20480 || d[44]!=0x358637bd
  || d[45]!=5120 || d[46]!=6 || d[47]!=4 || d[48]!=960 || d[49]!=640 || d[50]>=8192
  || d[51]!=98304 || d[52]!=TT_Q38_RESIDENT_OUTPUT_COHORT_MARKER || d[53]!=d[6]-1
  || d[54]!=TT_Q38_RESIDENT_LAYER_MARKER)return false;
 if(d[3]==d[10] && d[11]+20480>d[4])return false;
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 for(unsigned i=55;i<64;i++)if(d[i])return false;
 return true;
}
static inline void PioneerQ38ResidentLayerNormDescriptor(const uint32_t* l,uint32_t* d){
 for(unsigned i=0;i<64;i++)d[i]=0;
 d[0]=TT_Q38_RESIDENT_LAYER_NORM_MARKER;d[1]=1;d[2]=67108864;d[3]=l[8];d[4]=l[9];d[6]=l[6];d[7]=1;d[8]=l[10];d[9]=129;
 d[10]=l[12];d[11]=l[13];d[12]=l[14];d[13]=20480;d[14]=5120;d[15]=TT_Q38_PROJECTION_OUTPUT_OFFSET;
 for(unsigned i=16;i<40;i++)d[i]=l[i];
 d[40]=l[3];d[41]=l[11];d[42]=98304;d[43]=20480;d[44]=0x358637bd;d[45]=5120;
 d[46]=6;d[47]=4;d[48]=960;d[49]=640;d[50]=l[4];d[51]=98304;
 d[52]=TT_Q38_RESIDENT_OUTPUT_COHORT_MARKER;d[53]=l[44];d[54]=TT_Q38_RESIDENT_LAYER_MARKER;
}
static_assert(TT_Q38_OUTPUT_COHORT_SHADOW+TT_Q38_OUTPUT_COHORT_WORDS*4<=TT_Q38_RESIDENT_LAYER_MANIFEST
 && TT_Q38_RESIDENT_LAYER_MANIFEST+TT_Q38_RESIDENT_LAYER_WORDS*4<=TT_Q38_RESIDENT_LAYER_SHADOW
 && TT_Q38_RESIDENT_LAYER_SHADOW+TT_Q38_RESIDENT_LAYER_WORDS*4<=TT_Q38_RESIDENT_LAYER_RECEIPT
 && TT_Q38_RESIDENT_LAYER_RECEIPT+TT_Q38_RESIDENT_LAYER_RECEIPT_WORDS*4<=TT_Q38_RESIDENT_LAYER_GDN_SAVED
 && TT_Q38_RESIDENT_LAYER_GDN_SAVED+TT_Q38_RESIDENT_LAYER_GDN_SAVED_WORDS*4<=TT_Q38_RESIDENT_LAYER_NORM_ROM
 && TT_Q38_RESIDENT_LAYER_NORM_ROM+0x4000==TT_Q38_RESIDENT_LAYER_FFN_ROM
 && TT_Q38_RESIDENT_LAYER_ROM_END<=0x17ffc0,"NEW ROOT data+ROM footprint;NEVER a68 snapshot grant");
#endif
