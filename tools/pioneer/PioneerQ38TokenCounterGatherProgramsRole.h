/* SPDX-License-Identifier: MIT */
// Exact immutable DATA/private/diagnostic binding,rechecked every include.
#ifndef PIONEER_Q38_TOKEN_COUNTER_GATHER_SELECTION_H
#define PIONEER_Q38_TOKEN_COUNTER_GATHER_SELECTION_H
#ifdef PIONEER_Q38_TOKEN_COUNTER_GATHER_BENCHMARK_NATIVE
enum {PioneerQ38TokenCounterGatherSelection=3};
#elif defined(PIONEER_Q38_TOKEN_COUNTER_GATHER_NATIVE_CONTEXT)
enum {PioneerQ38TokenCounterGatherSelection=2};
#elif defined(PIONEER_Q38_TOKEN_COUNTER_GATHER_PROGRAMS)
enum {PioneerQ38TokenCounterGatherSelection=1};
#else
enum {PioneerQ38TokenCounterGatherSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_COUNTER_GATHER_PROGRAMS
#if PIONEER_Q38_TOKEN_COUNTER_GATHER_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_TID_BOUNDARY_PROGRAMS) || PIONEER_Q38_TOKEN_TID_BOUNDARY_PROGRAMS != 1 \
 || defined(PIONEER_Q38_TOKEN_TID_BOUNDARY_PROGRAMS_H)
#error Exact separate counter-gather DATA before old code binding required
#endif
#ifdef PIONEER_Q38_TOKEN_COUNTER_GATHER_BENCHMARK_NATIVE
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenCounterGatherBenchmarkRole.h"
static_assert(PioneerQ38TokenCounterGatherSelection==3,"late counter-gather diagnostic selection");
#elif defined(PIONEER_Q38_TOKEN_COUNTER_GATHER_NATIVE_CONTEXT)
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenCounterGatherNativeRole.h"
static_assert(PioneerQ38TokenCounterGatherSelection==2,"late counter-gather private selection");
#else
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_TID_BOUNDARY_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_TID_BOUNDARY_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_RETAINED_COHORT_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_RETAINED_COHORT_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_FRONTIER_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || defined(PIONEER_Q38_COUNTER_GATHER_IMAGE) \
 || defined(PIONEER_Q38_TID_BOUNDARY_IMAGE) || defined(PIONEER_Q38_PACKET_IDENTITY_BOUNDARY_IMAGE) \
 || defined(PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT) || defined(PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT) \
 || defined(PIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT) || defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_TOKEN_SELECTED_PROGRAMS_H)
#error Counter-gather DATA alone has NO native,physical,USER,ROOT BUILD,CPU or Work owner
#endif
static_assert(PioneerQ38TokenCounterGatherSelection==1,"late counter-gather DATA selection");
#endif
#include "PioneerQ38TokenTidBoundaryProgramsRole.h"
#else
#if defined(PIONEER_Q38_TOKEN_COUNTER_GATHER_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_COUNTER_GATHER_BENCHMARK_NATIVE)
#error Counter-gather context requires its distinct immutable code package
#endif
static_assert(PioneerQ38TokenCounterGatherSelection==0,"late counter-gather DATA removal");
#endif
