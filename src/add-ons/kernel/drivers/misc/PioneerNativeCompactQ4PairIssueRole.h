/* SPDX-License-Identifier: MIT */
// Rechecked on every include. Native108 is NOT the paired GPU BUILD role.
#ifdef PIONEER_NATIVE_COMPACT_Q4_PAIR_ISSUE_TIMING
#if PIONEER_NATIVE_COMPACT_Q4_PAIR_ISSUE_TIMING != 1 \
 || !defined(PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING) || PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING != 1 \
 || defined(PIONEER_Q38_Q4_PAIR_ISSUE_IMAGE)
#error Exact separate paired compact Q4 diagnostic108 role required
#endif
#ifdef _KERNEL_MODE
#if !defined(PIONEER_NATIVE_COMPACT_Q4_STREAM_DIAGNOSTIC) || PIONEER_NATIVE_COMPACT_Q4_STREAM_DIAGNOSTIC != 1 \
 || !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026110600010108ULL
#error Exact diagnostic108 kernel identity required
#endif
#endif
#endif
#ifndef PIONEER_NATIVE_COMPACT_Q4_PAIR_ISSUE_BINDING
#define PIONEER_NATIVE_COMPACT_Q4_PAIR_ISSUE_BINDING
#ifdef PIONEER_NATIVE_COMPACT_Q4_PAIR_ISSUE_TIMING
enum {PioneerNativeCompactQ4PairIssueSelection=1};
#else
enum {PioneerNativeCompactQ4PairIssueSelection=0};
#endif
#endif
#ifdef PIONEER_NATIVE_COMPACT_Q4_PAIR_ISSUE_TIMING
static_assert(PioneerNativeCompactQ4PairIssueSelection==1,"late compact Q4 pair issue selection");
#else
static_assert(PioneerNativeCompactQ4PairIssueSelection==0,"late compact Q4 pair issue removal");
#endif
