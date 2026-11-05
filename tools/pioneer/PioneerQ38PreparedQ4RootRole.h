/* SPDX-License-Identifier: MIT */
// Distinct whole ROOT BUILD only. No old program/native/source authority.
#if !defined(PIONEER_Q38_PREPARED_Q4_ROOT_IMAGE) || PIONEER_Q38_PREPARED_Q4_ROOT_IMAGE != 1 \
 || !defined(PIONEER_Q38_BATCHED_PREFETCH_ROOT_IMAGE) || PIONEER_Q38_BATCHED_PREFETCH_ROOT_IMAGE != 1 \
 || !defined(PIONEER_Q38_BATCHED_OUTPUT_CLOSURE_ROOT_IMAGE) || PIONEER_Q38_BATCHED_OUTPUT_CLOSURE_ROOT_IMAGE != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION) || PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION != 1
#error Prepared Q4 requires the matching complete batched/pushed ROOT BUILD
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(__FAST_MATH__) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_PREPARED_EXACT_Q4_IMAGE) || defined(PIONEER_Q38_PREPARED_EXACT_Q4_WORKER_IMAGE)
#error Prepared ROOT BUILD cannot alias native,math worker,CPU reference or Work authority
#endif
