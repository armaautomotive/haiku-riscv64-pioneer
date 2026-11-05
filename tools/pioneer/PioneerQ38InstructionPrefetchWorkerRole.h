/* SPDX-License-Identifier: MIT */
// Rechecked on every include. New IMAGE only,not a native/client capability.
#if !defined(PIONEER_Q38_INSTRUCTION_PREFETCH_WORKER_IMAGE) || PIONEER_Q38_INSTRUCTION_PREFETCH_WORKER_IMAGE != 1 \
 || !defined(PIONEER_Q38_PREPARED_EXACT_Q4_WORKER_IMAGE) || PIONEER_Q38_PREPARED_EXACT_Q4_WORKER_IMAGE != 1 \
 || !defined(PIONEER_Q38_PUSH_COMPLETION_WORKER_IMAGE) || PIONEER_Q38_PUSH_COMPLETION_WORKER_IMAGE != 1 \
 || !defined(PIONEER_Q38_PRECISE_WORKER_IMAGE) || PIONEER_Q38_PRECISE_WORKER_IMAGE != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(TT_PIONEER_DRIVER_RELOAD) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(__FAST_MATH__)
#error Instruction-prefetch IMAGE requires its separate prepared returning-worker parent,not CPU/native/Work
#endif
