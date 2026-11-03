/* SPDX-License-Identifier: MIT */
// Rechecked binding:inert private DRCNODRV or explicit bounded diagnostic77.
// Neither role selects model/CPU-reference/Assistant Work inference.
#ifndef PIONEER_DRAM_COHORT_NATIVE_SELECTION_H
#define PIONEER_DRAM_COHORT_NATIVE_SELECTION_H
#ifdef PIONEER_DRAM_COHORT_NATIVE_CONTEXT
enum {PioneerDramCohortNativeSelection=1};
#else
enum {PioneerDramCohortNativeSelection=0};
#endif
#ifdef PIONEER_DRAM_COHORT_DIAGNOSTIC
enum {PioneerDramCohortPhysicalSelection=1};
#else
enum {PioneerDramCohortPhysicalSelection=0};
#endif
#ifdef PIONEER_DRAM_COHORT_DIRECT_TLB
enum {PioneerDramCohortDirectSelection=1};
#else
enum {PioneerDramCohortDirectSelection=0};
#endif
#endif
#ifdef PIONEER_DRAM_COHORT_DIRECT_TLB
#if PIONEER_DRAM_COHORT_DIRECT_TLB != 1 || !defined(PIONEER_DRAM_COHORT_DIAGNOSTIC)
#error Direct TLB checks require the explicit separate physical cohort revision
#endif
static_assert(PioneerDramCohortDirectSelection==1,"late direct TLB selection");
#else
static_assert(PioneerDramCohortDirectSelection==0,"late direct TLB removal");
#endif
#ifdef PIONEER_DRAM_COHORT_NATIVE_CONTEXT
#if PIONEER_DRAM_COHORT_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_DRAM_READ_SWEEP_NATIVE_CONTEXT) || PIONEER_DRAM_READ_SWEEP_NATIVE_CONTEXT != 1 \
 || !defined(TT_PIONEER_DRIVER_RELOAD) || !defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH) \
 || !defined(PIONEER_DRIVER_BUILD_TAG)
#error Cohort context requires its actual scoped parent lease
#endif
#ifdef PIONEER_DRAM_COHORT_DIAGNOSTIC
static_assert(PioneerDramCohortPhysicalSelection==1,"late physical cohort selection");
#if PIONEER_DRAM_COHORT_DIAGNOSTIC != 1 || defined(PIONEER_DRAM_SWEEP_CONTEXT_COMPILE_ONLY) \
 || (defined(PIONEER_DRAM_COHORT_DIRECT_TLB) && PIONEER_DRIVER_BUILD_TAG != 0x2026110300020077ULL) \
 || (!defined(PIONEER_DRAM_COHORT_DIRECT_TLB) && PIONEER_DRIVER_BUILD_TAG != 0x2026110300010077ULL)
#error Cohort diagnostic77 requires its distinct physical role,never DRCNODRV
#endif
#else
static_assert(PioneerDramCohortPhysicalSelection==0,"late physical cohort removal");
#if !defined(PIONEER_DRAM_SWEEP_CONTEXT_COMPILE_ONLY) || PIONEER_DRAM_SWEEP_CONTEXT_COMPILE_ONLY != 1 \
 || PIONEER_DRIVER_BUILD_TAG != 0x4452434e4f445256ULL
#error Private DRCNODRV requires its inert renamed SDK subject
#endif
#endif
#if defined(PIONEER_DRAM_SWEEP_DIAGNOSTIC) || defined(PIONEER_DRAM_SWEEP_QUEUED) || defined(PIONEER_DRAM_SWEEP_WIDE) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(__FAST_MATH__)
#error Cohort native context cannot alias old physical roles,model,CPU reference or Work
#endif
static_assert(PioneerDramCohortNativeSelection==1,"late cohort native selection");
#else
#ifdef PIONEER_DRAM_COHORT_DIAGNOSTIC
#error Physical cohort diagnostic requires its exact native cohort context
#endif
static_assert(PioneerDramCohortPhysicalSelection==0,"late physical cohort selection without native context");
static_assert(PioneerDramCohortNativeSelection==0,"late cohort native removal");
#endif
