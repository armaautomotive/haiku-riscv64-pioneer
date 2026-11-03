/* SPDX-License-Identifier: MIT */
// Rechecked native-only binding. No public install/driver/Work selection.
#ifndef PIONEER_DRAM_COHORT_NATIVE_SELECTION_H
#define PIONEER_DRAM_COHORT_NATIVE_SELECTION_H
#ifdef PIONEER_DRAM_COHORT_NATIVE_CONTEXT
enum {PioneerDramCohortNativeSelection=1};
#else
enum {PioneerDramCohortNativeSelection=0};
#endif
#endif
#ifdef PIONEER_DRAM_COHORT_NATIVE_CONTEXT
#if PIONEER_DRAM_COHORT_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_DRAM_READ_SWEEP_NATIVE_CONTEXT) || PIONEER_DRAM_READ_SWEEP_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_DRAM_SWEEP_CONTEXT_COMPILE_ONLY) || PIONEER_DRAM_SWEEP_CONTEXT_COMPILE_ONLY != 1 \
 || !defined(TT_PIONEER_DRIVER_RELOAD) || !defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH) \
 || !defined(PIONEER_DRIVER_BUILD_TAG) || PIONEER_DRIVER_BUILD_TAG != 0x4452434e4f445256ULL
#error Private DRCNODRV requires its actual native lease and inert renamed SDK subject
#endif
#if defined(PIONEER_DRAM_SWEEP_DIAGNOSTIC) || defined(PIONEER_DRAM_SWEEP_QUEUED) || defined(PIONEER_DRAM_SWEEP_WIDE) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(__FAST_MATH__)
#error Cohort native context cannot alias old physical roles,model,CPU reference or Work
#endif
static_assert(PioneerDramCohortNativeSelection==1,"late cohort native selection");
#else
static_assert(PioneerDramCohortNativeSelection==0,"late cohort native removal");
#endif
