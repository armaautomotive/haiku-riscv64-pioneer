/* SPDX-License-Identifier: MIT */
// Bounded diagnostic only;NO CPU oracle or Assistant Work route.
#define PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_PROGRAMS 1
#define PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_BENCHMARK_NATIVE 1
#include "pioneer_q38_token_cohort_local_sources_benchmark_native_compile.cpp"
static_assert(PioneerQ38TokenPacketIdentityBoundarySelection==3
 && PioneerQ38TokenPacketIdentityBoundaryPhysicalSelection==1
 && PioneerQ38TokenPrograms::PacketIdentityBoundary==1,"actual new bounded diagnostic binding");
