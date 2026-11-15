/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FULL_OFFLOAD_COUNTER_CADENCE_H
#define PIONEER_Q38_FULL_OFFLOAD_COUNTER_CADENCE_H
#include <cstdint>
#include <climits>

// Read-only progress telemetry is expensive: the native observer re-seals
// the immutable ROOT image on every call. Keep this cadence separate from
// the terminal/cleanup polling cadence.
class PioneerQ38FullOffloadCounterCadence {
 static constexpr int64_t IntervalUs=250000;
 int64_t nextUs=0;
 bool scheduled=false;
public:
 bool Due(int64_t nowUs)const{return nowUs>=0 && (!scheduled || nowUs>=nextUs);}
 bool Record(int64_t completedUs){
  if(completedUs<0 || completedUs>INT64_MAX-IntervalUs)return false;
  nextUs=completedUs+IntervalUs;scheduled=true;return true;
 }
};
#endif
