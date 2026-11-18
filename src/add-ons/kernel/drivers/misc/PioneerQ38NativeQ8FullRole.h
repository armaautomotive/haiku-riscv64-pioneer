/* SPDX-License-Identifier: MIT */
// All transport and program consumers must target the same isolated tag.
// Rechecked outside a guard: the legacy Work layout is not this package.
#if defined(PIONEER_Q38_NATIVE_Q8_FULL_PACKAGE)
#if PIONEER_Q38_NATIVE_Q8_FULL_PACKAGE != 1 || !defined(PIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE) \
 || PIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE != 1 || !defined(PIONEER_Q38_FAST_DECODE_SPLIT_PACKAGE) \
 || PIONEER_Q38_FAST_DECODE_SPLIT_PACKAGE != 1 || !defined(PIONEER_Q38_FULL_OFFLOAD_BATCHED) \
 || PIONEER_Q38_FULL_OFFLOAD_BATCHED != 1 || !defined(PIONEER_Q38_FULL_OFFLOAD_METRICS) \
 || PIONEER_Q38_FULL_OFFLOAD_METRICS != 1 || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4) || defined(PIONEER_Q38_RECIPE_BATCH_PACKAGE)
#error Native Q8 full-card package requires its separate raw-v4 parent without hybrid or CPU-reference roles
#endif
#ifdef _KERNEL_MODE
#if _KERNEL_MODE != 1 || !defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || PIONEER_Q38_FULL_OFFLOAD_NATIVE != 1 \
 || !defined(PIONEER_Q38_TWIN_NATIVE_Q8_WIRE) || PIONEER_Q38_TWIN_NATIVE_Q8_WIRE != 1 \
 || !defined(PIONEER_Q38_SPAN_LIVE_IMAGE_CHECK) || PIONEER_Q38_SPAN_LIVE_IMAGE_CHECK != 1 \
 || !defined(PIONEER_DRIVER_BUILD_TAG) \
 || (!defined(PIONEER_Q38_HEAD_BATCH_V4_PACKAGE) && PIONEER_DRIVER_BUILD_TAG != 0x2026111700010217ULL) \
 || (defined(PIONEER_Q38_HEAD_BATCH_V4_PACKAGE) && PIONEER_DRIVER_BUILD_TAG != 0x2026111700010219ULL) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT)
#error Native Q8 full-card package requires matched native code and live span audit
#endif
#elif !defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || PIONEER_Q38_FULL_OFFLOAD_CLIENT != 1 \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_TWIN_NATIVE_Q8_WIRE) \
 || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_SPAN_LIVE_IMAGE_CHECK)
#error Native Q8 full-card package requires a separate authenticated USER producer
#endif
#elif defined(_KERNEL_MODE) && (defined(PIONEER_Q38_TWIN_NATIVE_Q8_WIRE) \
 || (defined(PIONEER_DRIVER_BUILD_TAG) && PIONEER_DRIVER_BUILD_TAG == 0x2026111700010217ULL))
#error Native Q8 card programs cannot be selected by an unmatched driver
#endif
