/* SPDX-License-Identifier: MIT */
// Rechecked on every include. This permits a kernel-SDK TEMPLATE witness,
// never a loadable driver, native context, Work or model execution.
#ifdef PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY
#if PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY != 1 || !defined(_KERNEL_MODE) || _KERNEL_MODE != 1 \
 || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_BF16_FULL_NATIVE) || defined(PIONEER_Q38_BF16_FULL_CLIENT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Hybrid kernel template inspection forbids a driver, native context, Work or reference role
#endif
#endif
