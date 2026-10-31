/* SPDX-License-Identifier: MIT */
// Deliberately re-evaluated on every include: no late policy rebinding.
#ifdef PIONEER_Q38_JOB_BOOT_FOLD_IMAGE
#if PIONEER_Q38_JOB_BOOT_FOLD_IMAGE != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION) || PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION) || PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION != 1
#error Job boot fold requires the exact distinct precise QGW3 profile
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) || defined(__FAST_MATH__)
#error Job boot fold is isolated ROOT BUILD or HOST DATA TEST,not kernel/native/CPU/worker/Work
#endif
#ifdef PIONEER_Q38_JOB_BOOT_FOLD_HOST_TEST
#if PIONEER_Q38_JOB_BOOT_FOLD_HOST_TEST != 1 || defined(__riscv) \
 || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE)
#error Exact separate HOST DATA-only job boot fold test required
#endif
#else
#if !defined(__riscv) \
 || (defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) == defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE))
#error Job boot fold device BUILD requires exactly one returning-component or token ROOT role
#endif
#ifdef PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE
#if PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE != 1
#error Exact token ROOT role required for job boot fold
#endif
#else
#if PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE != 1
#error Exact returning component role required for job boot fold
#endif
#endif
#endif
#elif defined(PIONEER_Q38_JOB_BOOT_FOLD_HOST_TEST)
#error A HOST job boot fold test cannot silently use legacy policy
#endif
#ifndef PIONEER_Q38_JOB_BOOT_FOLD_BINDING_H
#define PIONEER_Q38_JOB_BOOT_FOLD_BINDING_H
#ifdef PIONEER_Q38_JOB_BOOT_FOLD_IMAGE
enum {PioneerQ38JobBootFoldProfile=1};
#else
enum {PioneerQ38JobBootFoldProfile=0};
#endif
#endif
#ifdef PIONEER_Q38_JOB_BOOT_FOLD_IMAGE
static_assert(PioneerQ38JobBootFoldProfile==1,"late job boot fold selection");
#else
static_assert(PioneerQ38JobBootFoldProfile==0,"late job boot fold removal");
#endif
