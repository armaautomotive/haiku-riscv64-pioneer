/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_DRAM_COHORT_HOST_BRACKET_H
#define PIONEER_DRAM_COHORT_HOST_BRACKET_H
#include <stdint.h>

// DATA only: observations must be actual aligned, uncached device reads made
// under the SAME live native scope. A valid result is published only AFTER
// the complete owner has independently checked reports/counters and restored.
struct PioneerDramCohortHostTiming {
 uint32_t version=1,state=0; // 0 pending,1 bracketed,2 unavailable,3 malformed
 uint64_t startLowerUs=0,startUpperUs=0,endLowerUs=0,endUpperUs=0;
 uint64_t elapsedMinUs=0,elapsedMaxUs=0,cycles=0,bytes=0;
};
class PioneerDramCohortHostBracket {
 uint32_t nonce=0,seen=0;
 uint64_t last=0,startLower=0,startUpper=0,endLower=0,endUpper=0;
 bool malformed=false,armed=false;
 bool Bad(){malformed=true;return false;}
public:
 enum {Ready=0,Start=1,Done=2,Payload=3};
 bool Arm(uint32_t n,uint64_t beforeRelease){
  if(armed || !n || !beforeRelease || beforeRelease==UINT64_MAX)return Bad();
  armed=true;nonce=n;last=startLower=beforeRelease;return true;
 }
 bool Observe(unsigned marker,uint32_t value,uint64_t before,uint64_t after){
  if(!armed || malformed || marker>Payload || before<last || after<before || after==UINT64_MAX)return Bad();
  last=after;
  if(marker<Payload){
   const unsigned bit=1U<<marker;
   if(value && value!=nonce)return Bad();
   if(!value){
    if(seen&bit)return Bad(); // flags are monotonic for this one launch
    if(marker==Ready)startLower=before;
    if(marker==Done)endLower=before;
   }else{
    if(marker==Start && !(seen&bit))startUpper=after+1;
    seen|=bit;
   }
  }else{
   if(value!=0 && value!=nonce && value!=~nonce)return Bad();
   if(value==nonce){
    if(seen&16)return Bad();
    seen|=8; // actual payload baseline,not merely a presumed initial value
   }else if(value==~nonce && (seen&14)==14){
    if(!(seen&16))endUpper=after+1;
    seen|=16;
   }else if(!value && value!=~nonce && (seen&8)){
    return Bad();
   }else if(value && value!=~nonce)return Bad();
   // ~FFFFFFFF is zero: never mistake pristine zero for an end marker.
   // A missed baseline/transition is unavailable timing,not a guessed rate.
  }
  return true;
 }
 void Finish(uint64_t cycleDelta,uint64_t transferred,PioneerDramCohortHostTiming& r)const{
  r.startLowerUs=startLower;r.startUpperUs=startUpper;r.endLowerUs=endLower;r.endUpperUs=endUpper;
  r.cycles=cycleDelta;r.bytes=transferred;
  r.state=malformed?3:2;
  if(!armed || malformed || seen!=31 || !cycleDelta || !transferred
   || startUpper<startLower || endUpper<endLower || endLower<=startUpper || endUpper<=startLower)return;
  r.elapsedMinUs=endLower-startUpper;r.elapsedMaxUs=endUpper-startLower;r.state=1;
 }
};
// For state1 ONLY: frequency Hz is [cycles*1e6/maxUs,cycles*1e6/minUs],
// throughput B/s is [bytes*1e6/maxUs,bytes*1e6/minUs]. Use checked arithmetic
// or floating point OFFBOARD. Never use the firmware MHz to narrow bounds.
// The +1us upper censor accommodates system_time's integer-us resolution;
// no claim about clock calibration finer than the actual host clock follows.
#endif
