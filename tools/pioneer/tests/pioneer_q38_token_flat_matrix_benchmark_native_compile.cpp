/* SPDX-License-Identifier: MIT */
// Bounded diagnostic only,NO runtime CPU oracle or Assistant Work.
#define PIONEER_Q38_TOKEN_FLAT_MATRIX_PROGRAMS 1
#define PIONEER_Q38_TOKEN_FLAT_MATRIX_BENCHMARK_NATIVE 1
#include "pioneer_q38_token_empty_receipt_poll_benchmark_native_compile.cpp"
static_assert(PioneerQ38TokenFlatMatrixSelection==3
 && PioneerQ38TokenFlatMatrixPhysicalSelection==1 && PioneerQ38TokenPrograms::FlatMatrixGrammar==1,
 "actual distinct bounded flat QGW3 code binding");
