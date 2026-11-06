/* SPDX-License-Identifier: MIT */
// Rechecked on every include,including late selector changes.
#include "PioneerNativeCompactQ4PairIssueRole.h"
#include "PioneerNativeFactoredQ4CompareRole.h"
#include "PioneerNativeFactoredQ4TraceRole.h"
#ifdef PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING
#if PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING != 1 \
 || defined(PIONEER_Q38_COMPACT_Q4_TIMING_IMAGE) || defined(PIONEER_Q38_COMPACT_Q4_FPU_STREAM_IMAGE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_Q4_PAIR_ISSUE_IMAGE) || defined(__FAST_MATH__)
#error Exact separate timed compact Q4 diagnostic107 role required
#endif
#ifdef _KERNEL_MODE
#if !defined(PIONEER_NATIVE_COMPACT_Q4_STREAM_DIAGNOSTIC) || PIONEER_NATIVE_COMPACT_Q4_STREAM_DIAGNOSTIC != 1 \
 || !defined(TT_PIONEER_DRIVER_RELOAD) \
 || (!defined(PIONEER_NATIVE_COMPACT_Q4_PAIR_ISSUE_TIMING) && PIONEER_DRIVER_BUILD_TAG != (defined(PIONEER_NATIVE_FACTORED_Q4_TRACE)?0x2026110600010112ULL:defined(PIONEER_NATIVE_FACTORED_Q4_COMPARE)?0x2026110600010111ULL:0x2026110600010107ULL))
#error Exact diagnostic107 kernel identity required
#endif
#endif
#endif
#ifndef PIONEER_NATIVE_COMPACT_Q4_TIMING_BINDING
#define PIONEER_NATIVE_COMPACT_Q4_TIMING_BINDING
#ifdef PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING
enum {PioneerNativeCompactQ4TimingSelection=1};
#else
enum {PioneerNativeCompactQ4TimingSelection=0};
#endif
#endif
#ifdef PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING
static_assert(PioneerNativeCompactQ4TimingSelection==1,"late compact Q4 timing selection");
#else
static_assert(PioneerNativeCompactQ4TimingSelection==0,"late compact Q4 timing removal");
#endif
