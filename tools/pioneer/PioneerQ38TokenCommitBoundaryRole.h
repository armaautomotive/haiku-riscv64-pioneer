/* SPDX-License-Identifier: MIT */
// ROOT BUILD only;never a native selector or a Work/CPU-validation alias.
#ifdef PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_SEALS_IMAGE
#if PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_SEALS_IMAGE != 1 \
 || !defined(__riscv) || __riscv != 1 \
 || !defined(PIONEER_Q38_TOKEN_FRONTIER_IMAGE) || PIONEER_Q38_TOKEN_FRONTIER_IMAGE != 1 \
 || !defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK) || PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK != 1 \
 || !defined(PIONEER_Q38_TOKEN_CANCEL_BOUNDARY) || PIONEER_Q38_TOKEN_CANCEL_BOUNDARY != 1 \
 || !defined(PIONEER_Q38_JOB_BOOT_FOLD_IMAGE) || PIONEER_Q38_JOB_BOOT_FOLD_IMAGE != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION) || PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION) || PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION != 1
#error Commit-boundary seals require the exact distinct token ROOT BUILD
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_RETAINED_COHORT_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_RETAINED_COHORT_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_RETAINED_CONV_CLOSURE_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(__FAST_MATH__)
#error Commit-boundary BUILD cannot alias native,worker,component,CPU or Work
#endif
#include "PioneerQ38TokenFrontierImageRole.h"
#endif
#ifndef PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_BINDING_H
#define PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_BINDING_H
#ifdef PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_SEALS_IMAGE
enum {PioneerQ38TokenCommitBoundaryProfile=1};
#else
enum {PioneerQ38TokenCommitBoundaryProfile=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_SEALS_IMAGE
static_assert(PioneerQ38TokenCommitBoundaryProfile==1,"late commit-boundary selection");
#else
static_assert(PioneerQ38TokenCommitBoundaryProfile==0,"late commit-boundary removal");
#endif
