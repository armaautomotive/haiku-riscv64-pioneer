/* SPDX-License-Identifier: MIT */
#pragma once
#include <cstdint>
// HOST arithmetic experiment using the actual partial-Blackhole FMA model.
// NOT emitted instructions, an installed kernel or a CPU fallback.
bool Q38Layer29WideReductionCandidate(const float*,const float*,const float*,float,float,float*,float*,uint64_t*);
