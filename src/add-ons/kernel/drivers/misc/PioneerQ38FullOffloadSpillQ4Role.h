/* SPDX-License-Identifier: MIT */
// New tag100 pairs the exact macro-Q4 workers with the five-span ROOT image.
#if !defined(PIONEER_Q38_FULL_OFFLOAD_SPILL_Q4) || PIONEER_Q38_FULL_OFFLOAD_SPILL_Q4 != 1 \
 || !defined(PIONEER_Q38_FULL_OFFLOAD_MACRO_Q4) || PIONEER_Q38_FULL_OFFLOAD_MACRO_Q4 != 1 \
 || !defined(PIONEER_Q38_FULL_OFFLOAD_BATCHED) || PIONEER_Q38_FULL_OFFLOAD_BATCHED != 1 \
 || !defined(PIONEER_Q38_FULL_OFFLOAD_METRICS) || PIONEER_Q38_FULL_OFFLOAD_METRICS != 1 \
 || defined(PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4) || defined(PIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(__FAST_MATH__)
#error Full-card spill Q4 requires the isolated tag100 macro-Q4 Work role
#endif
#ifdef _KERNEL_MODE
#if _KERNEL_MODE != 1 || !defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || PIONEER_Q38_FULL_OFFLOAD_NATIVE != 1 \
 || !defined(TT_PIONEER_DRIVER_RELOAD) || TT_PIONEER_DRIVER_RELOAD != 1 \
 || !defined(PIONEER_DRIVER_BUILD_TAG) || PIONEER_DRIVER_BUILD_TAG != 0x2026111500010100ULL \
 || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT)
#error Full-card spill Q4 requires its distinct tag100 native owner
#endif
#else
#if !defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || PIONEER_Q38_FULL_OFFLOAD_CLIENT != 1 \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_DRIVER_BUILD_TAG)
#error Full-card spill Q4 requires the distinct full-offload USER client
#endif
#endif
