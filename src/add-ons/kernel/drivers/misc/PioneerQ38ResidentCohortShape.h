/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_COHORT_SHAPE_H
#define PIONEER_Q38_RESIDENT_COHORT_SHAPE_H
#include "PioneerQ38ResidentConvServiceShape.h"
#include "PioneerQ38ResidentGdnServiceShape.h"
// OFFLINE immutable schedule DATA,NOT authenticated model/initialized
// history/cohort/clock/native ownership or a current-slot/token commit.
enum {TT_Q38_RESIDENT_COHORT_MARKER=0x31524351,TT_Q38_RESIDENT_COHORT_FAULT=0x46524351,
 TT_Q38_COHORT_SEQUENCE_WORD=64,TT_Q38_COHORT_SOURCE_WORD=192,
 TT_Q38_COHORT_CONV_WORD=2112,TT_Q38_COHORT_GDN_WORD=48192,
 TT_Q38_COHORT_WORDS=66624,TT_Q38_COHORT_MANIFEST=0xc8000,
 TT_Q38_COHORT_SHADOW=0x109100,TT_Q38_COHORT_COUNTERS=0x14a400,
 TT_Q38_COHORT_SEQUENCE=0x14b300,TT_Q38_COHORT_STATS=0x14b500};
static inline const uint32_t* PioneerQ38CohortSource(const uint32_t* m,unsigned w){return m+TT_Q38_COHORT_SOURCE_WORD+w*16;}
static inline const uint32_t* PioneerQ38CohortConv(const uint32_t* m,unsigned w){return m+TT_Q38_COHORT_CONV_WORD+w*384;}
static inline const uint32_t* PioneerQ38CohortGdn(const uint32_t* m,unsigned w){return m+TT_Q38_COHORT_GDN_WORD+(w-1)*192;}
static inline void PioneerQ38CohortJob(const uint32_t* m,unsigned w,unsigned op,uint32_t* j){
 for(unsigned i=0;i<16;i++)j[i]=0;
 j[0]=op==7?uint32_t(TT_Q38_RESIDENT_CONV_SERVICE_JOB):uint32_t(TT_Q38_RESIDENT_GDN_SERVICE_JOB);
 j[1]=op==7?PioneerQ38ConvBatchCount(w):64;j[2]=op==7?1536:768;
 j[3]=m[op==7?5:6];j[10]=TT_Q38_FFN_PERSISTENT_MARKER;j[11]=w;
 j[12]=m[64+w-1]+(op==7?1:2);j[13]=op;
}
static inline bool PioneerQ38ResidentCohortShape(const uint32_t* m){
 if(!m || m[0]!=TT_Q38_RESIDENT_COHORT_MARKER || m[1]!=1 || m[2]!=TT_Q38_COHORT_WORDS
  || m[3]>=64 || m[3]%4==3 || m[4]>=8192 || !m[5] || m[5]>=0x7ffffffe
  || m[6]!=m[5]+1 || m[7]>=7 || (m[8]&4095) || m[8]<0x04000000 || m[8]>0xc0000000
  || m[9]!=PioneerQ38ProjectionRoute(m[7]) || m[10]!=129 || m[11]!=119 || m[12]!=96
  || m[13]!=640 || m[14]>1 || m[15] || m[40]!=120 || m[41]!=384 || m[42]!=192 || m[43])return false;
 for(unsigned i=44;i<64;i++)if(m[i])return false;
 for(unsigned i=119;i<128;i++)if(m[64+i])return false;
 for(unsigned i=32;i<40;i+=2)if(!(m[i]|m[i+1]))return false;
 for(unsigned i=0;i<16;i++)if(m[16+i]!=PioneerQ38CohortSource(m,0)[i])return false;
 for(unsigned w=0;w<120;w++){
  const auto* d=PioneerQ38CohortConv(m,w);const auto* src=PioneerQ38CohortSource(m,w);
  if(!src[0] || !PioneerQ38ConvBatchShape(d) || d[10]!=w || d[5]!=m[3] || d[40]!=m[4]
   || d[6]!=m[5] || d[3]!=m[7] || d[4]!=m[8] || d[8]!=m[9])return false;
  for(unsigned i=0;i<16;i++)if(d[16+i]!=src[i])return false;
  for(unsigned i=0;i<8;i++)if(d[32+i]!=m[32+i])return false;
  if(!w)continue;
  if(m[64+w-1]>=0x7ffffffc || m[64+w-1]+1>m[5])return false;
  uint32_t j[16];PioneerQ38CohortJob(m,w,7,j);
  if(!PioneerQ38ResidentConvServiceJob(j,d,src,w,PioneerQ38ConvPeer(w)))return false;
  if(w>96)continue;
  const auto* input=PioneerQ38CohortGdn(m,w);const auto* state=input+64;const auto* gated=state+64;
  PioneerQ38CohortJob(m,w,8,j);
  if(!PioneerQ38ResidentGdnServiceJob(j,gated,input,state,src,w,PioneerQ38ConvPeer(w))
   || gated[5]!=m[3] || gated[40]!=m[4] || gated[6]!=m[6]
   || gated[3]!=m[7] || gated[4]!=m[8] || state[11]!=m[14])return false;
  for(unsigned i=0;i<8;i++)if(gated[32+i]!=m[32+i])return false;
 }
 return true; // geometry only;NO owner/page/source/clock/producer capability
}
static_assert(TT_Q38_COHORT_MANIFEST+TT_Q38_COHORT_WORDS*4==TT_Q38_COHORT_SHADOW
 && TT_Q38_COHORT_SHADOW+TT_Q38_COHORT_WORDS*4<=TT_Q38_COHORT_COUNTERS
 && TT_Q38_COHORT_COUNTERS+119*32<=TT_Q38_COHORT_SEQUENCE
 && TT_Q38_COHORT_SEQUENCE+119*4<=TT_Q38_COHORT_STATS
 && TT_Q38_COHORT_STATS+256<0x17ffc0,"NEW explicit root schedule footprint,NOT existing native grant");
#endif
