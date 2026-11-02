/* SPDX-License-Identifier: MIT */
// Separate bounded diagnostic BUILD, not an installer or Assistant Work.
#define PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_PROGRAMS 1
#define PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_BENCHMARK_NATIVE 1
#include "pioneer_q38_frontier_failure_link_benchmark_native_compile.cpp"
static_assert(PioneerQ38TokenBatchedOutputClosureDataSelection==3
 && PioneerQ38TokenPrograms::BatchedOutputClosure==1 && PioneerQ38TokenPrograms::GlobalOutputReadsPerGroup==10,
 "actual distinct corrected ROOT package in bounded native subject");
