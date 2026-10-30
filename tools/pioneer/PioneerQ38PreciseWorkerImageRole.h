/* SPDX-License-Identifier: MIT */
// No include guard: late and defined-zero contamination must fail.
#if !defined(PIONEER_Q38_PRECISE_WORKER_IMAGE) || PIONEER_Q38_PRECISE_WORKER_IMAGE != 1
#error Exact separate QGW2 precise worker image required
#endif
#if defined(_KERNEL_MODE) || defined(__FAST_MATH__) || defined(PIONEER_DRIVER_BUILD_TAG) \
 || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) || defined(PIONEER_Q38_MIXED_WORKER_IMAGE) \
 || defined(PIONEER_Q38_MIXED_CAPSULE_IMAGE) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error QGW2 image is separate from kernel, old worker, ROOT, CPU reference or Work
#endif
