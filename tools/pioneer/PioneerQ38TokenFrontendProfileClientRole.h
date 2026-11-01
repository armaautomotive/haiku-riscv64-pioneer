/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT) || PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_FRONTEND_PROFILE_NATIVE) || defined(PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(__FAST_MATH__)
#error Exact separate USER frontend-snapshot profile required,not native,CPU or Work
#endif
#include "PioneerQ38TokenFrontierBenchmarkClientRole.h"
