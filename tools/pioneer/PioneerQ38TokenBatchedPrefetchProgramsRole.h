/* SPDX-License-Identifier: MIT */
// Re-evaluate on EVERY include;separate DATA/private/diagnostic binding.
#include "PioneerQ38TokenBatchedPrefetchDataBoundary.h"
#ifndef PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PARENT_BINDING_H
#define PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PARENT_BINDING_H
#ifdef PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PROGRAMS
enum {PioneerQ38TokenBatchedPrefetchParentBinding=1};
#else
enum {PioneerQ38TokenBatchedPrefetchParentBinding=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_BATCHED_PREFETCH_BENCHMARK_NATIVE
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenBatchedPrefetchBenchmarkRole.h"
#elif defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_NATIVE_CONTEXT)
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenBatchedPrefetchNativeRole.h"
#endif
#include "PioneerQ38TokenPrefetchProgramsRole.h"
#ifdef PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PROGRAMS
#ifdef PIONEER_Q38_TOKEN_BATCHED_PREFETCH_BENCHMARK_NATIVE
static_assert(PioneerQ38TokenBatchedPrefetchDataSelection==3,"exact separate batched diagnostic binding");
#elif defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_NATIVE_CONTEXT)
static_assert(PioneerQ38TokenBatchedPrefetchDataSelection==2,"exact separate private batched binding");
#else
static_assert(PioneerQ38TokenBatchedPrefetchDataSelection==1,"exact separate batched DATA binding");
#endif
#endif
