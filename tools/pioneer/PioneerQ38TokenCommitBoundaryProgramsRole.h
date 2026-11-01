/* SPDX-License-Identifier: MIT */
// Distinct DATA,private NODRV or explicit diagnostic. Recheck every include.
#ifndef PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_SELECTION_H
#define PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_SELECTION_H
#ifdef PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_BENCHMARK_NATIVE
enum {PioneerQ38TokenCommitBoundarySelection=3};
#elif defined(PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_NATIVE_CONTEXT)
enum {PioneerQ38TokenCommitBoundarySelection=2};
#elif defined(PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_PROGRAMS)
enum {PioneerQ38TokenCommitBoundarySelection=1};
#else
enum {PioneerQ38TokenCommitBoundarySelection=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_PROGRAMS
#if PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_RETAINED_COHORT_PROGRAMS) || PIONEER_Q38_TOKEN_RETAINED_COHORT_PROGRAMS != 1 \
 || defined(PIONEER_Q38_TOKEN_RETAINED_COHORT_PROGRAMS_H)
#error Exact separate commit-boundary DATA before any old program binding required
#endif
#ifdef PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_BENCHMARK_NATIVE
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenCommitBoundaryBenchmarkRole.h"
static_assert(PioneerQ38TokenCommitBoundarySelection==3,"late commit-boundary physical selection");
#elif defined(PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_NATIVE_CONTEXT)
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenCommitBoundaryNativeRole.h"
static_assert(PioneerQ38TokenCommitBoundarySelection==2,"late commit-boundary private selection");
#else
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_RETAINED_COHORT_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_RETAINED_COHORT_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_FRONTIER_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || defined(PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_SEALS_IMAGE) \
 || defined(PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT) || defined(PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT) \
 || defined(PIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT) || defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_TOKEN_SELECTED_PROGRAMS_H)
#error Commit-boundary DATA has NO private,physical,USER,ROOT BUILD,CPU or Work owner
#endif
static_assert(PioneerQ38TokenCommitBoundarySelection==1,"late commit-boundary DATA selection");
#endif
#include "PioneerQ38TokenRetainedCohortProgramsRole.h"
#else
#if defined(PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_BENCHMARK_NATIVE)
#error Commit-boundary private context requires its actual distinct code package
#endif
static_assert(PioneerQ38TokenCommitBoundarySelection==0,"late commit-boundary DATA removal");
#endif
