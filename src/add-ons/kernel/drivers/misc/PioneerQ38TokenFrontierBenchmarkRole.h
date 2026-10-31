/* SPDX-License-Identifier: MIT */
// Separate bounded physical diagnostic. Tag75 is ABI,not module identity.
#if !defined(PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_NATIVE) || PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_NATIVE != 1 \
 || !defined(PIONEER_Q38_TOKEN_FRONTIER_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_FRONTIER_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS) || PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_DECODE_BENCHMARK_NATIVE) || PIONEER_Q38_TOKEN_DECODE_BENCHMARK_NATIVE != 1 \
 || defined(PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT) || defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY)
#error Exact separate physical frontier diagnostic required,not old physical or private NODRV
#endif
#include "PioneerQ38TokenDecodeBenchmarkRole.h"
#ifndef PIONEER_Q38_TOKEN_FRONTIER_PHYSICAL_BINDING_H
#define PIONEER_Q38_TOKEN_FRONTIER_PHYSICAL_BINDING_H
enum {PioneerQ38TokenFrontierPhysicalSelection=1};
#endif
static_assert(PioneerQ38TokenFrontierPhysicalSelection==1,"late physical frontier selection");
