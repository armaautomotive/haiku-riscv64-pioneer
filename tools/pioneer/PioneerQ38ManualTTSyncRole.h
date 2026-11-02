/* SPDX-License-Identifier: MIT */
// Rechecked on every inclusion. Shader BUILD only;not a native/Work route.
#ifndef PIONEER_Q38_MANUAL_TTSYNC_BINDING_H
#define PIONEER_Q38_MANUAL_TTSYNC_BINDING_H
#ifdef PIONEER_Q38_MANUAL_TTSYNC_IMAGE
enum {PioneerQ38ManualTTSyncSelection=1};
#else
enum {PioneerQ38ManualTTSyncSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_MANUAL_TTSYNC_IMAGE
#if PIONEER_Q38_MANUAL_TTSYNC_IMAGE != 1 \
 || !defined(PIONEER_Q38_MANUAL_TTSYNC_PARENT_BINDING) || PIONEER_Q38_MANUAL_TTSYNC_PARENT_BINDING != 1 \
 || !defined(PIONEER_Q38_Q4_CONSTANT_ISSUE_IMAGE) || PIONEER_Q38_Q4_CONSTANT_ISSUE_IMAGE != 1 \
 || !defined(PIONEER_Q38_Q4_CONSTANT_ISSUE_PARENT_BINDING) || PIONEER_Q38_Q4_CONSTANT_ISSUE_PARENT_BINDING != 1 \
 || !defined(PIONEER_Q38_Q4_CONSTANT_ISSUE) || PIONEER_Q38_Q4_CONSTANT_ISSUE != 1 \
 || !defined(PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_WORKER_IMAGE) || PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_WORKER_IMAGE != 1 \
 || !defined(PIONEER_Q38_WORK_TWIN_BATCH_PREFETCH) || PIONEER_Q38_WORK_TWIN_BATCH_PREFETCH != 1 \
 || !defined(PIONEER_Q38_STATIC_VECTOR) || !defined(PIONEER_Q38_REPLAY_OPERAND) \
 || !defined(PIONEER_Q38_NATIVE_OPERAND_STORE) || !defined(PIONEER_Q38_VECTOR_INLINE)
#error Manual TTSync requires its separate full returning constant-Q4 shader BUILD
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(__FAST_MATH__) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) \
 || defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT) || defined(PIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT) \
 || defined(PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT) \
 || defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_IMAGE) || defined(PIONEER_Q38_WORK_PEER_PHASE_PROFILE) \
 || defined(PIONEER_Q38_WORK_FRONTEND_BARRIER) || defined(PIONEER_Q38_WORK_RAW_UNPACK_CACHE) \
 || defined(PIONEER_Q38_WORK_LOCAL_Q4) || defined(PIONEER_Q38_PREDECODED_Q4) \
 || defined(PIONEER_Q38_PREPARED_Q4_BLOCK) || defined(PIONEER_Q38_PREPARED_Q4_AFFINE) \
 || defined(PIONEER_Q38_NATIVE_Q8) || defined(PIONEER_Q38_BFLOAT_PANEL) \
 || defined(PIONEER_Q38_Q4_GROUP32)
#error Manual TTSync BUILD cannot alias a native owner,oracle,Work or old experiment
#endif
static_assert(PioneerQ38ManualTTSyncSelection==1,"late manual completion selection");
#else
#ifdef PIONEER_Q38_MANUAL_TTSYNC_PARENT_BINDING
#error Manual TTSync parent requires its distinct shader BUILD selection
#endif
static_assert(PioneerQ38ManualTTSyncSelection==0,"late manual completion removal");
#endif
