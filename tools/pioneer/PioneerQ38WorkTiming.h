/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_WORK_TIMING_H
#define PIONEER_Q38_WORK_TIMING_H
#include <stdint.h>

// Observation only: host-monotonic time spent inside Work ioctl calls.
// This is NOT card-only time and never authorizes or changes computation.
struct PioneerQ38WorkTiming {
 uint64_t calls=0,nanos=0,pairedBatches=0;unsigned samples=0;
 bool Add(uint64_t beforeCalls,uint64_t afterCalls,uint64_t beforeNanos,uint64_t afterNanos){
  return Add(beforeCalls,afterCalls,beforeNanos,afterNanos,0,0);
 }
 bool Add(uint64_t beforeCalls,uint64_t afterCalls,uint64_t beforeNanos,uint64_t afterNanos,
  uint64_t beforePairs,uint64_t afterPairs){
  if(afterCalls<beforeCalls || afterNanos<beforeNanos || afterPairs<beforePairs)return false;
  const uint64_t c=afterCalls-beforeCalls,n=afterNanos-beforeNanos,p=afterPairs-beforePairs;
  if(!c || calls>UINT64_MAX-c || nanos>UINT64_MAX-n || pairedBatches>UINT64_MAX-p || samples==UINT32_MAX)return false;
  calls+=c;nanos+=n;pairedBatches+=p;samples++;return true;
 }
};
#endif
