/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_OUTPUT_COHORT_SHAPE_H
#define PIONEER_Q38_RESIDENT_OUTPUT_COHORT_SHAPE_H
#include "PioneerQ38ResidentCohortShape.h"
#include "PioneerQ38ResidentGdnOutputShape.h"
#include "PioneerQ38ProjectionMatrixShape.h"
// Immutable canonical-part GEOMETRY only. Native code/model/source/init,
// clock, complete cohort ownership and token/state commit are STILL OWED.
enum {TT_Q38_RESIDENT_OUTPUT_COHORT_MARKER=0x314f4351,TT_Q38_RESIDENT_OUTPUT_COHORT_FAULT=0x464f4351,
 TT_Q38_OUTPUT_COHORT_WORDS=1152,TT_Q38_OUTPUT_COHORT_MANIFEST=0x14b600,
 TT_Q38_OUTPUT_COHORT_SHADOW=0x14c800,TT_Q38_OUTPUT_COHORT_PART=64,TT_Q38_OUTPUT_COHORT_ROW=1016};
static inline bool PioneerQ38ResidentOutputCohortShape(const uint32_t* q,const uint32_t* m){
 if(!q || !m || q[0]!=TT_Q38_RESIDENT_OUTPUT_COHORT_MARKER || q[1]!=1 || q[2]!=TT_Q38_OUTPUT_COHORT_WORDS
  || q[3]!=m[3] || q[4]!=m[4] || m[6]>=0x7ffffffb || q[5]!=m[6]+1 || q[6]!=q[5]+1 || q[7]!=q[6]+1
  || q[8]!=m[7] || q[9]!=m[8] || q[10]!=m[9] || q[11]>=248320 || q[12]!=6144 || q[13]!=5120 || q[14]!=119 || q[15])return false;
 for(unsigned i=16;i<40;i++)if(q[i]!=m[i])return false;
 for(unsigned i=40;i<64;i++)if(q[i])return false;
 for(unsigned i=1135;i<1152;i++)if(q[i])return false;
 unsigned groups=0;
 for(unsigned w=0;w<119;w++){
  const auto* p=q+64+w*8;unsigned first=q[1016+w];
  if(p[0]<0x04000000 || p[0]>=0xc4000000 || (p[0]&63) || p[1]>=7 || !p[2] || p[2]>16
   || p[2]*110592>0xc4000000-p[0] || p[3] || p[4]!=PioneerQ38ProjectionRoute(p[1])
   || !p[5] || p[5]!=PioneerQ38CohortSource(m,w+1)[0] || p[6] || p[7]
   || first>=5120 || (first&15) || p[2]*16>5120-first)return false;
  for(unsigned old=0;old<w;old++){
   const auto* a=q+64+old*8;
   if(first<q[1016+old]+a[2]*16 && q[1016+old]<first+p[2]*16)return false;
   if(p[1]==a[1] && p[0]<a[0]+a[2]*110592 && a[0]<p[0]+p[2]*110592)return false;
  }
  groups+=p[2];
 }
 return groups==320; // bounded disjoint total5120 proves complete row cover.
}
static inline void PioneerQ38ResidentOutputDescriptor(const uint32_t* m,const uint32_t* q,unsigned w,uint32_t* d){
 for(unsigned i=0;i<64;i++)d[i]=0;
 d[0]=TT_Q38_RESIDENT_GDN_OUTPUT_MARKER;d[1]=1;d[2]=67108864;d[3]=m[7];d[4]=m[8];d[5]=w-1;d[6]=q[5];
 d[7]=w;d[8]=m[9];d[9]=PioneerQ38ConvPeer(w);d[10]=TT_Q38_GDN_GATED_OUTPUT+(w-1)*1024;d[11]=6144;
 d[12]=TT_Q38_PROJECTION_NATIVE_OFFSET+(w-1)*512;d[13]=TT_Q38_PROJECTION_LINES_OFFSET+(w-1)*128;d[14]=8;d[15]=24;
 for(unsigned i=0;i<16;i++)d[16+i]=PioneerQ38CohortSource(m,w)[i];
 for(unsigned i=32;i<40;i++)d[i]=m[i];
 d[40]=m[4];d[41]=m[6];d[42]=m[3];d[43]=13;d[44]=TT_Q38_GDN_RENDEZVOUS_MARKER;d[45]=q[11];
}
static_assert(TT_Q38_COHORT_STATS+256==TT_Q38_OUTPUT_COHORT_MANIFEST
 && TT_Q38_OUTPUT_COHORT_MANIFEST+TT_Q38_OUTPUT_COHORT_WORDS*4==TT_Q38_OUTPUT_COHORT_SHADOW
 && TT_Q38_OUTPUT_COHORT_SHADOW+TT_Q38_OUTPUT_COHORT_WORDS*4<0x17ffc0,"NEW fixed post-manifest/immutable shadow;NO old native footprint grant");
#endif
