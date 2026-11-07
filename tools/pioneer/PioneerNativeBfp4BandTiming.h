/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_BFP4_BAND_TIMING_H
#define PIONEER_NATIVE_BFP4_BAND_TIMING_H
// Fixed DATA contract for a distinct timed synthetic BFP4 band image. The
// 64-byte area is at the bottom of the owner's already-snapshotted 8KiB
// stack span; a timed image must prove its real stack never enters it.
// This is not a device owner, CPU validation, Work selection or speed claim.
struct PioneerNativeBfp4BandTiming {
 enum : unsigned {Address=0x6e000,PassWords=8,Words=16,Magic=0x34544d51,
  Available=0x10001,Unavailable=0x20001}; // QMT4
 static bool Later(const unsigned* a,const unsigned* b){return a[1]>b[1] || (a[1]==b[1] && a[0]>b[0]);}
 static void Record(unsigned k,unsigned pass,const unsigned* start,bool startValid,const unsigned* end,bool endValid,unsigned* out){
  bool valid=startValid && endValid && (start[0]|start[1]) && Later(end,start);
  out[0]=Magic;out[1]=valid?Available:Unavailable;out[2]=k;out[3]=pass;
  for(unsigned i=0;i<2;i++){out[4+i]=valid?start[i]:0;out[6+i]=valid?end[i]:0;}
 }
 static unsigned State(const unsigned* r,unsigned k,unsigned pass){
  if(!r || (k!=5120 && k!=6144 && k!=17408) || pass>=2 || r[0]!=Magic || r[2]!=k || r[3]!=pass)return 0;
  if(r[1]==Available && (r[4]|r[5]) && Later(r+6,r+4))return 1;
  return r[1]==Unavailable && !(r[4]|r[5]|r[6]|r[7])?2:0;
 }
};
#endif
