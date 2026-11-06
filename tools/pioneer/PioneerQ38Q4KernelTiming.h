/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_Q4_KERNEL_TIMING_H
#define PIONEER_Q38_Q4_KERNEL_TIMING_H
#if defined(__STDC_HOSTED__) && !__STDC_HOSTED__
typedef __UINT32_TYPE__ uint32_t;
#else
#include <stdint.h>
#endif
// Diagnostic DATA only. This block is already inside104's complete L1
// journal, but104 treats it as immutable zero. Only a NEW owner may accept
// a timed image/marker and its writes here;this header grants no access.
struct PioneerQ38Q4KernelTiming {
 enum : unsigned {Address=0x98100,PassWords=8,Words=16,Magic=0x314d5451,
  Version=1,Available=0x10001,Unavailable=0x20001}; // QTM1
 static bool Later(const uint32_t* a,const uint32_t* b){
  return a[1]>b[1] || (a[1]==b[1] && a[0]>b[0]);
 }
 static void Record(unsigned k,unsigned pass,const uint32_t* start,bool startValid,
  const uint32_t* end,bool endValid,uint32_t* out){
  const bool valid=startValid && endValid && (start[0]|start[1]) && Later(end,start);
  out[0]=Magic;out[1]=valid?Available:Unavailable;out[2]=k;out[3]=pass;
  for(unsigned i=0;i<2;i++){out[4+i]=valid?start[i]:0;out[6+i]=valid?end[i]:0;}
 }
 static unsigned State(const uint32_t* r,unsigned k,unsigned pass){
  if(!r || (k!=5120 && k!=17408) || pass>=2 || r[0]!=Magic || r[2]!=k || r[3]!=pass)return 0;
  if(r[1]==Available && (r[4]|r[5]) && Later(r+6,r+4))return 1;
  return r[1]==Unavailable && !(r[4]|r[5]|r[6]|r[7])?2:0;
 }
};
#endif
