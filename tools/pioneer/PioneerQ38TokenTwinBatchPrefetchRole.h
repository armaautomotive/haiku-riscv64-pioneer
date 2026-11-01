/* SPDX-License-Identifier: MIT */
// Separate returning math BUILD only. Same batched sender packet counts,
// but a DIFFERENT raw-buffer lifetime:old DATA/native packages cannot use it.
#ifndef PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_BINDING_H
#define PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_BINDING_H
#ifdef PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_WORKER_IMAGE
enum {PioneerQ38TokenTwinBatchPrefetchSelection=1};
#else
enum {PioneerQ38TokenTwinBatchPrefetchSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_WORKER_IMAGE
#if PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_WORKER_IMAGE != 1 \
 || !defined(PIONEER_Q38_WORK_TWIN_BATCH_PREFETCH) || PIONEER_Q38_WORK_TWIN_BATCH_PREFETCH != 1 \
 || !defined(PIONEER_Q38_TWIN_BATCH_PARENT_BINDING) || PIONEER_Q38_TWIN_BATCH_PARENT_BINDING != 1 \
 || !defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_WORKER_IMAGE) || PIONEER_Q38_TOKEN_BATCHED_PREFETCH_WORKER_IMAGE != 1 \
 || !defined(PIONEER_Q38_WORK_BATCHED_PREFETCH) || PIONEER_Q38_WORK_BATCHED_PREFETCH != 1 \
 || !defined(PIONEER_Q38_TOKEN_PREFETCH_WORKER_IMAGE) || PIONEER_Q38_TOKEN_PREFETCH_WORKER_IMAGE != 1 \
 || !defined(PIONEER_Q38_WORK_TILE_PREFETCH) || PIONEER_Q38_WORK_TILE_PREFETCH != 1
#error Exact separate twin-batch returning math BUILD and parent binding required
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(__FAST_MATH__) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PROGRAMS) || defined(PIONEER_Q38_TOKEN_PREFETCH_PROGRAMS) \
 || defined(PIONEER_Q38_PRECISE_WORKER_IMAGE) || defined(PIONEER_Q38_PUSH_COMPLETION_WORKER_IMAGE) \
 || defined(PIONEER_Q38_BATCHED_PREFETCH_ROOT_IMAGE) || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE)
#error Twin-batch BUILD cannot alias old DATA/native/ROOT/sender/CPU/Work ownership
#endif
static_assert(PioneerQ38TokenTwinBatchPrefetchSelection==1,"late twin-batch shader selection");
#else
#if defined(PIONEER_Q38_WORK_TWIN_BATCH_PREFETCH) || defined(PIONEER_Q38_TWIN_BATCH_PARENT_BINDING)
#error Twin-batch math requires its separate full-worker BUILD selection
#endif
static_assert(PioneerQ38TokenTwinBatchPrefetchSelection==0,"late twin-batch shader removal");
#endif
