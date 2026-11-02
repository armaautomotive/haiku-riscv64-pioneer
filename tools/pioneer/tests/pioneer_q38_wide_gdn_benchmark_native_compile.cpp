/* SPDX-License-Identifier: MIT */
// New bounded diagnostic subject, not Assistant Work or an installer.
#define PIONEER_Q38_TOKEN_WIDE_GDN_PROGRAMS 1
#define PIONEER_Q38_TOKEN_WIDE_GDN_BENCHMARK_NATIVE 1
#include "pioneer_q38_batched_output_closure_benchmark_native_compile.cpp"
static_assert(PioneerQ38TokenWideGdnDataSelection==3 && PioneerQ38TokenPrograms::WideGdn==1
 && PioneerQ38TokenPrograms::WideGdnDeltaAddress==0x8e600 && PioneerQ38TokenPrograms::WideGdnDeltaWords==64
 && PioneerQ38TokenPrograms::WideGdnImports==13,"actual new returning-GDN DATA in bounded native subject");
