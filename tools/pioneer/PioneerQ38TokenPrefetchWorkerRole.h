/* SPDX-License-Identifier: MIT */
// Rechecked on every include. Firmware BUILD only;no native/Work grant.
#ifndef PIONEER_Q38_TOKEN_PREFETCH_WORKER_BINDING_H
#define PIONEER_Q38_TOKEN_PREFETCH_WORKER_BINDING_H
#ifdef PIONEER_Q38_TOKEN_PREFETCH_WORKER_IMAGE
enum {PioneerQ38TokenPrefetchWorkerSelection=1};
#else
enum {PioneerQ38TokenPrefetchWorkerSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_PREFETCH_WORKER_IMAGE
#if PIONEER_Q38_TOKEN_PREFETCH_WORKER_IMAGE != 1 \
 || !defined(PIONEER_Q38_WORK_TILE_PREFETCH) || PIONEER_Q38_WORK_TILE_PREFETCH != 1 \
 || !defined(PIONEER_Q38_MIXED_CAPSULE_IMAGE) || PIONEER_Q38_MIXED_CAPSULE_IMAGE != 1 \
 || !defined(PIONEER_Q38_FFN_PERSISTENT) || PIONEER_Q38_FFN_PERSISTENT != 1 \
 || !defined(PIONEER_Q38_PROJECTION_PERSISTENT) || PIONEER_Q38_PROJECTION_PERSISTENT != 1 \
 || !defined(PIONEER_Q38_RESIDENT_OPERATOR_IMAGE) || PIONEER_Q38_RESIDENT_OPERATOR_IMAGE != 1 \
 || !defined(PIONEER_Q38_RESIDENT_GDN_OPERATOR_IMAGE) || PIONEER_Q38_RESIDENT_GDN_OPERATOR_IMAGE != 1 \
 || !defined(PIONEER_Q38_RESIDENT_GDN_OUTPUT_OPERATOR_IMAGE) || PIONEER_Q38_RESIDENT_GDN_OUTPUT_OPERATOR_IMAGE != 1 \
 || !defined(PIONEER_Q38_WORK_DIRECT_SRCB) || PIONEER_Q38_WORK_DIRECT_SRCB != 1 \
 || !defined(PIONEER_Q38_WORK_AFFINE_REPLAY) || PIONEER_Q38_WORK_AFFINE_REPLAY != 1
#error Exact distinct full-model prefetch firmware and current returning-math parents required
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_FLAT_MATRIX_PROGRAMS) \
 || defined(PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS) || defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS) \
 || defined(PIONEER_Q38_TILE_PREFETCH_QUALIFICATION) || defined(PIONEER_Q38_RESIDENT_COHORT_ROOT_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) \
 || defined(PIONEER_Q38_PRECISE_WORKER_IMAGE) || defined(PIONEER_Q38_PUSH_COMPLETION_WORKER_IMAGE) \
 || defined(PIONEER_Q38_WORK_PEER_PHASE_PROFILE) || defined(PIONEER_Q38_WORK_LOCAL_Q4) \
 || defined(PIONEER_Q38_WORK_Q4_NIBBLE) || defined(PIONEER_Q38_WORK_SIGNED_MAG2) \
 || defined(PIONEER_Q38_WORK_FRONTEND_BARRIER) || defined(PIONEER_Q38_WORK_RAW_UNPACK_CACHE) \
 || defined(PIONEER_Q38_WORK_BATCH4_FETCH) || defined(PIONEER_Q38_WORK_BATCH_RAW_INPUT) \
 || defined(PIONEER_Q38_NATIVE_Q8) || defined(PIONEER_Q38_PREDECODED_Q4) \
 || defined(PIONEER_Q38_PREPARED_Q4_BLOCK) || defined(PIONEER_Q38_PREPARED_Q4_AFFINE) \
 || defined(PIONEER_Q38_BFLOAT_PANEL) || defined(PIONEER_Q38_Q4_GROUP32) || defined(__FAST_MATH__)
#error Prefetch shader BUILD cannot alias a native owner,ROOT,old image,CPU oracle or Work selection
#endif
static_assert(PioneerQ38TokenPrefetchWorkerSelection==1,"late full-model prefetch firmware selection");
#else
#if defined(PIONEER_Q38_WORK_TILE_PREFETCH) && defined(PIONEER_Q38_PROJECTION_PERSISTENT)
#error Returning projection prefetch requires its distinct full-model shader selection
#endif
static_assert(PioneerQ38TokenPrefetchWorkerSelection==0,"late full-model prefetch firmware removal");
#endif
