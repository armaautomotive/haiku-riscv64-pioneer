/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_LIVE_ORIGINAL_INSTRUCTIONS_TEST_H
#define PIONEER_Q38_MIXED_LIVE_ORIGINAL_INSTRUCTIONS_TEST_H
#include "../PioneerQ38OriginalWeightBytes.h"
#include "../PioneerQ38MixedLiveComparison.h"
#include <array>
#include <vector>
struct MixedOriginalInstructionTrace {
 std::array<std::vector<uint32_t>,PioneerQ38MixedStageCount> actual;
}; // ACTUAL output snapshots only; never an input, reference or native grant.
// Offline actual-instruction TEST bridges. No native context, private
// source, reference products, device permission or Work entry is exposed.
void ExecuteMixedOriginalEmbedding(const char*,ProjectionExecutionLearnedBytes&,uint32_t*,unsigned);
void ExecuteMixedOriginalBlock(const char*,const char*,const char*,const char*,
 ProjectionExecutionLearnedBytes&,const uint32_t*,unsigned,uint32_t*,MixedOriginalInstructionTrace&);
#endif
