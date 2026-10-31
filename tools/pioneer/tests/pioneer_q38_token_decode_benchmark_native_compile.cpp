/* SPDX-License-Identifier: MIT */
// Compile/inspect only. Same qualified firmware; separate bounded endpoint.
#define PIONEER_Q38_TOKEN_DECODE_BENCHMARK_NATIVE 1
#include "pioneer_q38_token_job_boot_fold_validation_native_compile.cpp"
static_assert(PioneerQ38TokenDecodeBenchmark::Positions==34
 && PioneerQ38TokenDecodeBenchmark::FeedbackPositions==32,"distinct longer diagnostic");
