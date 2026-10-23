/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_LEARNED_RMS_EXECUTION_BRIDGE_H
#define PIONEER_Q38_LEARNED_RMS_EXECUTION_BRIDGE_H
#include <vector>
#include <array>
#include <stdint.h>
struct LearnedRmsExecutionBridge {
 std::vector<uint32_t> l1;
 std::array<uint32_t,5120> resident,backup;
 uint32_t command[10]={},counts[8]={},source[16]={};
 unsigned failPacket=0,badCounter=0,sourceFault=0,descriptorFault=0,steps=0,packets=0,pc=0;
 const uint32_t* originalLearnedWeights=nullptr; // TEST-only5120 original F32 weight bits,NOT normalized output
};
void ExecuteLearnedRmsBridge(const char* image,LearnedRmsExecutionBridge& transfer);
#endif
