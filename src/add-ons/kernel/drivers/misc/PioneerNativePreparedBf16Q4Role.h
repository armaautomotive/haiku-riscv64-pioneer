/* SPDX-License-Identifier: MIT */
// Deliberately rechecked outside include guards, including late removal.
#ifdef PIONEER_NATIVE_PREPARED_BF16_Q4_COMPARE
#if PIONEER_NATIVE_PREPARED_BF16_Q4_COMPARE!=1 || !defined(PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING) || PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING!=1 || defined(PIONEER_NATIVE_FACTORED_Q4_COMPARE) || defined(PIONEER_NATIVE_FACTORED_Q4_TRACE) || defined(PIONEER_NATIVE_FACTORED_Q4_MANUAL_LAST) || defined(PIONEER_NATIVE_FACTORED_Q4_CONFIGURED) || defined(PIONEER_NATIVE_COMPACT_Q4_PAIR_ISSUE_TIMING) || defined(PIONEER_Q38_PREPARED_BF16_COMPARE_BUILD) || defined(PIONEER_Q38_PREPARED_BF16_Q4_BUILD) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || defined(__FAST_MATH__)
#error Exact separate prepared BF16 Q4 comparison115 role required
#endif
#ifdef _KERNEL_MODE
#if !defined(PIONEER_NATIVE_COMPACT_Q4_STREAM_DIAGNOSTIC) || PIONEER_NATIVE_COMPACT_Q4_STREAM_DIAGNOSTIC!=1 || !defined(TT_PIONEER_DRIVER_RELOAD) || !defined(PIONEER_DRIVER_BUILD_TAG) || PIONEER_DRIVER_BUILD_TAG!=0x2026110600010115ULL || defined(PIONEER_Q38_PREPARED_BF16_Q4_OFFBOARD)
#error Exact prepared BF16 Q4 comparison115 kernel identity required
#endif
#endif
#endif
#ifndef PIONEER_NATIVE_PREPARED_BF16_Q4_BINDING
#define PIONEER_NATIVE_PREPARED_BF16_Q4_BINDING
#ifdef PIONEER_NATIVE_PREPARED_BF16_Q4_COMPARE
enum {PioneerNativePreparedBf16Q4Selection=1};
#else
enum {PioneerNativePreparedBf16Q4Selection=0};
#endif
#endif
#ifdef PIONEER_NATIVE_PREPARED_BF16_Q4_COMPARE
static_assert(PioneerNativePreparedBf16Q4Selection==1,"late prepared BF16 Q4 selection");
#else
static_assert(PioneerNativePreparedBf16Q4Selection==0,"late prepared BF16 Q4 removal");
#endif
