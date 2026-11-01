/* SPDX-License-Identifier: MIT */
// Exact new immutable DATA/private/diagnostic binding;recheck every include.
#ifndef PIONEER_Q38_TOKEN_TID_BOUNDARY_SELECTION_H
#define PIONEER_Q38_TOKEN_TID_BOUNDARY_SELECTION_H
#ifdef PIONEER_Q38_TOKEN_TID_BOUNDARY_BENCHMARK_NATIVE
enum {PioneerQ38TokenTidBoundarySelection=3};
#elif defined(PIONEER_Q38_TOKEN_TID_BOUNDARY_NATIVE_CONTEXT)
enum {PioneerQ38TokenTidBoundarySelection=2};
#elif defined(PIONEER_Q38_TOKEN_TID_BOUNDARY_PROGRAMS)
enum {PioneerQ38TokenTidBoundarySelection=1};
#else
enum {PioneerQ38TokenTidBoundarySelection=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_TID_BOUNDARY_PROGRAMS
#if PIONEER_Q38_TOKEN_TID_BOUNDARY_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_PROGRAMS) || PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_PROGRAMS != 1 \
 || defined(PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_PROGRAMS_H)
#error Exact separate TID-boundary DATA before old code binding required
#endif
#ifdef PIONEER_Q38_TOKEN_TID_BOUNDARY_BENCHMARK_NATIVE
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenTidBoundaryBenchmarkRole.h"
static_assert(PioneerQ38TokenTidBoundarySelection==3,"late TID-boundary diagnostic selection");
#elif defined(PIONEER_Q38_TOKEN_TID_BOUNDARY_NATIVE_CONTEXT)
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenTidBoundaryNativeRole.h"
static_assert(PioneerQ38TokenTidBoundarySelection==2,"late TID-boundary private selection");
#else
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_RETAINED_COHORT_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_RETAINED_COHORT_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_FRONTIER_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || defined(PIONEER_Q38_TID_BOUNDARY_IMAGE) \
 || defined(PIONEER_Q38_PACKET_IDENTITY_BOUNDARY_IMAGE) \
 || defined(PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT) || defined(PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT) \
 || defined(PIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT) || defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_TOKEN_SELECTED_PROGRAMS_H)
#error Packet-identity DATA alone has NO native,physical,USER,ROOT BUILD,CPU or Work owner
#endif
static_assert(PioneerQ38TokenTidBoundarySelection==1,"late TID-boundary DATA selection");
#endif
#include "PioneerQ38TokenPacketIdentityBoundaryProgramsRole.h"
#else
#if defined(PIONEER_Q38_TOKEN_TID_BOUNDARY_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_TID_BOUNDARY_BENCHMARK_NATIVE)
#error Packet-identity context requires its distinct immutable code package
#endif
static_assert(PioneerQ38TokenTidBoundarySelection==0,"late TID-boundary DATA removal");
#endif
