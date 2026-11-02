/* SPDX-License-Identifier: MIT */
#include "PioneerQ38TokenBatchedOutputClosureDataBoundary.h"
#ifndef PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_PARENT_BINDING_H
#define PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_PARENT_BINDING_H
#ifdef PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_PROGRAMS
enum {PioneerQ38TokenBatchedOutputClosureParentBinding=1};
#else
enum {PioneerQ38TokenBatchedOutputClosureParentBinding=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_PROGRAMS
static_assert(PioneerQ38TokenBatchedOutputClosureParentBinding==1,"late output closure outer binding");
#if defined(PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_BENCHMARK_NATIVE)
#include "PioneerQ38TokenTwinBatchPrefetchProgramsRole.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenBatchedOutputClosureRole.h"
#endif
#else
static_assert(PioneerQ38TokenBatchedOutputClosureParentBinding==0,"late output closure outer removal");
#endif
