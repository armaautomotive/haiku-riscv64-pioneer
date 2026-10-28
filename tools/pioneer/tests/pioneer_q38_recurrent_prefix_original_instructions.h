/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RECURRENT_PREFIX_ORIGINAL_INSTRUCTIONS_TEST_H
#define PIONEER_Q38_RECURRENT_PREFIX_ORIGINAL_INSTRUCTIONS_TEST_H
#include "../PioneerQ38RecurrentOriginalWeightBytes.h"
#include "../PioneerQ38RecurrentBlockComparison.h"
#include <array>
#include <vector>
// Actual instruction OUTPUT snapshots only. No reference/native capability.
struct PrefixOriginalInstructionTrace {
 std::array<std::array<std::vector<uint32_t>,PioneerRecurrentStageCount>,3> actual;
};
void ExecutePrefixOriginalEmbedding(const char*,RecurrentBlockRawLearnedBytes&,uint32_t*,unsigned);
void ExecutePrefixOriginalInstructions(const char*,const char*,RecurrentBlockRawLearnedBytes&,const uint32_t*,unsigned,PrefixOriginalInstructionTrace&);
#endif
