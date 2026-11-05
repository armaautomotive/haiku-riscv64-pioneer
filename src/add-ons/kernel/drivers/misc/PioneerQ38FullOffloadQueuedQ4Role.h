/* SPDX-License-Identifier: MIT */
// Rechecked on EVERY include;distinct complete-model queued-Q4 subject.
#include "PioneerQ38FullOffloadInstructionPrefetchRole.h"
#if !defined(PIONEER_Q38_FULL_OFFLOAD_QUEUED_Q4) || PIONEER_Q38_FULL_OFFLOAD_QUEUED_Q4 != 1 \
 || defined(PIONEER_Q38_FULL_OFFLOAD_INSTRUCTION_PREFETCH_Q4_PROGRAMS_H) \
 || defined(PIONEER_Q38_QUEUED_Q4_BLOCK_IMAGE) \
 || defined(PIONEER_Q38_QUEUED_Q4_BLOCK_WORKER_TEST) \
 || defined(PIONEER_Q38_QUEUED_Q4_FIXED_CONFIG_IMAGE) \
 || defined(PIONEER_Q38_QUEUED_Q4_DELTA_CONFIG_IMAGE) || defined(PIONEER_Q38_QUEUED_Q4_PAIR_CONFIG_IMAGE) \
 || defined(PIONEER_Q38_INSTRUCTION_PREFETCH_WORKER_IMAGE)
#error Full-card queued Q4 requires its distinct complete-model role,not old firmware or an offboard worker test
#endif
#ifdef _KERNEL_MODE
#if PIONEER_DRIVER_BUILD_TAG != (defined(PIONEER_Q38_FULL_OFFLOAD_FIXED_CONFIG_Q4)?0x2026110500010103ULL:0x2026110500010102ULL)
#error Full-card queued Q4 requires exact tag102 native parent
#endif
#endif
