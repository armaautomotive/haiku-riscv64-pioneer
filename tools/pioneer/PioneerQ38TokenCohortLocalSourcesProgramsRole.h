/* SPDX-License-Identifier: MIT */
// Distinct immutable DATA/private context/bounded diagnostic;recheck every include.
#ifndef PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_SELECTION_H
#define PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_SELECTION_H
#ifdef PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_BENCHMARK_NATIVE
enum {PioneerQ38TokenCohortLocalSourcesSelection=3};
#elif defined(PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_NATIVE_CONTEXT)
enum {PioneerQ38TokenCohortLocalSourcesSelection=2};
#elif defined(PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_PROGRAMS)
enum {PioneerQ38TokenCohortLocalSourcesSelection=1};
#else
enum {PioneerQ38TokenCohortLocalSourcesSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_PROGRAMS
#if PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_PROGRAMS) || PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_PROGRAMS != 1 \
 || defined(PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_PROGRAMS_H)
#error Exact separate cohort-local-source DATA before any old code binding required
#endif
#ifdef PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_BENCHMARK_NATIVE
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenCohortLocalSourcesBenchmarkRole.h"
static_assert(PioneerQ38TokenCohortLocalSourcesSelection==3,"late cohort source diagnostic selection");
#elif defined(PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_NATIVE_CONTEXT)
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenCohortLocalSourcesNativeRole.h"
static_assert(PioneerQ38TokenCohortLocalSourcesSelection==2,"late cohort source private selection");
#else
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_RETAINED_COHORT_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_RETAINED_COHORT_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_FRONTIER_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || defined(PIONEER_Q38_COHORT_LOCAL_SOURCES_IMAGE) \
 || defined(PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT) || defined(PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT) \
 || defined(PIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT) || defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_TOKEN_SELECTED_PROGRAMS_H)
#error Cohort source DATA alone has NO native,physical,USER,ROOT BUILD,CPU or Work owner
#endif
static_assert(PioneerQ38TokenCohortLocalSourcesSelection==1,"late cohort source DATA selection");
#endif
#include "PioneerQ38TokenCommitBoundaryProgramsRole.h"
#else
#if defined(PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_BENCHMARK_NATIVE)
#error Cohort source context requires its distinct immutable code package
#endif
static_assert(PioneerQ38TokenCohortLocalSourcesSelection==0,"late cohort source DATA removal");
#endif
