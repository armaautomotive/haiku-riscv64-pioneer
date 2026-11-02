/* SPDX-License-Identifier: MIT */
// Pure DATA only. Do not reuse an old native owner just because packet
// counts match. Exact ROM,buffer lifetime and full END need qualification.
#include "PioneerQ38TokenTwinBatchPrefetchDataBoundary.h"
#ifdef PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_PROGRAMS
static_assert(PioneerQ38TokenBatchedOutputClosureParentBinding==1,"output closure DATA requires its distinct outer role");
#endif
#ifndef PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_PARENT_BINDING_H
#define PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_PARENT_BINDING_H
#ifdef PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_PROGRAMS
enum {PioneerQ38TokenTwinBatchPrefetchParentBinding=1};
#else
enum {PioneerQ38TokenTwinBatchPrefetchParentBinding=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_BENCHMARK_NATIVE
#include "PioneerQ38TokenBatchedPrefetchProgramsRole.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenTwinBatchPrefetchBenchmarkRole.h"
static_assert(PioneerQ38TokenTwinBatchPrefetchDataSelection==3,"exact bounded twin-batch diagnostic binding");
#elif defined(PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_NATIVE_CONTEXT)
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenTwinBatchPrefetchNativeRole.h"
#include "PioneerQ38TokenBatchedPrefetchProgramsRole.h"
static_assert(PioneerQ38TokenTwinBatchPrefetchDataSelection==2,"exact private twin-batch binding");
#endif
