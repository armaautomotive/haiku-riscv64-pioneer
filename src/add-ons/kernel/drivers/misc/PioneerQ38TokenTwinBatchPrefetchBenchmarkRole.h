/* SPDX-License-Identifier: MIT */
// Explicit fixed6607+6608 diagnostic BUILD only. Whole module identity,
// not the shared tag75,is binding. No automatic staging or Work grant.
#if !defined(PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_BENCHMARK_NATIVE) || PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_BENCHMARK_NATIVE != 1 \
 || !defined(PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_PROGRAMS) || PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_BENCHMARK_NATIVE) || PIONEER_Q38_TOKEN_BATCHED_PREFETCH_BENCHMARK_NATIVE != 1 \
 || !defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PROGRAMS) || PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PROGRAMS != 1 \
 || defined(PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_WORKER_IMAGE) || defined(PIONEER_Q38_WORK_TWIN_BATCH_PREFETCH) \
 || defined(PIONEER_Q38_TOKEN_FRONTIER_FAILURE_REPORT_CLIENT) || defined(PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT)
#error Exact separate bounded twin-batch diagnostic required,never private NODRV,USER,shader or Work
#endif
static_assert(PioneerQ38TokenTwinBatchPrefetchParentBinding==1,"distinct twin diagnostic requires its own outer binding");
#include "PioneerQ38TokenBatchedPrefetchBenchmarkRole.h"
#ifndef PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_PHYSICAL_BINDING_H
#define PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_PHYSICAL_BINDING_H
enum {PioneerQ38TokenTwinBatchPrefetchPhysicalSelection=1};
#endif
static_assert(PioneerQ38TokenTwinBatchPrefetchPhysicalSelection==1,"late twin-batch diagnostic binding");
