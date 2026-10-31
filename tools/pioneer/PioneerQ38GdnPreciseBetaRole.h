/* SPDX-License-Identifier: MIT */
// No guard: late and defined-zero role changes must also fail.
#if !defined(PIONEER_Q38_GDN_PRECISE_BETA_IMAGE) || PIONEER_Q38_GDN_PRECISE_BETA_IMAGE != 1 \
 || !defined(PIONEER_Q38_SIGMOID_PRECISION_IMAGE) || PIONEER_Q38_SIGMOID_PRECISION_IMAGE != 1 \
 || !defined(PIONEER_Q38_GDN_PREPARE_IMAGE) || PIONEER_Q38_GDN_PREPARE_IMAGE != 1 \
 || !defined(PIONEER_Q38_GDN_INPUT_IMAGE) || PIONEER_Q38_GDN_INPUT_IMAGE != 1 \
 || !defined(PIONEER_Q38_GDN_INPUT_COMBINED_STAGE) || PIONEER_Q38_GDN_INPUT_COMBINED_STAGE != 1
#error Exact separate combined-input precise beta IMAGE required
#endif
#if defined(_KERNEL_MODE) || defined(__FAST_MATH__) || defined(PIONEER_DRIVER_BUILD_TAG) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) \
 || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) \
 || defined(PIONEER_Q38_SILU_PRECISION_IMAGE) || defined(PIONEER_Q38_SILU_LEGACY_CONTROL_IMAGE)
#error Precise beta is isolated card input math, not CPU, native, ROOT or Work
#endif
