/* SPDX-License-Identifier: MIT */
// Recheck outside include guards;late selection/removal is forbidden.
#ifdef PIONEER_NATIVE_FACTORED_Q4_MANUAL_LAST
#if PIONEER_NATIVE_FACTORED_Q4_MANUAL_LAST!=1 || !defined(PIONEER_NATIVE_FACTORED_Q4_COMPARE) || PIONEER_NATIVE_FACTORED_Q4_COMPARE!=1 || defined(PIONEER_NATIVE_FACTORED_Q4_TRACE)
#error Exact separate manual-Last comparison113 role required
#endif
#endif
#ifdef PIONEER_NATIVE_FACTORED_Q4_COMPARE
#if PIONEER_NATIVE_FACTORED_Q4_COMPARE!=1 || !defined(PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING) || PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING!=1 || defined(PIONEER_NATIVE_COMPACT_Q4_PAIR_ISSUE_TIMING) || defined(PIONEER_Q38_FACTORED_Q4_COMPARE_BUILD) || defined(PIONEER_Q38_FACTORED_Q4_MATRIX_BUILD) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error Exact separate factored-Q4 comparison111 role required
#endif
#ifdef _KERNEL_MODE
#if !defined(PIONEER_NATIVE_COMPACT_Q4_STREAM_DIAGNOSTIC) || PIONEER_NATIVE_COMPACT_Q4_STREAM_DIAGNOSTIC!=1 || !defined(TT_PIONEER_DRIVER_RELOAD) || !defined(PIONEER_DRIVER_BUILD_TAG) || PIONEER_DRIVER_BUILD_TAG!=(defined(PIONEER_NATIVE_FACTORED_Q4_MANUAL_LAST)?0x2026110600010113ULL:0x2026110600010111ULL)
#error Exact comparison111 kernel identity required
#endif
#endif
#endif
#ifndef PIONEER_NATIVE_FACTORED_Q4_COMPARE_BINDING
#define PIONEER_NATIVE_FACTORED_Q4_COMPARE_BINDING
#ifdef PIONEER_NATIVE_FACTORED_Q4_MANUAL_LAST
enum {PioneerNativeFactoredQ4ManualLastSelection=1};
#else
enum {PioneerNativeFactoredQ4ManualLastSelection=0};
#endif
#ifdef PIONEER_NATIVE_FACTORED_Q4_COMPARE
enum {PioneerNativeFactoredQ4CompareSelection=1};
#else
enum {PioneerNativeFactoredQ4CompareSelection=0};
#endif
#endif
#ifdef PIONEER_NATIVE_FACTORED_Q4_MANUAL_LAST
static_assert(PioneerNativeFactoredQ4ManualLastSelection==1,"late manual-Last113 selection");
#else
static_assert(PioneerNativeFactoredQ4ManualLastSelection==0,"late manual-Last113 removal");
#endif
#ifdef PIONEER_NATIVE_FACTORED_Q4_COMPARE
static_assert(PioneerNativeFactoredQ4CompareSelection==1,"late factored comparison selection");
#else
static_assert(PioneerNativeFactoredQ4CompareSelection==0,"late factored comparison removal");
#endif
