/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_MATRIX_PHASE_DATA_H
#define PIONEER_Q38_TOKEN_MATRIX_PHASE_DATA_H
#if defined(__STDC_HOSTED__) && !__STDC_HOSTED__
typedef __UINT32_TYPE__ uint32_t;
typedef __UINT64_TYPE__ uint64_t;
#else
#include <stdint.h>
#endif
// Pure observation grammar ONLY. Does NOT establish freshness,code/source/
// run/job ownership,completed inference,HOLD,clock frequency or cleanup.
// The caller must obtain worker/STOP nonce independently of this record.
// A bounded nonce is NOT an independently predicted latest-job identity.
namespace PioneerQ38TokenMatrixPhaseData {
enum {Address=0x84500,Words=16,Magic=0x324d5051,Version=2,
 Missing=0,InvalidClock=1,Intervals=2,Malformed=3};
static inline uint64_t Get(const uint32_t* p,unsigned i){return uint64_t(p[i])|(uint64_t(p[i+1])<<32);}
static inline unsigned Classify(const uint32_t (&p)[Words],unsigned worker,unsigned stopNonce){
 if(!worker || worker>=120 || !stopNonce || stopNonce>=0x7fffffff)return Malformed;
 uint32_t any=0;for(unsigned i=0;i<Words;i++)any|=p[i];
 if(!any)return Missing;
 if(p[0]!=Magic || p[1]!=Version || !p[2] || p[2]>=stopNonce || p[3]!=worker || p[4]!=12
  || (p[5]!=5120 && p[5]!=6144 && p[5]!=17408) || !p[6] || p[6]>16 || p[7]>1 || p[14] || p[15])return Malformed;
 uint64_t total=0;
 for(unsigned i=8;i<14;i+=2){
  const uint64_t part=Get(p,i);
  if((p[7]?part==0:part!=0) || part>~uint64_t(0)-total)return Malformed;
  total+=part;
 }
 return p[7]?Intervals:InvalidClock;
}
}
#endif
