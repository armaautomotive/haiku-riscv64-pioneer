/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BFP4_FULL_CPU_BACKEND_H
#define PIONEER_Q38_BFP4_FULL_CPU_BACKEND_H
#include <vector>
// Opaque CPU-only diagnostic backend. Never used by device or Work code.
void* Q38Bfp4CpuOpen(const char* path);
void Q38Bfp4CpuClose(void* context);
bool Q38Bfp4CpuProject(void* context,unsigned layer,unsigned role,
 const std::vector<float>& input,std::vector<float>& output);
#endif
