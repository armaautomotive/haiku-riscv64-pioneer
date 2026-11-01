/* SPDX-License-Identifier: MIT */
// Private renamed-ABI subject only. No export,install or Work authority.
#if !defined(PIONEER_Q38_TOKEN_FLAT_MATRIX_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_FLAT_MATRIX_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_FLAT_MATRIX_PROGRAMS) || PIONEER_Q38_TOKEN_FLAT_MATRIX_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_PROGRAMS) || PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_PROGRAMS != 1
#error Exact flat grammar private context and empty-receipt parents required
#endif
#if defined(PIONEER_Q38_TOKEN_FLAT_MATRIX_BENCHMARK_NATIVE) || defined(PIONEER_Q38_FLAT_MATRIX_GRAMMAR_IMAGE)
#error Flat grammar private context cannot alias diagnostic or ROOT BUILD
#endif
#include "PioneerQ38TokenEmptyReceiptPollNativeRole.h"
