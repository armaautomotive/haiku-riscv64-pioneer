/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RECURRENT_PREFIX_COMPARISON_H
#define PIONEER_Q38_RECURRENT_PREFIX_COMPARISON_H
#include "PioneerQ38RecurrentBlockComparison.h"
// Neutral TEST output sink only. No expected input/reference products,
// initialized-state/source/driver/native ownership or release capability.
struct PrefixOriginalStageSink {
 virtual ~PrefixOriginalStageSink()=default;
 virtual bool Publish(unsigned,PioneerRecurrentOriginalStage,const uint32_t*,unsigned)=0;
};
#endif
