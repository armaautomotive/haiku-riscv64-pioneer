/* SPDX-License-Identifier: MIT */
// Recheck every include. Profiling is a distinct BUILD, never a Work grant.
#ifdef PIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE
#ifdef PIONEER_Q38_BF16_PROFILE_IMAGE
#include "PioneerQ38Bf16ProfileBuildRole.h"
#else
#if PIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE != 1 \
 || !defined(__riscv) || __riscv != 1 \
 || !defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK) || PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK != 1 \
 || !defined(PIONEER_Q38_TOKEN_CANCEL_BOUNDARY) || PIONEER_Q38_TOKEN_CANCEL_BOUNDARY != 1 \
 || !defined(PIONEER_Q38_TOKEN_FRONTIER_IMAGE) || PIONEER_Q38_TOKEN_FRONTIER_IMAGE != 1 \
 || !defined(PIONEER_Q38_BATCHED_OUTPUT_CLOSURE_ROOT_IMAGE) || PIONEER_Q38_BATCHED_OUTPUT_CLOSURE_ROOT_IMAGE != 1
#error Phase timing requires its exact distinct full output-closure token ROOT BUILD
#endif
#if defined(_KERNEL_MODE) || defined(__FAST_MATH__) || defined(PIONEER_DRIVER_BUILD_TAG) \
 || defined(TT_PIONEER_DRIVER_RELOAD) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_WIDE_GDN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_WIDE_GDN_PROGRAMS) || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_ROOT_IMAGE) \
 || defined(PIONEER_Q38_ROOT_CYCLE_IMAGE) || defined(PIONEER_Q38_TOKEN_FRONTEND_PROFILE_NATIVE)
#error Phase timing BUILD is not kernel,native,CPU,worker,old parent or Work
#endif
#endif
#endif
#ifndef PIONEER_Q38_TOKEN_PHASE_PROFILE_BINDING_H
#define PIONEER_Q38_TOKEN_PHASE_PROFILE_BINDING_H
#ifdef PIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE
enum {PioneerQ38TokenPhaseProfileSelection=1};
#else
enum {PioneerQ38TokenPhaseProfileSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE
static_assert(PioneerQ38TokenPhaseProfileSelection==1,"late phase timing selection");
#else
static_assert(PioneerQ38TokenPhaseProfileSelection==0,"late phase timing removal");
#endif
