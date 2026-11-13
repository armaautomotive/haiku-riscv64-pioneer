/* SPDX-License-Identifier: MIT */
// Non-loadable fake-tag compilation only. This does not authorize a
// registered ioctl, native owner, Work mode, device execution or cleanup.
#ifdef PIONEER_Q38_RECURRENT_KERNEL_COMPILE_ONLY
#if PIONEER_Q38_RECURRENT_KERNEL_COMPILE_ONLY != 1 \
 || !defined(_KERNEL_MODE) || _KERNEL_MODE != 1 \
 || !defined(PIONEER_Q38_QGWD_NATIVE_INSPECTION) || PIONEER_Q38_QGWD_NATIVE_INSPECTION != 1 \
 || !defined(PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY) || PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_DRIVER_BUILD_TAG) || PIONEER_DRIVER_BUILD_TAG != 0x51544b4e4f445256ULL \
 || defined(PIONEER_Q38_QGWD_NATIVE_141) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Recurrent kernel compile requires the fake-tag non-loadable driver profile
#endif
#endif
#ifdef PIONEER_Q38_SMALL_RECURRENT_MOCK_ROUTE
#if PIONEER_Q38_SMALL_RECURRENT_MOCK_ROUTE != 1 || !defined(PIONEER_BANKS_MOCK_ONLY) \
 || !defined(PIONEER_Q38_SMALL_RECURRENT_SPAN_INSPECTION) || PIONEER_Q38_SMALL_RECURRENT_SPAN_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE)
#error Small recurrent mock route is simulated hardware only
#endif
#endif
