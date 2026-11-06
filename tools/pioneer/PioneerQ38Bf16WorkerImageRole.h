/* SPDX-License-Identifier: MIT */
// Recheck on every inclusion,including defined-zero contamination.
#if !defined(PIONEER_Q38_BF16_WORKER_IMAGE) || PIONEER_Q38_BF16_WORKER_IMAGE != 1
#error Exact separate BF16/FPU returning worker image required
#endif
#if defined(_KERNEL_MODE) || defined(__FAST_MATH__) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) || defined(PIONEER_Q38_PRECISE_WORKER_IMAGE) || defined(PIONEER_Q38_PUSH_COMPLETION_WORKER_IMAGE) || defined(PIONEER_Q38_CACHED_FPU_MATRIX_IMAGE) || defined(PIONEER_NATIVE_COMPACT_Q4_STREAM_DIAGNOSTIC)
#error BF16 worker cannot alias installed/native/CPU/old worker roles
#endif
