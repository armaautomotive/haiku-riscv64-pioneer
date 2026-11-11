/* SPDX-License-Identifier: MIT */
// Offline-only QGWD token-count publisher. It grants no native reader,
// device ownership, Work authority, numerical quality or speed claim.
#if defined(PIONEER_Q38_QGWD_FRONTIER_IMAGE) && (PIONEER_Q38_QGWD_FRONTIER_IMAGE != 1 \
 || !defined(__riscv) || __riscv != 1 \
 || !defined(PIONEER_Q38_QGWD_ROOT_IMAGE) || PIONEER_Q38_QGWD_ROOT_IMAGE != 1 \
 || !defined(PIONEER_Q38_TOKEN_FRONTIER_IMAGE) || PIONEER_Q38_TOKEN_FRONTIER_IMAGE != 1 \
 || !defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK) || PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK != 1 \
 || !defined(PIONEER_Q38_TOKEN_CANCEL_BOUNDARY) || PIONEER_Q38_TOKEN_CANCEL_BOUNDARY != 1 \
 || !defined(PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION) || PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU) || PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP) || PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP != 1 \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION) \
 || defined(PIONEER_Q38_BF16_PROFILE_IMAGE) || defined(PIONEER_Q38_QGW8_COARSE_PROFILE_IMAGE) \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(TT_PIONEER_DRIVER_RELOAD) \
 || defined(PIONEER_Q38_HYBRID_FULL_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) \
 || defined(PIONEER_Q38_HYBRID_FULL_CLIENT) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_BF16_FULL_NATIVE) || defined(PIONEER_Q38_BF16_FULL_CLIENT) \
 || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__))
#error QGWD frontier is a distinct ROOT build-only role
#endif
#ifndef PIONEER_Q38_QGWD_FRONTIER_IMAGE_BINDING_H
#define PIONEER_Q38_QGWD_FRONTIER_IMAGE_BINDING_H
#ifdef PIONEER_Q38_QGWD_FRONTIER_IMAGE
enum {PioneerQ38QgwdFrontierImageRole=1};
#else
enum {PioneerQ38QgwdFrontierImageRole=0};
#endif
#endif
#ifdef PIONEER_Q38_QGWD_FRONTIER_IMAGE
static_assert(PioneerQ38QgwdFrontierImageRole==1,"late QGWD frontier selection");
#else
static_assert(PioneerQ38QgwdFrontierImageRole==0,"late QGWD frontier removal");
#endif
