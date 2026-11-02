/* SPDX-License-Identifier: MIT */
// Recheck on EVERY include. This is a distinct ROOT build, not permission
// to use its changed receipt grammar in old programs/native/Work subjects.
#include "PioneerQ38BatchedPrefetchRootRole.h"
#ifndef PIONEER_Q38_BATCHED_OUTPUT_CLOSURE_ROOT_BINDING_H
#define PIONEER_Q38_BATCHED_OUTPUT_CLOSURE_ROOT_BINDING_H
#ifdef PIONEER_Q38_BATCHED_OUTPUT_CLOSURE_ROOT_IMAGE
enum {PioneerQ38BatchedOutputClosureRootSelection=1};
#else
enum {PioneerQ38BatchedOutputClosureRootSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_BATCHED_OUTPUT_CLOSURE_ROOT_IMAGE
#if PIONEER_Q38_BATCHED_OUTPUT_CLOSURE_ROOT_IMAGE != 1 \
 || !defined(PIONEER_Q38_BATCHED_PREFETCH_ROOT_IMAGE) || PIONEER_Q38_BATCHED_PREFETCH_ROOT_IMAGE != 1
#error Batched output closure requires the exact distinct batched-prefetch ROOT BUILD
#endif
#if defined(PIONEER_Q38_JOB_BOOT_FOLD_HOST_TEST) \
 || defined(PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_WORKER_IMAGE) \
 || defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PROGRAMS) \
 || defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_PROGRAMS) \
 || defined(PIONEER_Q38_LAYER29_EMITTED_DELTA_DIAGNOSTIC) \
 || defined(PIONEER_Q38_LAYER29_WIDE_REDUCTION_DIAGNOSTIC)
#error Batched output closure ROOT BUILD cannot alias HOST tests,worker,old programs or native authority
#endif
static_assert(PioneerQ38BatchedOutputClosureRootSelection==1,"late batched output closure selection");
#else
static_assert(PioneerQ38BatchedOutputClosureRootSelection==0,"late batched output closure removal");
#endif
