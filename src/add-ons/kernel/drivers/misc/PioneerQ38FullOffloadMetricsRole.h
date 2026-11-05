/* SPDX-License-Identifier: MIT */
// Read-only integer token progress for the complete full-card backend.
// This is not the old fixed diagnostic endpoint or any terminal/END grant.
#if !defined(PIONEER_Q38_FULL_OFFLOAD_METRICS) || PIONEER_Q38_FULL_OFFLOAD_METRICS != 1 \
 || !defined(PIONEER_Q38_FULL_OFFLOAD_BATCHED) || PIONEER_Q38_FULL_OFFLOAD_BATCHED != 1 \
 || defined(PIONEER_Q38_TOKEN_FRONTIER_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(__FAST_MATH__)
#error Full-card metrics requires its distinct exact batched role,never CPU inference or old diagnostic authority
#endif
#ifdef _KERNEL_MODE
#ifdef PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4
#include "PioneerQ38FullOffloadPreparedQ4Role.h"
#elif defined(PIONEER_Q38_FULL_OFFLOAD_MACRO_Q4)
#include "PioneerQ38FullOffloadMacroQ4Role.h"
#elif defined(PIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4)
#include "PioneerQ38FullOffloadVectorQ4Role.h"
#else
#if _KERNEL_MODE != 1 || !defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || PIONEER_Q38_FULL_OFFLOAD_NATIVE != 1 \
 || !defined(PIONEER_DRIVER_BUILD_TAG) || PIONEER_DRIVER_BUILD_TAG != 0x2026110400010090ULL \
 || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT)
#error Full-card metrics requires the actual tag90 full native parent
#endif
#endif
#else
#if !defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || PIONEER_Q38_FULL_OFFLOAD_CLIENT != 1 \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE)
#error Full-card metrics requires the distinct full-offload USER client
#endif
#endif
