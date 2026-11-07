/* SPDX-License-Identifier: MIT */
// Isolated ROOT-build experiment. Recheck every include, including late changes.
#ifndef PIONEER_Q38_EARLY_JOB_SNAPSHOT_BINDING_H
#define PIONEER_Q38_EARLY_JOB_SNAPSHOT_BINDING_H
#ifdef PIONEER_Q38_EARLY_JOB_SNAPSHOT_IMAGE
enum { PioneerQ38EarlyJobSnapshotSelection = 1 };
#else
enum { PioneerQ38EarlyJobSnapshotSelection = 0 };
#endif
#endif

#ifdef PIONEER_Q38_EARLY_JOB_SNAPSHOT_IMAGE
#include "PioneerQ38BatchedOutputClosureRootRole.h"
#if PIONEER_Q38_EARLY_JOB_SNAPSHOT_IMAGE != 1 || !defined(__riscv) || __riscv != 1 \
 || !defined(PIONEER_Q38_BATCHED_OUTPUT_CLOSURE_ROOT_IMAGE) || PIONEER_Q38_BATCHED_OUTPUT_CLOSURE_ROOT_IMAGE != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) || PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE != 1 \
 || !defined(PIONEER_Q38_LAYER_RETURN_COMPONENT_IMAGE) || PIONEER_Q38_LAYER_RETURN_COMPONENT_IMAGE != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION) || PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION) || PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_LOCAL_SOURCE_GUARDS) || PIONEER_Q38_GLOBAL_CHAIN_LOCAL_SOURCE_GUARDS != 1
#error Early job snapshot requires the original returning pushed RAW component BUILD
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(TT_PIONEER_DRIVER_RELOAD) \
 || defined(__FAST_MATH__) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_ROOT_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || defined(PIONEER_Q38_FAST_DECODE_PROFILE_IMAGE) \
 || defined(PIONEER_Q38_BF16_PROFILE_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU) \
 || defined(PIONEER_Q38_PREPARED_Q4_ROOT_IMAGE) || defined(PIONEER_Q38_ROOT_COUNTER_UNROLL_IMAGE)
#error Early job snapshot cannot alias kernel, native, CPU, Work, worker, timed, BF16 or prepared builds
#endif
#if defined(PIONEER_Q38_LAYER_COMPONENT_RECURRENT) == defined(PIONEER_Q38_LAYER_COMPONENT_DENSE)
#error Select exactly one returning component
#endif
#if defined(PIONEER_Q38_LAYER_COMPONENT_RECURRENT) && (PIONEER_Q38_LAYER_COMPONENT_RECURRENT != 1 \
 || !defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE) || PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE != 1 \
 || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE))
#error Invalid recurrent returning component
#endif
#if defined(PIONEER_Q38_LAYER_COMPONENT_DENSE) && (PIONEER_Q38_LAYER_COMPONENT_DENSE != 1 \
 || !defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE != 1 \
 || defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE))
#error Invalid dense returning component
#endif
static_assert(PioneerQ38EarlyJobSnapshotSelection == 1, "late snapshot selection");
#else
static_assert(PioneerQ38EarlyJobSnapshotSelection == 0, "late snapshot removal");
#endif
