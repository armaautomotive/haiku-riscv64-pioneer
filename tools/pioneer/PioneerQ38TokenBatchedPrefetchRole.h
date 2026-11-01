/* SPDX-License-Identifier: MIT */
// Distinct shader BUILD only. Changed read-count grammar is NOT compatible
// with the existing ROOT/native packages. No owner,launch or Work grant.
#include "PioneerQ38TokenTwinBatchPrefetchRole.h"
#ifndef PIONEER_Q38_TOKEN_BATCHED_PREFETCH_BINDING_H
#define PIONEER_Q38_TOKEN_BATCHED_PREFETCH_BINDING_H
#ifdef PIONEER_Q38_TOKEN_BATCHED_PREFETCH_WORKER_IMAGE
#ifdef PIONEER_Q38_TOKEN_PREFETCH_WORKER_IMAGE
enum {PioneerQ38TokenBatchedPrefetchSelection=1};
#else
enum {PioneerQ38TokenBatchedPrefetchSelection=2};
#endif
#else
enum {PioneerQ38TokenBatchedPrefetchSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_BATCHED_PREFETCH_WORKER_IMAGE
#if PIONEER_Q38_TOKEN_BATCHED_PREFETCH_WORKER_IMAGE != 1
#error Exact separate batched-prefetch shader BUILD required
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(__FAST_MATH__) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_PREFETCH_PROGRAMS) \
 || defined(PIONEER_Q38_TOKEN_PREFETCH_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_PREFETCH_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_FLAT_MATRIX_PROGRAMS) || defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS) \
 || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) \
 || defined(PIONEER_Q38_BATCHED_PREFETCH_ROOT_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION) || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) \
 || defined(PIONEER_Q38_MIXED_WORKER_IMAGE) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_WORK_BATCH4_FETCH) \
 || defined(PIONEER_Q38_WORK_BATCH_RAW_INPUT) || defined(PIONEER_Q38_WORK_RAW_UNPACK_CACHE)
#error Batched-prefetch BUILD cannot alias old/native/ROOT/CPU oracle/Work ownership
#endif
#ifdef PIONEER_Q38_TOKEN_PREFETCH_WORKER_IMAGE
#if PIONEER_Q38_TOKEN_PREFETCH_WORKER_IMAGE != 1 \
 || !defined(PIONEER_Q38_WORK_BATCHED_PREFETCH) || PIONEER_Q38_WORK_BATCHED_PREFETCH != 1 \
 || !defined(PIONEER_Q38_WORK_TILE_PREFETCH) || PIONEER_Q38_WORK_TILE_PREFETCH != 1 \
 || !defined(PIONEER_Q38_MIXED_CAPSULE_IMAGE) || PIONEER_Q38_MIXED_CAPSULE_IMAGE != 1 \
 || defined(PIONEER_Q38_PRECISE_WORKER_IMAGE) || defined(PIONEER_Q38_PUSH_COMPLETION_WORKER_IMAGE)
#error Batched math requires its distinct returning prefetch math parents only
#endif
static_assert(PioneerQ38TokenBatchedPrefetchSelection==1,"late batched math selection");
#else
#if !defined(PIONEER_Q38_PRECISE_WORKER_IMAGE) || PIONEER_Q38_PRECISE_WORKER_IMAGE != 1 \
 || !defined(PIONEER_Q38_PUSH_COMPLETION_WORKER_IMAGE) || PIONEER_Q38_PUSH_COMPLETION_WORKER_IMAGE != 1 \
 || defined(PIONEER_Q38_WORK_BATCHED_PREFETCH) || defined(PIONEER_Q38_WORK_TILE_PREFETCH) \
 || defined(PIONEER_Q38_MIXED_CAPSULE_IMAGE)
#error Batched counter grammar requires its distinct QGW3 sender parents only
#endif
static_assert(PioneerQ38TokenBatchedPrefetchSelection==2,"late batched sender selection");
#endif
#else
#ifdef PIONEER_Q38_WORK_BATCHED_PREFETCH
#error Batched math requires its separately selected full worker BUILD
#endif
static_assert(PioneerQ38TokenBatchedPrefetchSelection==0,"late batched BUILD removal");
#endif
