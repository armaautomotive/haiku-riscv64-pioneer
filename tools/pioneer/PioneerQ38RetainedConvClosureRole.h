/* SPDX-License-Identifier: MIT */
// Rechecked isolated returning-component BUILD policy,not Work authority.
#ifdef PIONEER_Q38_RETAINED_CONV_CLOSURE_IMAGE
#if PIONEER_Q38_RETAINED_CONV_CLOSURE_IMAGE != 1 \
 || !defined(__riscv) || __riscv != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) || PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE != 1 \
 || !defined(PIONEER_Q38_LAYER_COMPONENT_RECURRENT) || PIONEER_Q38_LAYER_COMPONENT_RECURRENT != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_LOCAL_SOURCE_GUARDS) || PIONEER_Q38_GLOBAL_CHAIN_LOCAL_SOURCE_GUARDS != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_WORKER_SEALED_ADMISSION) || PIONEER_Q38_GLOBAL_CHAIN_WORKER_SEALED_ADMISSION != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION) || PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION) || PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION != 1 \
 || !defined(PIONEER_Q38_JOB_BOOT_FOLD_IMAGE) || PIONEER_Q38_JOB_BOOT_FOLD_IMAGE != 1
#error Retained Conv closure requires the exact isolated returning recurrent QGW3 BUILD
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) || defined(PIONEER_Q38_LAYER_COMPONENT_DENSE) \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_CONV_CAPSULE_IMAGE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(__FAST_MATH__)
#error Retained Conv closure BUILD cannot alias another component,native,CPU or Work
#endif
#endif
#ifndef PIONEER_Q38_RETAINED_CONV_CLOSURE_BINDING_H
#define PIONEER_Q38_RETAINED_CONV_CLOSURE_BINDING_H
#ifdef PIONEER_Q38_RETAINED_CONV_CLOSURE_IMAGE
enum {PioneerQ38RetainedConvClosureProfile=1};
#else
enum {PioneerQ38RetainedConvClosureProfile=0};
#endif
#endif
#ifdef PIONEER_Q38_RETAINED_CONV_CLOSURE_IMAGE
static_assert(PioneerQ38RetainedConvClosureProfile==1,"late retained Conv closure selection");
#else
static_assert(PioneerQ38RetainedConvClosureProfile==0,"late retained Conv closure removal");
#endif
