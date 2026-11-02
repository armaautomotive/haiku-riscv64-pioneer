/* SPDX-License-Identifier: MIT */
// Separate DATA/private/diagnostic boundary,rechecked through direct old
// header/geometry includes. Package identity alone grants no lifecycle/Work.
#ifndef PIONEER_Q38_TOKEN_BATCHED_PREFETCH_DATA_SELECTION_H
#define PIONEER_Q38_TOKEN_BATCHED_PREFETCH_DATA_SELECTION_H
#ifdef PIONEER_Q38_TOKEN_BATCHED_PREFETCH_BENCHMARK_NATIVE
enum {PioneerQ38TokenBatchedPrefetchDataSelection=3};
#elif defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_NATIVE_CONTEXT)
enum {PioneerQ38TokenBatchedPrefetchDataSelection=2};
#elif defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PROGRAMS)
enum {PioneerQ38TokenBatchedPrefetchDataSelection=1};
#else
enum {PioneerQ38TokenBatchedPrefetchDataSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PROGRAMS
#if PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_PREFETCH_PROGRAMS) || PIONEER_Q38_TOKEN_PREFETCH_PROGRAMS != 1
#error Exact distinct batched-prefetch DATA and prefetch parent revision required
#endif
#if defined(PIONEER_Q38_TOKEN_PROGRAMS_H) || defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS_H) \
 || defined(PIONEER_Q38_TOKEN_PREFETCH_PROGRAMS_H) || defined(PIONEER_Q38_TOKEN_FLAT_MATRIX_PROGRAMS_H) \
 || defined(PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_PROGRAMS_H) || defined(PIONEER_Q38_TOKEN_COUNTER_GATHER_PROGRAMS_H) \
 || defined(PIONEER_Q38_TOKEN_TID_BOUNDARY_PROGRAMS_H) || defined(PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_PROGRAMS_H) \
 || defined(PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_PROGRAMS_H) || defined(PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_PROGRAMS_H) \
 || defined(PIONEER_Q38_TOKEN_RETAINED_COHORT_PROGRAMS_H) || defined(PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS_H) \
 || defined(PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS_H) || defined(PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS_H) \
 || defined(PIONEER_Q38_TOKEN_FAULT_PROGRAMS_H) || defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS_H) \
 || (defined(PIONEER_Q38_TOKEN_SELECTED_PROGRAMS_H) && !defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PROGRAMS_H) \
  && !((defined(PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_NATIVE_CONTEXT) && PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_NATIVE_CONTEXT == 1 \
     || defined(PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_BENCHMARK_NATIVE) && PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_BENCHMARK_NATIVE == 1) \
   && defined(PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_PROGRAMS_H)) \
  && !(defined(PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_PROGRAMS) && PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_PROGRAMS == 1 \
   && defined(PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_PROGRAMS_H)) \
  && !(defined(PIONEER_Q38_TOKEN_WIDE_GDN_PROGRAMS) && PIONEER_Q38_TOKEN_WIDE_GDN_PROGRAMS == 1 \
   && defined(PIONEER_Q38_TOKEN_WIDE_GDN_PROGRAMS_H)))
#error Batched-prefetch DATA cannot coexist with old arrays or selected native namespace
#endif
#if defined(PIONEER_Q38_TOKEN_NATIVE_TEST_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_WORKER_IMAGE) || defined(PIONEER_Q38_WORK_BATCHED_PREFETCH) \
 || defined(PIONEER_Q38_TOKEN_PREFETCH_WORKER_IMAGE) || defined(PIONEER_Q38_WORK_TILE_PREFETCH) \
 || defined(PIONEER_Q38_BATCHED_PREFETCH_ROOT_IMAGE) || defined(PIONEER_Q38_FLAT_MATRIX_GRAMMAR_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) || defined(PIONEER_Q38_PRECISE_WORKER_IMAGE) \
 || defined(PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT) || defined(PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(__FAST_MATH__)
#error Batched binding cannot alias a USER,shader,CPU oracle or Work role
#endif
#ifdef PIONEER_Q38_TOKEN_BATCHED_PREFETCH_BENCHMARK_NATIVE
#if PIONEER_Q38_TOKEN_BATCHED_PREFETCH_BENCHMARK_NATIVE != 1 \
 || !defined(PIONEER_Q38_TOKEN_PREFETCH_BENCHMARK_NATIVE) || PIONEER_Q38_TOKEN_PREFETCH_BENCHMARK_NATIVE != 1 \
 || defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_PREFETCH_NATIVE_CONTEXT)
#error Exact distinct batched diagnostic DATA binding required,never private NODRV
#endif
static_assert(PioneerQ38TokenBatchedPrefetchDataSelection==3,"late batched diagnostic DATA boundary");
#elif defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_NATIVE_CONTEXT)
#if PIONEER_Q38_TOKEN_BATCHED_PREFETCH_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_PREFETCH_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_PREFETCH_NATIVE_CONTEXT != 1
#error Exact distinct private batched-prefetch context and prefetch parent required
#endif
// Native role checks must wait until ProgramsRole AFTER geometry exists.
static_assert(PioneerQ38TokenBatchedPrefetchDataSelection==2,"late batched private DATA boundary");
#else
#if defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_PREFETCH_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY)
#error Batched DATA alone has no kernel,private native,driver tag or ABI binding
#endif
static_assert(PioneerQ38TokenBatchedPrefetchDataSelection==1,"late batched DATA selection");
#endif
#if defined(PIONEER_Q38_TOKEN_PREFETCH_BENCHMARK_NATIVE) && !defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_BENCHMARK_NATIVE)
#error Old prefetch diagnostic cannot acquire batched DATA without its distinct outer binding
#endif
#else
#ifdef PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PROGRAMS_H
#error Batched code guard cannot forge an unselected old package binding
#endif
#if defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_BENCHMARK_NATIVE)
#error Batched-prefetch native binding requires its own separately qualified DATA
#endif
static_assert(PioneerQ38TokenBatchedPrefetchDataSelection==0,"late batched DATA removal");
#endif
