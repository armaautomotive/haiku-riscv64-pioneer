/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_BOUNDARY_DELAY_H
#define PIONEER_Q38_TOKEN_BOUNDARY_DELAY_H
#include <stdint.h>
// One bounded healthy-trial delay,not a target-layer selector or a retry.
template<class Clock> static bool PioneerQ38TokenBoundaryDelay(Clock& clock,unsigned milliseconds,int64_t& finished){
 if(milliseconds>5000)return false;
 const int64_t duration=int64_t(milliseconds)*1000,before=clock.Now();
 if(before<0 || before>INT64_MAX-duration || (duration && !clock.Sleep(duration)))return false;
 const int64_t after=clock.Now();
 if(after<before || after-before<duration)return false;
 finished=after;return true;
} // failure preserves the caller output;no cancellation,device call or cleanup
#endif
