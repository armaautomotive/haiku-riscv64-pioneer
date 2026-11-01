/* SPDX-License-Identifier: MIT */
// Fixed6607+6608 diagnostic BUILD only. Whole module SHA,not the shared
// tag75,is identity. Native execution and safe staging remain separate.
#if !defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_BENCHMARK_NATIVE) || PIONEER_Q38_TOKEN_BATCHED_PREFETCH_BENCHMARK_NATIVE != 1 \
 || !defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PROGRAMS) || PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_PREFETCH_BENCHMARK_NATIVE) || PIONEER_Q38_TOKEN_PREFETCH_BENCHMARK_NATIVE != 1 \
 || !defined(PIONEER_Q38_TOKEN_PREFETCH_PROGRAMS) || PIONEER_Q38_TOKEN_PREFETCH_PROGRAMS != 1 \
 || defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_PREFETCH_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_WORKER_IMAGE) || defined(PIONEER_Q38_WORK_BATCHED_PREFETCH) \
 || defined(PIONEER_Q38_BATCHED_PREFETCH_ROOT_IMAGE) || defined(PIONEER_Q38_TOKEN_PREFETCH_WORKER_IMAGE) \
 || defined(PIONEER_Q38_WORK_TILE_PREFETCH) || defined(PIONEER_Q38_FLAT_MATRIX_GRAMMAR_IMAGE)
#error Exact separate batched diagnostic required,never DATA,private NODRV or shader BUILD
#endif
static_assert(PioneerQ38TokenBatchedPrefetchParentBinding==1,"batched public role requires its own outer package binding");
#include "PioneerQ38TokenPrefetchBenchmarkRole.h"
#ifndef PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PHYSICAL_BINDING_H
#define PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PHYSICAL_BINDING_H
enum {PioneerQ38TokenBatchedPrefetchPhysicalSelection=1};
#endif
static_assert(PioneerQ38TokenBatchedPrefetchPhysicalSelection==1,"late batched diagnostic binding");
