/* SPDX-License-Identifier: MIT */
// Separate USER diagnostic only. Rechecked on every include. Returned
// packet consistency is NOT driver provenance,root cause or cleanup proof.
#ifndef PIONEER_Q38_FRONTIER_FAILURE_REPORT_SELECTION_H
#define PIONEER_Q38_FRONTIER_FAILURE_REPORT_SELECTION_H
#ifdef PIONEER_Q38_TOKEN_FRONTIER_FAILURE_REPORT_CLIENT
enum {PioneerQ38FrontierFailureReportSelection=1};
#else
enum {PioneerQ38FrontierFailureReportSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_FRONTIER_FAILURE_REPORT_CLIENT
#if PIONEER_Q38_TOKEN_FRONTIER_FAILURE_REPORT_CLIENT != 1 \
 || !defined(PIONEER_Q38_FRONTIER_FAILURE_REPORT_PARENT_BINDING) || PIONEER_Q38_FRONTIER_FAILURE_REPORT_PARENT_BINDING != 1 \
 || !defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT) || PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT != 1 \
 || !defined(PIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT) || PIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT != 1 \
 || !defined(PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT) || PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT != 1
#error Exact separate failed-reply USER leaf and all diagnostic parents required
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(__FAST_MATH__) \
 || defined(PIONEER_ATTENTION_FREESTANDING) || defined(PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) \
 || defined(PIONEER_Q38_TOKEN_FRONTIER_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE) || defined(PIONEER_Q38_TOKEN_DECODE_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_VALIDATION_CLIENT) || defined(PIONEER_Q38_TOKEN_BOUNDARY_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) \
 || defined(PIONEER_Q38_TOKEN_PREFETCH_WORKER_IMAGE) || defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_WORKER_IMAGE) \
 || defined(PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_WORKER_IMAGE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE)
#error Failed-reply USER diagnostic cannot alias profile,native,shader,CPU oracle or Work ownership
#endif
static_assert(PioneerQ38FrontierFailureReportSelection==1,"late failed-reply USER selection");
#else
#ifdef PIONEER_Q38_FRONTIER_FAILURE_REPORT_PARENT_BINDING
#error Failed-reply leaf binding requires its own distinct USER selector
#endif
static_assert(PioneerQ38FrontierFailureReportSelection==0,"late failed-reply USER removal");
#endif
