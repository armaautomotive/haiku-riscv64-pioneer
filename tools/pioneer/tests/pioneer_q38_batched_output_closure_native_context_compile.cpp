/* SPDX-License-Identifier: MIT */
// Separate PRIVATE renamed ABI; no installable module or Work route.
#define PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_PROGRAMS 1
#define PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_NATIVE_CONTEXT 1
#include "pioneer_q38_frontier_failure_link_native_context_compile.cpp"
static_assert(PioneerQ38TokenBatchedOutputClosureDataSelection==2
 && PioneerQ38TokenPrograms::BatchedOutputClosure==1 && PioneerQ38TokenPrograms::GlobalOutputReadsPerGroup==10,
 "actual distinct corrected ROOT package in private native subject");
