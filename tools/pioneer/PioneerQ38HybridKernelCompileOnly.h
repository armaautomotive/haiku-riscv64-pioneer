/* SPDX-License-Identifier: MIT */
// Rechecked on every include. The first selector is a kernel-SDK template
// witness; the second permits only the renamed, non-loadable native test
// context. Neither selector grants a physical driver, Work or execution.
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
#ifdef PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY
#if PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY != 1 || !defined(_KERNEL_MODE) || _KERNEL_MODE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1 \
 || !defined(PIONEER_Q38_HYBRID_TRACKED_UPLOAD) || PIONEER_Q38_HYBRID_TRACKED_UPLOAD != 1 \
 || !defined(PIONEER_Q38_TOKEN_BF16_PROGRAMS) || PIONEER_Q38_TOKEN_BF16_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC) || PIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC != 1 \
 || !defined(PIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC) || PIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC != 1 \
 || !defined(PIONEER_DRIVER_BUILD_TAG) || PIONEER_DRIVER_BUILD_TAG != 0x51544b4e4f445256ULL \
 || defined(PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY) || defined(PIONEER_Q38_HYBRID_TEST_BFP8_LAYER0) \
 || defined(PIONEER_Q38_HYBRID_INPUT_REUSE_INSPECTION) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || defined(PIONEER_Q38_BF16_FULL_NATIVE) \
 || defined(PIONEER_Q38_BF16_FULL_CLIENT) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Hybrid native inspection requires the exact private tracked QGW5 context; never a loadable driver
#endif
#endif
