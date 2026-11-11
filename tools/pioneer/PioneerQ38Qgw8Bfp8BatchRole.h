/* SPDX-License-Identifier: MIT */
// Separate worker-only batching experiment; no ROOT or device authority.
// QGWE combines the existing dense-Q8/SrcA worker with BFP8 input batching.
// Its distinct boot/job identity must never be accepted by an older ROOT.
#ifdef PIONEER_Q38_QGWE_BATCH_DENSE_WORKER
#if PIONEER_Q38_QGWE_BATCH_DENSE_WORKER != 1 \
 || !defined(PIONEER_Q38_QGWD_SRCA_DENSE_Q8_WORKER) || PIONEER_Q38_QGWD_SRCA_DENSE_Q8_WORKER != 1 \
 || !defined(PIONEER_Q38_QGW8_BFP8_BATCH_WORKER) || PIONEER_Q38_QGW8_BFP8_BATCH_WORKER != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(TT_PIONEER_DRIVER_RELOAD) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error QGWE is an isolated combined worker experiment
#endif
#endif
#ifndef PIONEER_Q38_QGWE_BATCH_DENSE_BINDING_H
#define PIONEER_Q38_QGWE_BATCH_DENSE_BINDING_H
#ifdef PIONEER_Q38_QGWE_BATCH_DENSE_WORKER
enum {PioneerQ38QgweBatchDenseWorker=1};
#else
enum {PioneerQ38QgweBatchDenseWorker=0};
#endif
#endif
#ifdef PIONEER_Q38_QGWE_BATCH_DENSE_WORKER
static_assert(PioneerQ38QgweBatchDenseWorker==1,"late QGWE selection");
#else
static_assert(PioneerQ38QgweBatchDenseWorker==0,"late QGWE removal");
#endif
#if defined(PIONEER_Q38_QGW8_BFP8_BATCH_WORKER)
#if PIONEER_Q38_QGW8_BFP8_BATCH_WORKER != 1 \
 || !defined(PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION) || PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION != 1 \
 || !defined(PIONEER_Q38_HYBRID_WORKER_IMAGE) || PIONEER_Q38_HYBRID_WORKER_IMAGE != 1 \
 || !defined(PIONEER_Q38_BF16_WORKER_IMAGE) || PIONEER_Q38_BF16_WORKER_IMAGE != 1 \
 || !defined(PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION) || PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION != 1 \
 || !defined(PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT) || PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT != 1 \
 || defined(PIONEER_Q38_QGW8_BFP8_REUSE_WORKER) || defined(PIONEER_Q38_BFP8_INPUT_REUSE_EXPERIMENT) \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(TT_PIONEER_DRIVER_RELOAD) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY) \
 || defined(PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY) || defined(PIONEER_Q38_QGW8_HIDDEN_NATIVE_129) \
 || defined(PIONEER_Q38_QGW8_NATIVE_INSPECTION) || defined(PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128) \
 || defined(PIONEER_Q38_HYBRID_FULL_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) \
 || defined(PIONEER_Q38_HYBRID_FULL_CLIENT) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_BF16_FULL_NATIVE) || defined(PIONEER_Q38_BF16_FULL_CLIENT) \
 || defined(PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION) || defined(PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION) \
 || defined(PIONEER_Q38_QGW8_HIDDEN_CLIENT_129) || defined(PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128) \
 || defined(PIONEER_Q38_HYBRID_INPUT_REUSE_INSPECTION) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error BFP8 batching is an isolated worker-only experiment
#endif
#endif
#ifndef PIONEER_Q38_QGW8_BFP8_BATCH_ROLE_H
#define PIONEER_Q38_QGW8_BFP8_BATCH_ROLE_H
#ifdef PIONEER_Q38_QGW8_BFP8_BATCH_WORKER
enum {PioneerQ38Qgw8Bfp8BatchWorker=1};
#else
enum {PioneerQ38Qgw8Bfp8BatchWorker=0};
#endif
#endif
#ifdef PIONEER_Q38_QGW8_BFP8_BATCH_WORKER
static_assert(PioneerQ38Qgw8Bfp8BatchWorker==1,"late batching selector");
#else
static_assert(PioneerQ38Qgw8Bfp8BatchWorker==0,"late batching removal");
#endif
