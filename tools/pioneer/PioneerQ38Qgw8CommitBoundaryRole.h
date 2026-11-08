/* SPDX-License-Identifier: MIT */
// Isolated QGW8 ROOT build only. This is not a driver, Work, validation,
// runtime, or device-ownership selector; checks remain on the transaction
// boundary but mid-copy corruption can be detected later than before.
#ifdef PIONEER_Q38_QGW8_COMMIT_BOUNDARY_IMAGE
#if PIONEER_Q38_QGW8_COMMIT_BOUNDARY_IMAGE != 1 \
 || !defined(__riscv) || __riscv != 1 \
 || !defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK) || PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK != 1 \
 || !defined(PIONEER_Q38_TOKEN_CANCEL_BOUNDARY) || PIONEER_Q38_TOKEN_CANCEL_BOUNDARY != 1 \
 || !defined(PIONEER_Q38_FULL_OFFLOAD_ROOT_IMAGE) || PIONEER_Q38_FULL_OFFLOAD_ROOT_IMAGE != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU) || PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP) || PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP != 1 \
 || !defined(PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION) || PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION != 1 \
 || !defined(PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT) || PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT != 1 \
 || !defined(PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION) || PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION != 1 \
 || defined(PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_SEALS_IMAGE) || defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE) \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(__FAST_MATH__)
#error QGW8 commit-boundary optimization requires its isolated ROOT image role
#endif
#endif
#ifndef PIONEER_Q38_QGW8_COMMIT_BOUNDARY_BINDING_H
#define PIONEER_Q38_QGW8_COMMIT_BOUNDARY_BINDING_H
#ifdef PIONEER_Q38_QGW8_COMMIT_BOUNDARY_IMAGE
enum {PioneerQ38Qgw8CommitBoundaryProfile=1};
#else
enum {PioneerQ38Qgw8CommitBoundaryProfile=0};
#endif
#endif
#ifdef PIONEER_Q38_QGW8_COMMIT_BOUNDARY_IMAGE
static_assert(PioneerQ38Qgw8CommitBoundaryProfile==1,"late QGW8 commit-boundary selection");
#else
static_assert(PioneerQ38Qgw8CommitBoundaryProfile==0,"late QGW8 commit-boundary removal");
#endif
