/* SPDX-License-Identifier: MIT */
// USER role only. Host label/CLI do NOT attest the loaded driver image.
#ifndef PIONEER_Q38_TOKEN_CONSTANT_Q4_CLIENT_SELECTION_H
#define PIONEER_Q38_TOKEN_CONSTANT_Q4_CLIENT_SELECTION_H
#ifdef PIONEER_Q38_TOKEN_CONSTANT_Q4_CLIENT
enum {PioneerQ38TokenConstantQ4ClientSelection=1};
#else
enum {PioneerQ38TokenConstantQ4ClientSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_CONSTANT_Q4_CLIENT
#include "PioneerQ38FrontierFailureReportRole.h"
#if PIONEER_Q38_TOKEN_CONSTANT_Q4_CLIENT != 1 \
 || !defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_CLIENT) || PIONEER_Q38_TOKEN_PHASE_PROFILE_CLIENT != 1 \
 || !defined(PIONEER_Q38_TOKEN_FRONTIER_FAILURE_REPORT_CLIENT) || PIONEER_Q38_TOKEN_FRONTIER_FAILURE_REPORT_CLIENT != 1
#error Exact constant Q4 USER client and complete held-timing/failed-reply parents required
#endif
#if defined(PIONEER_Q38_TOKEN_CONSTANT_Q4_PROGRAMS) || defined(PIONEER_Q38_TOKEN_CONSTANT_Q4_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_CONSTANT_Q4_BENCHMARK_NATIVE) || defined(PIONEER_Q38_Q4_CONSTANT_ISSUE_IMAGE) \
 || defined(PIONEER_Q38_Q4_CONSTANT_ISSUE) || defined(PIONEER_Q38_Q4_CONSTANT_ISSUE_PARENT_BINDING) \
 || defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_PROGRAMS) || defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_BENCHMARK_NATIVE) || defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE) \
 || defined(PIONEER_Q38_TOKEN_WIDE_GDN_PROGRAMS) || defined(PIONEER_Q38_TOKEN_WIDE_GDN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_WIDE_GDN_BENCHMARK_NATIVE) \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(__FAST_MATH__) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE)
#error Constant Q4 USER client cannot alias DATA,shader,native,oracle or Work
#endif
static_assert(PioneerQ38TokenConstantQ4ClientSelection==1,"late constant Q4 USER selection");
#else
static_assert(PioneerQ38TokenConstantQ4ClientSelection==0,"late constant Q4 USER removal");
#endif
