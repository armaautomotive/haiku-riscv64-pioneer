/* SPDX-License-Identifier: MIT */
// Separate ROOT BUILD only;old worker/native/DATA counter grammar differs.
#include "PioneerQ38FlatMatrixGrammarRole.h"
#ifndef PIONEER_Q38_BATCHED_PREFETCH_ROOT_BINDING_H
#define PIONEER_Q38_BATCHED_PREFETCH_ROOT_BINDING_H
#ifdef PIONEER_Q38_BATCHED_PREFETCH_ROOT_IMAGE
enum {PioneerQ38BatchedPrefetchRootSelection=1};
#else
enum {PioneerQ38BatchedPrefetchRootSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_BATCHED_PREFETCH_ROOT_IMAGE
#if PIONEER_Q38_BATCHED_PREFETCH_ROOT_IMAGE != 1 \
 || !defined(PIONEER_Q38_FLAT_MATRIX_GRAMMAR_IMAGE) || PIONEER_Q38_FLAT_MATRIX_GRAMMAR_IMAGE != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION) || PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION) || PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION != 1 \
 || !defined(__riscv) || __riscv != 1 || !defined(__riscv_xlen) || __riscv_xlen != 32 \
 || (defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) && PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE != 1) \
 || (defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) && PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE != 1) \
 || (defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) == defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE))
#error Batched counters require the exact distinct flat-grammar returning or token ROOT BUILD
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(__FAST_MATH__) \
 || defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_WORKER_IMAGE) || defined(PIONEER_Q38_WORK_BATCHED_PREFETCH) \
 || defined(PIONEER_Q38_WORK_TILE_PREFETCH) || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) \
 || defined(PIONEER_Q38_PRECISE_WORKER_IMAGE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_PREFETCH_PROGRAMS) || defined(PIONEER_Q38_TOKEN_PREFETCH_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_PREFETCH_BENCHMARK_NATIVE) || defined(PIONEER_Q38_TOKEN_FLAT_MATRIX_PROGRAMS) \
 || defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE)
#error Batched ROOT BUILD cannot alias shader,old DATA,native,CPU oracle or Work ownership
#endif
static_assert(PioneerQ38BatchedPrefetchRootSelection==1,"late batched ROOT selection");
#else
static_assert(PioneerQ38BatchedPrefetchRootSelection==0,"late batched ROOT removal");
#endif
