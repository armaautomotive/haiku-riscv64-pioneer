/* SPDX-License-Identifier: MIT */
#include "PioneerQ38ManualTTSyncRole.h"
// Rechecked on every inclusion. Emitted-worker BUILD only, never a native
// selector or permission to install/execute a changed image.
#ifndef PIONEER_Q38_Q4_CONSTANT_ISSUE_BINDING_H
#define PIONEER_Q38_Q4_CONSTANT_ISSUE_BINDING_H
#ifdef PIONEER_Q38_Q4_CONSTANT_ISSUE_IMAGE
enum { PioneerQ38Q4ConstantIssueSelection = 1 };
#else
enum { PioneerQ38Q4ConstantIssueSelection = 0 };
#endif
#endif
#ifdef PIONEER_Q38_Q4_CONSTANT_ISSUE_IMAGE
#if PIONEER_Q38_Q4_CONSTANT_ISSUE_IMAGE != 1 \
 || !defined(PIONEER_Q38_Q4_CONSTANT_ISSUE) || PIONEER_Q38_Q4_CONSTANT_ISSUE != 1 \
 || !defined(PIONEER_Q38_Q4_CONSTANT_ISSUE_PARENT_BINDING) || PIONEER_Q38_Q4_CONSTANT_ISSUE_PARENT_BINDING != 1 \
 || !defined(PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_WORKER_IMAGE) || PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_WORKER_IMAGE != 1 \
 || !defined(PIONEER_Q38_WORK_TWIN_BATCH_PREFETCH) || PIONEER_Q38_WORK_TWIN_BATCH_PREFETCH != 1 \
 || !defined(PIONEER_Q38_STATIC_VECTOR) || !defined(PIONEER_Q38_REPLAY_OPERAND) \
 || !defined(PIONEER_Q38_NATIVE_OPERAND_STORE) || !defined(PIONEER_Q38_VECTOR_INLINE)
#error Constant Q4 issue requires its distinct full returning twin worker BUILD
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(__FAST_MATH__) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_WORK_PEER_PHASE_PROFILE) || defined(PIONEER_Q38_WORK_LOCAL_Q4) \
 || defined(PIONEER_Q38_WORK_Q4_NIBBLE) || defined(PIONEER_Q38_WORK_Q4_DIRECT_DECODE) \
 || defined(PIONEER_Q38_WORK_SIGNED_MAG2) || defined(PIONEER_Q38_WORK_RAW_UNPACK_CACHE) \
 || defined(PIONEER_Q38_WORK_FRONTEND_BARRIER) || defined(PIONEER_Q38_NATIVE_Q8) \
 || defined(PIONEER_Q38_PREDECODED_Q4) || defined(PIONEER_Q38_PREPARED_Q4_BLOCK) \
 || defined(PIONEER_Q38_PREPARED_Q4_AFFINE) || defined(PIONEER_Q38_BFLOAT_PANEL) \
 || defined(PIONEER_Q38_Q4_GROUP32) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE)
#error Constant Q4 issue BUILD cannot alias a native owner, oracle or old experiment
#endif
static_assert(PioneerQ38Q4ConstantIssueSelection == 1, "late constant Q4 selection");
#else
#if defined(PIONEER_Q38_Q4_CONSTANT_ISSUE) || defined(PIONEER_Q38_Q4_CONSTANT_ISSUE_PARENT_BINDING)
#error Constant Q4 issue requires its separate full-worker BUILD selection
#endif
static_assert(PioneerQ38Q4ConstantIssueSelection == 0, "late constant Q4 removal");
#endif
