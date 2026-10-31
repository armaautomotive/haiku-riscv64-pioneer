/* SPDX-License-Identifier: MIT */
// Explicit new conventional diagnostic;NOT NODRV,CPU-reference,Work or TEST.
#define PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS 1
#define PIONEER_Q38_TOKEN_FRONTIER_NATIVE_CONTEXT 1
#define PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_NATIVE 1
#include "pioneer_q38_token_decode_benchmark_native_compile.cpp"
static_assert(PioneerQ38TokenFrontierProgramSelection==3 && PioneerQ38TokenFrontierPhysicalSelection==1
 && PioneerQ38TokenPrograms::TokenFrontier==1,"separate physical publisher/reader revision");
