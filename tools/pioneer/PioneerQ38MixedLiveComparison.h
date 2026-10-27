/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_LIVE_COMPARISON_H
#define PIONEER_Q38_MIXED_LIVE_COMPARISON_H
#include <stdint.h>
struct PioneerQ38MixedLiveReferenceComparison {
 uint32_t checked=0,nonfinite=0,differences=0,passed=0;
 double maxAbsolute=0,relativeL2=0;
}; // CPU report DATA only; not execution, token, initialization or END permission
#endif
