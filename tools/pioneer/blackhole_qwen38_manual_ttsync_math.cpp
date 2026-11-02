/* SPDX-License-Identifier: MIT */
// Separate complete returning shader prototype,NOT selected by a driver.
#define PIONEER_Q38_MANUAL_TTSYNC_PARENT_BINDING 1
#include "blackhole_qwen38_q4_constant_issue_math.cpp"
static_assert(PioneerQ38ManualTTSyncSelection==1,"manual completion TU cannot fall back");
