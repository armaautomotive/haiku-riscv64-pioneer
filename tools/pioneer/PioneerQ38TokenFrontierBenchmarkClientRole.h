/* SPDX-License-Identifier: MIT */
// Rechecked USER discriminator;not the old6606 diagnostic or Work.
#if !defined(PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT) || PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT != 1 \
 || !defined(PIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT) || PIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT != 1 \
 || defined(PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_FRONTIER_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS)
#error Exact separate frontier benchmark USER role required
#endif
#include "PioneerQ38TokenDecodeBenchmarkClientRole.h"
