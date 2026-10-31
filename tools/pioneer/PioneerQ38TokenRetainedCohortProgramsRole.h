/* SPDX-License-Identifier: MIT */
// Distinct DATA,private NODRV or explicit diagnostic;never a Work alias.
#ifndef PIONEER_Q38_TOKEN_RETAINED_COHORT_SELECTION_H
#define PIONEER_Q38_TOKEN_RETAINED_COHORT_SELECTION_H
#ifdef PIONEER_Q38_TOKEN_RETAINED_COHORT_BENCHMARK_NATIVE
enum {PioneerQ38TokenRetainedCohortSelection=3};
#elif defined(PIONEER_Q38_TOKEN_RETAINED_COHORT_NATIVE_CONTEXT)
enum {PioneerQ38TokenRetainedCohortSelection=2};
#elif defined(PIONEER_Q38_TOKEN_RETAINED_COHORT_PROGRAMS)
enum {PioneerQ38TokenRetainedCohortSelection=1};
#else
enum {PioneerQ38TokenRetainedCohortSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_RETAINED_COHORT_PROGRAMS
#if PIONEER_Q38_TOKEN_RETAINED_COHORT_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS) || PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS != 1 \
 || defined(PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS_H)
#error Exact distinct retained-cohort DATA before old program selection required
#endif
#ifdef PIONEER_Q38_TOKEN_RETAINED_COHORT_BENCHMARK_NATIVE
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenRetainedCohortBenchmarkRole.h"
static_assert(PioneerQ38TokenRetainedCohortSelection==3,"late retained-cohort physical selection");
#elif defined(PIONEER_Q38_TOKEN_RETAINED_COHORT_NATIVE_CONTEXT)
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenRetainedCohortNativeRole.h"
static_assert(PioneerQ38TokenRetainedCohortSelection==2,"late retained-cohort private selection");
#else
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_FRONTIER_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_DECODE_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT) || defined(PIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT) \
 || defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT) || defined(PIONEER_Q38_RETAINED_CONV_CLOSURE_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(__FAST_MATH__) \
 || defined(PIONEER_Q38_TOKEN_SELECTED_PROGRAMS_H)
#error Retained-cohort program DATA has NO native,physical,USER,CPU,image or Work owner
#endif
static_assert(PioneerQ38TokenRetainedCohortSelection==1,"late retained-cohort DATA selection");
#endif
#include "PioneerQ38TokenFrontierProgramsRole.h"
#else
#if defined(PIONEER_Q38_TOKEN_RETAINED_COHORT_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_RETAINED_COHORT_BENCHMARK_NATIVE)
#error Retained-cohort private context requires its actual distinct code package
#endif
static_assert(PioneerQ38TokenRetainedCohortSelection==0,"late retained-cohort DATA removal");
#endif
