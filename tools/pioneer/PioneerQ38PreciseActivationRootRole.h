/* SPDX-License-Identifier: MIT */
// Deliberately rechecked on every include, including defined-zero roles.
#if !defined(PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION) || PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION != 1
#error Exact distinct precise activation ROOT candidate required
#endif
#if defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE)
#if PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE != 1 || !defined(PIONEER_Q38_GLOBAL_CHAIN_LOCAL_SOURCE_GUARDS) \
 || PIONEER_Q38_GLOBAL_CHAIN_LOCAL_SOURCE_GUARDS != 1 || defined(PIONEER_Q38_GLOBAL_CHAIN_CONV_CAPSULE_IMAGE)
#error Precise activation requires the explicit returning FP32/local-source component profile
#endif
#elif defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE)
#if PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE != 1 || !defined(PIONEER_Q38_TOKEN_CANCEL_BOUNDARY) || PIONEER_Q38_TOKEN_CANCEL_BOUNDARY != 1
#error Precise activation parent requires the explicit persistent cancel profile
#endif
#else
#error Precise activation is not an old ROOT/component image
#endif
#if defined(_KERNEL_MODE) || defined(__FAST_MATH__) || defined(PIONEER_DRIVER_BUILD_TAG) \
 || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) || defined(PIONEER_Q38_PRECISE_WORKER_IMAGE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Precise ROOT profile is not worker, kernel, CPU reference or Work
#endif
