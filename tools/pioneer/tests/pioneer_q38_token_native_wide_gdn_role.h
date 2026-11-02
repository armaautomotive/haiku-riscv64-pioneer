/* SPDX-License-Identifier: MIT */
// Own HOST native TEST binding; never accept an old narrow peer as wide.
#if !defined(PIONEER_Q38_TOKEN_NATIVE_WIDE_GDN_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_WIDE_GDN_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_WIDE_GDN_PROGRAMS) || PIONEER_Q38_TOKEN_WIDE_GDN_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_WIDE_GDN_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_WIDE_GDN_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_BATCHED_OUTPUT_CLOSURE_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_BATCHED_OUTPUT_CLOSURE_FIXTURE != 1 \
 || defined(PIONEER_Q38_TOKEN_WIDE_GDN_BENCHMARK_NATIVE)
#error New returning GDN TEST requires own complete private binding and corrected-output lifecycle
#endif
#include "../PioneerQ38TokenWideGdnProgramsRole.h"
#include "pioneer_q38_token_native_batched_output_closure_role.h"
static_assert(PioneerQ38TokenWideGdnDataSelection==2 && PioneerQ38TokenPrograms::WideGdn==1
 && PioneerQ38TokenPrograms::WideGdnImports==13 && sizeof(PioneerQ38TokenPrograms::PeerGdn)==35944,
 "actual complete new wide-GDN package in this native history");
