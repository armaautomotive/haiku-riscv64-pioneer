/* SPDX-License-Identifier: MIT */
// Isolated dual-BFP4/BFP8 batching worker; no ROOT, native or Work role.
#if defined(PIONEER_Q38_QGWE_BATCH_DENSE_WORKER) && defined(PIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT) && !defined(PIONEER_Q38_QGWF_BATCH4_WORKER)
#error Dual batching requires the distinct QGWF worker identity
#endif
#ifdef PIONEER_Q38_QGWF_BATCH4_WORKER
#if PIONEER_Q38_QGWF_BATCH4_WORKER != 1 \
 || !defined(PIONEER_Q38_QGWE_BATCH_DENSE_WORKER) || PIONEER_Q38_QGWE_BATCH_DENSE_WORKER != 1 \
 || !defined(PIONEER_Q38_QGW8_BFP8_BATCH_WORKER) || PIONEER_Q38_QGW8_BFP8_BATCH_WORKER != 1 \
 || !defined(PIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT) || PIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(TT_PIONEER_DRIVER_RELOAD) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_BFP4_INPUT_REUSE_EXPERIMENT) \
 || defined(PIONEER_Q38_BFP8_INPUT_REUSE_EXPERIMENT) || defined(PIONEER_Q38_BFP4_BULK_LOAD_EXPERIMENT) \
 || defined(__FAST_MATH__)
#error QGWF requires the isolated dual-batching worker profile
#endif
#endif
#ifndef PIONEER_Q38_QGWF_BATCH4_WORKER_BINDING_H
#define PIONEER_Q38_QGWF_BATCH4_WORKER_BINDING_H
#ifdef PIONEER_Q38_QGWF_BATCH4_WORKER
enum {PioneerQ38QgwfBatch4Worker=1};
#else
enum {PioneerQ38QgwfBatch4Worker=0};
#endif
#endif
#ifdef PIONEER_Q38_QGWF_BATCH4_WORKER
static_assert(PioneerQ38QgwfBatch4Worker==1,"late QGWF selection");
#else
static_assert(PioneerQ38QgwfBatch4Worker==0,"late QGWF removal");
#endif
