/* SPDX-License-Identifier: MIT */
// Private renamed-ABI NODRV only;not installable or Assistant Work.
#if !defined(PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_PROGRAMS) || PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_COUNTER_GATHER_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_COUNTER_GATHER_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_COUNTER_GATHER_PROGRAMS) || PIONEER_Q38_TOKEN_COUNTER_GATHER_PROGRAMS != 1
#error Exact empty-receipt-poll private context and counter-gather parents required
#endif
#if defined(PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_BENCHMARK_NATIVE) || defined(PIONEER_Q38_EMPTY_RECEIPT_POLL_IMAGE) \
 || defined(PIONEER_Q38_READ_SOURCE_BOUNDARY_IMAGE)
#error Empty-receipt-poll private context cannot alias diagnostic or ROOT BUILD
#endif
#include "PioneerQ38TokenCounterGatherNativeRole.h"
