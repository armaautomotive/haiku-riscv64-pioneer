/* SPDX-License-Identifier: MIT */
// Private renamed-ABI BUILD subject only;NO install,export or Work grant.
#if !defined(PIONEER_Q38_TOKEN_PREFETCH_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_PREFETCH_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_PREFETCH_PROGRAMS) || PIONEER_Q38_TOKEN_PREFETCH_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_FLAT_MATRIX_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_FLAT_MATRIX_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_FLAT_MATRIX_PROGRAMS) || PIONEER_Q38_TOKEN_FLAT_MATRIX_PROGRAMS != 1
#error Exact separate prefetch private context and flat grammar parents required
#endif
#if defined(PIONEER_Q38_TOKEN_PREFETCH_BENCHMARK_NATIVE) || defined(PIONEER_Q38_TOKEN_PREFETCH_WORKER_IMAGE) \
 || defined(PIONEER_Q38_WORK_TILE_PREFETCH) || defined(PIONEER_Q38_FLAT_MATRIX_GRAMMAR_IMAGE)
#error Prefetch private context cannot alias diagnostic or shader BUILD
#endif
#include "PioneerQ38TokenFlatMatrixNativeRole.h"
