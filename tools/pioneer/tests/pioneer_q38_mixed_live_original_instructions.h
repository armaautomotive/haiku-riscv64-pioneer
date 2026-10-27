/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_LIVE_ORIGINAL_INSTRUCTIONS_TEST_H
#define PIONEER_Q38_MIXED_LIVE_ORIGINAL_INSTRUCTIONS_TEST_H
#include "../PioneerQ38OriginalWeightBytes.h"
// Offline actual-instruction TEST bridges. No native context, private
// source, reference products, device permission or Work entry is exposed.
void ExecuteMixedOriginalEmbedding(const char*,ProjectionExecutionLearnedBytes&,uint32_t*,unsigned);
void ExecuteMixedOriginalBlock(const char*,const char*,const char*,const char*,
 ProjectionExecutionLearnedBytes&,const uint32_t*,unsigned,uint32_t*);
#endif
