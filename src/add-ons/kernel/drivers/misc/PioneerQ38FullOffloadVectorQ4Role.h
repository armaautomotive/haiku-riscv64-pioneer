/* SPDX-License-Identifier: MIT */
// Exact full-card compute revision;NOT resident-lifecycle/CPU/hybrid authority.
#if !defined(PIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4) || PIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4 != 1 \
 || !defined(PIONEER_Q38_FULL_OFFLOAD_BATCHED) || PIONEER_Q38_FULL_OFFLOAD_BATCHED != 1 \
 || !defined(PIONEER_Q38_FULL_OFFLOAD_METRICS) || PIONEER_Q38_FULL_OFFLOAD_METRICS != 1 \
 || defined(PIONEER_Q38_FULL_OFFLOAD_RESIDENT) || defined(PIONEER_Q38_FULL_OFFLOAD_MACRO_Q4) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_VECTOR_Q4_METADATA_IMAGE) \
 || defined(PIONEER_Q38_VECTOR_Q4_METADATA) || defined(PIONEER_Q38_FULL_OFFLOAD_PROGRAMS_H) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_BATCHED_PROGRAMS_H) || defined(__FAST_MATH__)
#error Full-card vector Q4 requires its distinct exact complete compute revision
#endif
#ifdef _KERNEL_MODE
#if _KERNEL_MODE != 1 || !defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || PIONEER_Q38_FULL_OFFLOAD_NATIVE != 1 \
 || !defined(TT_PIONEER_DRIVER_RELOAD) || TT_PIONEER_DRIVER_RELOAD != 1 \
 || !defined(PIONEER_DRIVER_BUILD_TAG) || PIONEER_DRIVER_BUILD_TAG != 0x2026110400010092ULL \
 || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT)
#error Full-card vector Q4 requires the actual tag92 full native parent
#endif
#else
#if !defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || PIONEER_Q38_FULL_OFFLOAD_CLIENT != 1 \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_DRIVER_BUILD_TAG)
#error Full-card vector Q4 requires its distinct full-offload USER client
#endif
#endif
