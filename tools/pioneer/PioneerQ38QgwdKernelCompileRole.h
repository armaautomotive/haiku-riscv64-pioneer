/* SPDX-License-Identifier: MIT */
// Rechecked on every include: this role may compile QGWD geometry/journals
// for Haiku's kernel ABI, but cannot build a loadable/native/Work driver.
#ifdef PIONEER_Q38_QGWD_KERNEL_COMPILE_ONLY
#if PIONEER_Q38_QGWD_KERNEL_COMPILE_ONLY != 1 || !defined(_KERNEL_MODE) || _KERNEL_MODE != 1 \
 || !defined(PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY) || PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY != 1 \
 || defined(PIONEER_DRIVER_BUILD_TAG) || defined(TT_PIONEER_DRIVER_RELOAD) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY) || defined(PIONEER_Q38_HYBRID_FULL_NATIVE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error QGWD kernel compilation requires its exclusive non-loadable profile
#endif
#endif
