/* SPDX-License-Identifier: MIT */
// Re-evaluate on every include. This is only a distinct ROOT BUILD profile.
#ifdef PIONEER_Q38_TOKEN_FRONTIER_IMAGE
#if PIONEER_Q38_TOKEN_FRONTIER_IMAGE != 1 \
 || !defined(__riscv) || __riscv != 1 \
 || !defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK) || PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK != 1 \
 || !defined(PIONEER_Q38_TOKEN_CANCEL_BOUNDARY) || PIONEER_Q38_TOKEN_CANCEL_BOUNDARY != 1 \
 || !defined(PIONEER_Q38_JOB_BOOT_FOLD_IMAGE) || PIONEER_Q38_JOB_BOOT_FOLD_IMAGE != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION) || PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION) || PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION != 1
#error Frontier publication requires its exact distinct full token ROOT BUILD
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) || defined(__FAST_MATH__)
#error Frontier BUILD is not kernel,native,CPU,worker,returning component or Work
#endif
#endif
#ifndef PIONEER_Q38_TOKEN_FRONTIER_IMAGE_BINDING_H
#define PIONEER_Q38_TOKEN_FRONTIER_IMAGE_BINDING_H
#ifdef PIONEER_Q38_TOKEN_FRONTIER_IMAGE
enum {PioneerQ38TokenFrontierImageProfile=1};
#else
enum {PioneerQ38TokenFrontierImageProfile=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_FRONTIER_IMAGE
static_assert(PioneerQ38TokenFrontierImageProfile==1,"late frontier image selection");
#else
static_assert(PioneerQ38TokenFrontierImageProfile==0,"late frontier image removal");
#endif
