/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_HEAD_BFP8_CPU_BACKEND_H
#define PIONEER_Q38_HEAD_BFP8_CPU_BACKEND_H
#include <cstdint>
#include <vector>
// CPU-only complete-vocabulary observation; no device, Work or fallback.
bool Q38HeadBfp8CpuObserve(const char* path,const std::vector<uint32_t>& input,std::vector<uint32_t>& output);
#endif
