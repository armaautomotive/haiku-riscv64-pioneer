/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_WORK_TIMING_H
#define PIONEER_Q38_WORK_TIMING_H
#include <stdint.h>
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38RootCycleData.h"

// Observation only: host-monotonic time spent inside Work ioctl calls.
// This is NOT card-only time and never authorizes or changes computation.
struct PioneerQ38WorkTiming {
 uint64_t calls=0,nanos=0,pairedBatches=0;unsigned samples=0;
 uint64_t rootSamples=0,rootCycles[3]={};bool rootCycleDataValid=true;
 // Optional raw device counters are observational only. A missing signature,
 // overflow, or malformed delta makes the profile unavailable, never fails
 // or changes Work execution. These are ROOT cycles, not Tensix compute time.
 void ObserveRoot(uint64_t beforeSamples,uint64_t afterSamples,const uint64_t before[3],
  const uint64_t after[3],bool beforeValid,bool afterValid){
  if(!rootCycleDataValid)return;
  if(!beforeValid || !afterValid || afterSamples<beforeSamples){rootCycleDataValid=false;return;}
  uint64_t sampleDelta=afterSamples-beforeSamples;
  if(rootSamples>UINT64_MAX-sampleDelta){rootCycleDataValid=false;return;}
  uint64_t delta[3];
  for(unsigned i=0;i<3;i++){
   if(after[i]<before[i]){rootCycleDataValid=false;return;}
   delta[i]=after[i]-before[i];
   if(rootCycles[i]>UINT64_MAX-delta[i]){rootCycleDataValid=false;return;}
  }
  rootSamples+=sampleDelta;for(unsigned i=0;i<3;i++)rootCycles[i]+=delta[i];
 }
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
