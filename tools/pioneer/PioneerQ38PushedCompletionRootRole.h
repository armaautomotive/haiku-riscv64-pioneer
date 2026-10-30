/* SPDX-License-Identifier: MIT */
// Rechecked BUILD experiment only. Not a worker/native/DATA/Work role.
#if !defined(PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION) || PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION) || PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION != 1
#error Exact distinct QGW3 completion-receiving ROOT profile required
#endif
#if defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE)
#include "PioneerQ38WorkerSealedAdmissionRole.h"
#if defined(PIONEER_Q38_LAYER_COMPONENT_RECURRENT)
#if !defined(PIONEER_Q38_GLOBAL_CHAIN_CROSS_TOKEN_COMPONENT) || PIONEER_Q38_GLOBAL_CHAIN_CROSS_TOKEN_COMPONENT != 1
#error QGW3 recurrent receiver requires the exact cross-token returning body
#endif
#elif defined(PIONEER_Q38_LAYER_COMPONENT_DENSE)
#if !defined(PIONEER_Q38_GLOBAL_CHAIN_TOKEN_KV_APPEND) || PIONEER_Q38_GLOBAL_CHAIN_TOKEN_KV_APPEND != 1
#error QGW3 dense receiver requires the exact token-KV returning body
#endif
#else
#error QGW3 receiver is not a convolution capsule or unbound component
#endif
#elif defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE)
#if PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE != 1 || !defined(PIONEER_Q38_TOKEN_CANCEL_BOUNDARY) || PIONEER_Q38_TOKEN_CANCEL_BOUNDARY != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK) || PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK != 1
#error QGW3 persistent parent requires the exact cancel/feedback profile
#endif
#else
#error Pushed completion is not an old ROOT or standalone DATA role
#endif
#if defined(_KERNEL_MODE) || defined(__FAST_MATH__) || defined(PIONEER_DRIVER_BUILD_TAG) \
 || defined(PIONEER_Q38_PUSH_COMPLETION_WORKER_IMAGE) || defined(PIONEER_Q38_PRECISE_WORKER_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_CONV_CAPSULE_IMAGE) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_PRECISE_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE)
#error QGW3 receiver BUILD is not worker, capsule, kernel, native, CPU, DATA or Work authority
#endif
