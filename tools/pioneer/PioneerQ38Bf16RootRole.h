/* SPDX-License-Identifier: MIT */
// BUILD role only. Deliberately rechecked,including defined-zero roles.
#if !defined(PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU) || PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION) || PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION != 1
#error Exact distinct BF16/FPU ROOT with precise producer parent required
#endif
#if defined(_KERNEL_MODE) || defined(__FAST_MATH__) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_BF16_WORKER_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION) || defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS) || defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS) || defined(PIONEER_Q38_TOKEN_BF16_PROGRAMS)
#error BF16/FPU ROOT is not worker,old pushed/integer/native/CPU/Work authority
#endif
#if defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE)
#if PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE != 1 || !defined(PIONEER_Q38_GLOBAL_CHAIN_LOCAL_SOURCE_GUARDS) || PIONEER_Q38_GLOBAL_CHAIN_LOCAL_SOURCE_GUARDS != 1 || defined(PIONEER_Q38_GLOBAL_CHAIN_CONV_CAPSULE_IMAGE) || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE)
#error BF16/FPU ROOT component requires its actual returning local-source parent
#endif
#elif defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE)
#if PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE != 1 || !defined(PIONEER_Q38_TOKEN_CANCEL_BOUNDARY) || PIONEER_Q38_TOKEN_CANCEL_BOUNDARY != 1
#error BF16/FPU persistent ROOT requires its cancel-boundary parent
#endif
#else
#error BF16/FPU role is only a persistent ROOT or returning component
#endif
