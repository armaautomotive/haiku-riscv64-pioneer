/* SPDX-License-Identifier: MIT */
// New112 observation endpoint,never an alias/replay of rejected111.
#ifdef PIONEER_NATIVE_FACTORED_Q4_TRACE
#if PIONEER_NATIVE_FACTORED_Q4_TRACE!=1 || !defined(PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING) || PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING!=1 || defined(PIONEER_NATIVE_FACTORED_Q4_COMPARE) || defined(PIONEER_NATIVE_COMPACT_Q4_PAIR_ISSUE_TIMING) || defined(PIONEER_Q38_FACTORED_Q4_TRACE_BUILD) || defined(PIONEER_Q38_FACTORED_Q4_COMPARE_BUILD) || defined(PIONEER_Q38_FACTORED_Q4_MATRIX_BUILD) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error Exact separate factored-Q4 trace112 role required
#endif
#ifdef _KERNEL_MODE
#if !defined(PIONEER_NATIVE_COMPACT_Q4_STREAM_DIAGNOSTIC) || PIONEER_NATIVE_COMPACT_Q4_STREAM_DIAGNOSTIC!=1 || !defined(TT_PIONEER_DRIVER_RELOAD) || !defined(PIONEER_DRIVER_BUILD_TAG) || PIONEER_DRIVER_BUILD_TAG!=0x2026110600010112ULL
#error Exact trace112 kernel identity required
#endif
#endif
#endif
#ifndef PIONEER_NATIVE_FACTORED_Q4_TRACE_BINDING
#define PIONEER_NATIVE_FACTORED_Q4_TRACE_BINDING
#ifdef PIONEER_NATIVE_FACTORED_Q4_TRACE
enum {PioneerNativeFactoredQ4TraceSelection=1};
#else
enum {PioneerNativeFactoredQ4TraceSelection=0};
#endif
#endif
#ifdef PIONEER_NATIVE_FACTORED_Q4_TRACE
static_assert(PioneerNativeFactoredQ4TraceSelection==1,"late factored trace selection");
#else
static_assert(PioneerNativeFactoredQ4TraceSelection==0,"late factored trace removal");
#endif
