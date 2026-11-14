/* SPDX-License-Identifier: MIT */
// New model-resident lifecycle,not a flag that skips an old one-shot END.
// No include guard:late role/tag removal must be detected as well.
#if !defined(PIONEER_Q38_FULL_OFFLOAD_RESIDENT) || PIONEER_Q38_FULL_OFFLOAD_RESIDENT != 1 \
 || !defined(PIONEER_Q38_FULL_OFFLOAD_BATCHED) || PIONEER_Q38_FULL_OFFLOAD_BATCHED != 1 \
 || !defined(PIONEER_Q38_FULL_OFFLOAD_METRICS) || PIONEER_Q38_FULL_OFFLOAD_METRICS != 1 \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(__FAST_MATH__)
#error Full-card resident lifecycle requires its exact separate batched+metrics role,never CPU inference or Work
#endif
#ifdef _KERNEL_MODE
#if _KERNEL_MODE != 1 || !defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || PIONEER_Q38_FULL_OFFLOAD_NATIVE != 1 \
 || !defined(PIONEER_DRIVER_BUILD_TAG) || PIONEER_DRIVER_BUILD_TAG != 0x2026110400010091ULL \
 || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT)
#error Full-card resident lifecycle requires actual native tag91
#endif
#else
#if !defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || PIONEER_Q38_FULL_OFFLOAD_CLIENT != 1 \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE)
#error Full-card resident lifecycle requires its separate USER client
#endif
#endif
