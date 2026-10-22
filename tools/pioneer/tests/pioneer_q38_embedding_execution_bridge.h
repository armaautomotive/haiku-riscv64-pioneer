/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_EMBEDDING_EXECUTION_BRIDGE_H
#define PIONEER_Q38_EMBEDDING_EXECUTION_BRIDGE_H
#include <vector>
#include <array>
#include <stdint.h>
// Separate-TU ACTUAL scalar VM bridge;avoid duplicate old unguarded test
// machine includes. This contains NO synthetic completion fabric.
struct EmbeddingExecutionBridge {
 std::vector<uint32_t> l1;
 std::array<uint32_t,5120> resident;
 uint32_t command[10]={},counts[8]={},source[16]={};
 unsigned failPacket=0,steps=0,packets=0,pc=0;
};
void ExecuteEmbeddingBridge(const char* image,EmbeddingExecutionBridge& transfer);
#endif
