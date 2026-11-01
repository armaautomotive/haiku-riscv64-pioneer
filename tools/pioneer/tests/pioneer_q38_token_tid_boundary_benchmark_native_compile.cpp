/* SPDX-License-Identifier: MIT */
// Bounded diagnostic only;NO CPU oracle or Assistant Work route.
#define PIONEER_Q38_TOKEN_TID_BOUNDARY_PROGRAMS 1
#define PIONEER_Q38_TOKEN_TID_BOUNDARY_BENCHMARK_NATIVE 1
#include "pioneer_q38_token_packet_identity_boundary_benchmark_native_compile.cpp"
static_assert(PioneerQ38TokenTidBoundarySelection==3
 && PioneerQ38TokenTidBoundaryPhysicalSelection==1
 && PioneerQ38TokenPrograms::TidBoundary==1,"actual new bounded diagnostic binding");
