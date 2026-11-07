/* SPDX-License-Identifier: MIT */
// Recheck each include. A BUILD selection only, never native/Work authority.
#ifndef PIONEER_Q38_FAST_DECODE_PROFILE_BINDING_H
#define PIONEER_Q38_FAST_DECODE_PROFILE_BINDING_H
#ifdef PIONEER_Q38_FAST_DECODE_PROFILE_IMAGE
enum {PioneerQ38FastDecodeProfileSelection=1};
#else
enum {PioneerQ38FastDecodeProfileSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_FAST_DECODE_PROFILE_IMAGE
#include "PioneerQ38BatchedOutputClosureRootRole.h"
#if PIONEER_Q38_FAST_DECODE_PROFILE_IMAGE != 1 || !defined(__riscv) || __riscv != 1 \
 || !defined(PIONEER_Q38_BATCHED_OUTPUT_CLOSURE_ROOT_IMAGE) || PIONEER_Q38_BATCHED_OUTPUT_CLOSURE_ROOT_IMAGE != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION) || PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION) || PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION != 1
#error Fast decode timing requires the original full-output-closure pushed ROOT BUILD
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(TT_PIONEER_DRIVER_RELOAD) \
 || defined(__FAST_MATH__) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_ROOT_IMAGE) \
 || defined(PIONEER_Q38_BF16_PROFILE_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU) \
 || defined(PIONEER_Q38_PREPARED_Q4_ROOT_IMAGE) || defined(PIONEER_Q38_ROOT_COUNTER_UNROLL_IMAGE)
#error Fast decode timing cannot alias kernel,native,CPU,Work,worker,BF16 or prepared sources
#endif
#ifdef PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE
#if PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE != 1 \
 || !defined(PIONEER_Q38_FULL_OFFLOAD_ROOT_IMAGE) || PIONEER_Q38_FULL_OFFLOAD_ROOT_IMAGE != 1 \
 || !defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE) || PIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE != 1 \
 || !defined(PIONEER_Q38_TOKEN_FRONTIER_IMAGE) || PIONEER_Q38_TOKEN_FRONTIER_IMAGE != 1 \
 || !defined(PIONEER_Q38_TOKEN_CANCEL_BOUNDARY) || PIONEER_Q38_TOKEN_CANCEL_BOUNDARY != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK) || PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK != 1 \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE)
#error Fast decode sampler requires its genuine full-EOG cancel/frontier parent
#endif
#elif defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE)
#if PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_LOCAL_SOURCE_GUARDS) || PIONEER_Q38_GLOBAL_CHAIN_LOCAL_SOURCE_GUARDS != 1 \
 || defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE) || defined(PIONEER_Q38_TOKEN_FRONTIER_IMAGE) \
 || defined(PIONEER_Q38_TOKEN_CANCEL_BOUNDARY) || defined(PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK)
#error Fast decode component requires the original returning local-source parent
#endif
#else
#error Fast decode timing is only a persistent ROOT or returning component BUILD
#endif
static_assert(PioneerQ38FastDecodeProfileSelection==1,"late fast decode selection");
#else
static_assert(PioneerQ38FastDecodeProfileSelection==0,"late fast decode removal");
#endif
