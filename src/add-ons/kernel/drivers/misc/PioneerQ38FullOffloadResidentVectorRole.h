/* SPDX-License-Identifier: MIT */
// Tag210 combines the unchanged raw vector92 images with the existing
// resident request/weight lifetime. Recheck on every inclusion.
#if !defined(PIONEER_Q38_FULL_OFFLOAD_RESIDENT_VECTOR) || PIONEER_Q38_FULL_OFFLOAD_RESIDENT_VECTOR != 1 \
 || !defined(PIONEER_Q38_FULL_OFFLOAD_RESIDENT) || PIONEER_Q38_FULL_OFFLOAD_RESIDENT != 1 \
 || !defined(PIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4) || PIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4 != 1 \
 || defined(PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4) || defined(PIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_MACRO_Q4) || defined(PIONEER_Q38_BF16_FULL_CLIENT)
#error Full-card resident-vector requires both exact raw vector and resident request roles
#endif
#ifdef _KERNEL_MODE
#if !defined(PIONEER_DRIVER_BUILD_TAG) || PIONEER_DRIVER_BUILD_TAG != 0x2026111700010210ULL
#error Full-card resident-vector requires its distinct native tag210
#endif
#endif
