/* SPDX-License-Identifier: MIT */
// Rechecked outside header guards. This is a NEW offboard worker subject.
#if !defined(PIONEER_Q38_QUEUED_Q4_BLOCK_IMAGE) || PIONEER_Q38_QUEUED_Q4_BLOCK_IMAGE != 1 \
 || !defined(PIONEER_Q38_PREPARED_EXACT_Q4_IMAGE) || PIONEER_Q38_PREPARED_EXACT_Q4_IMAGE != 1 \
 || !defined(PIONEER_Q38_PREPARED_TWO_PRODUCT_IMAGE) || PIONEER_Q38_PREPARED_TWO_PRODUCT_IMAGE != 1 \
 || !defined(PIONEER_Q38_PREPARED_TWO_PRODUCT_REPLAY_IMAGE) || PIONEER_Q38_PREPARED_TWO_PRODUCT_REPLAY_IMAGE != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(TT_PIONEER_DRIVER_RELOAD) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(__FAST_MATH__)
#error Queued Q4 block IMAGE requires exact prepared replay13 offboard worker parents,not native/CPU/Work
#endif
