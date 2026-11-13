/* SPDX-License-Identifier: MIT */
// Rechecked on every include: a padded 48-row alpha/beta worker inspection
// is not an installed driver, native owner, Work or CPU-reference role.
#if defined(PIONEER_Q38_RECURRENT_SMALL_BFP8_WORKER_INSPECTION) && \
 (PIONEER_Q38_RECURRENT_SMALL_BFP8_WORKER_INSPECTION != 1 \
 || !defined(PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION) || PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__))
#error Padded recurrent alpha/beta worker is offline inspection only
#endif
#if defined(PIONEER_Q38_SMALL_INPUT_REUSE_INSPECTION) && \
 (PIONEER_Q38_SMALL_INPUT_REUSE_INSPECTION != 1 || !defined(PIONEER_Q38_RECURRENT_SMALL_BFP8_WORKER_INSPECTION) \
 || PIONEER_Q38_RECURRENT_SMALL_BFP8_WORKER_INSPECTION != 1 || defined(_KERNEL_MODE) \
 || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE))
#error Small recurrent input reuse is offline inspection only
#endif
