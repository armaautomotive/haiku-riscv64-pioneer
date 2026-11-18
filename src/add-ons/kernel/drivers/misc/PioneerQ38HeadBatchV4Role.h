/* SPDX-License-Identifier: MIT */
// Recheck on every include. Only the separately linked raw and native-Q8
// v4 head-batch packages own this longer private journal, never old Work.
#ifdef PIONEER_Q38_HEAD_BATCH_V4_PACKAGE
#if PIONEER_Q38_HEAD_BATCH_V4_PACKAGE != 1 || !defined(PIONEER_Q38_FAST_DECODE_SPLIT_PACKAGE) \
 || PIONEER_Q38_FAST_DECODE_SPLIT_PACKAGE != 1 || !defined(PIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE) \
 || PIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE != 1 || defined(PIONEER_Q38_RECIPE_BATCH_PACKAGE) \
 || defined(PIONEER_Q38_HEAD_CSR_BATCH_ROOT_IMAGE) || defined(PIONEER_Q38_HEAD_CSR_SERIAL_CONTROL_IMAGE)
#error V4 head batch requires its exact separate package and immutable firmware
#endif
#ifdef _KERNEL_MODE
#if !defined(PIONEER_Q38_SPAN_LIVE_IMAGE_CHECK) || PIONEER_Q38_SPAN_LIVE_IMAGE_CHECK != 1 \
 || !defined(PIONEER_DRIVER_BUILD_TAG) \
 || (!defined(PIONEER_Q38_NATIVE_Q8_FULL_PACKAGE) && PIONEER_DRIVER_BUILD_TAG != 0x2026111700010218ULL) \
 || (defined(PIONEER_Q38_NATIVE_Q8_FULL_PACKAGE) && PIONEER_DRIVER_BUILD_TAG != 0x2026111700010219ULL)
#error V4 head batch requires isolated native tag218 or its matched Q8 tag219
#endif
#endif
#elif defined(_KERNEL_MODE) && defined(PIONEER_DRIVER_BUILD_TAG) \
 && (PIONEER_DRIVER_BUILD_TAG == 0x2026111700010218ULL || PIONEER_DRIVER_BUILD_TAG == 0x2026111700010219ULL)
#error V4 head batch tags require their own longer journal package
#endif
