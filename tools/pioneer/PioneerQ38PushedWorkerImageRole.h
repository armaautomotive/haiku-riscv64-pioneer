/* SPDX-License-Identifier: MIT */
#if defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS) || defined(PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT)
#error QTP3 DATA/native selectors are not a QGW3 publisher BUILD role
#endif
// Rechecked BUILD-only QGW3 publisher,never a QGW2/native/Work capability.
#if !defined(PIONEER_Q38_PUSH_COMPLETION_WORKER_IMAGE) || PIONEER_Q38_PUSH_COMPLETION_WORKER_IMAGE != 1 \
 || !defined(PIONEER_Q38_PRECISE_WORKER_IMAGE) || PIONEER_Q38_PRECISE_WORKER_IMAGE != 1
#error Exact separate QGW3 completion publisher image required
#endif
#if defined(_KERNEL_MODE) || defined(__FAST_MATH__) || defined(PIONEER_DRIVER_BUILD_TAG) \
 || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) || defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_PRECISE_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE)
#error QGW3 publisher BUILD cannot acquire ROOT,DATA,native,CPU oracle or Work authority
#endif
