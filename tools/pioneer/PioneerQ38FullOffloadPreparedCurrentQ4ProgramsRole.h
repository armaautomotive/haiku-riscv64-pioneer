/* SPDX-License-Identifier: MIT */
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadRole.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadPreparedQ4Role.h"
#if !defined(PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_CURRENT_WORKER) \
 || PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_CURRENT_WORKER != 1 \
 || !defined(PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4) || PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4 != 1 \
 || !defined(PIONEER_Q38_FULL_OFFLOAD_BATCHED) || PIONEER_Q38_FULL_OFFLOAD_BATCHED != 1 \
 || !defined(PIONEER_Q38_FULL_OFFLOAD_METRICS) || PIONEER_Q38_FULL_OFFLOAD_METRICS != 1 \
 || defined(PIONEER_Q38_FULL_OFFLOAD_COUNTER_UNROLL) || defined(PIONEER_Q38_FULL_OFFLOAD_PHASE_PROFILE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_QUEUED_Q4) || defined(PIONEER_Q38_FULL_OFFLOAD_FIXED_CONFIG_Q4) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_SPILL_Q4) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE)
#error Current prepared-Q4 worker package requires its distinct tag102 full-card role
#endif
#if defined(_KERNEL_MODE)
#if _KERNEL_MODE != 1 || !defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || PIONEER_Q38_FULL_OFFLOAD_NATIVE != 1 \
 || !defined(TT_PIONEER_DRIVER_RELOAD) || TT_PIONEER_DRIVER_RELOAD != 1 \
 || !defined(PIONEER_DRIVER_BUILD_TAG) || PIONEER_DRIVER_BUILD_TAG != 0x2026111500010102ULL
#error Current prepared-Q4 worker native package requires exact tag102
#endif
#else
#if !defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || PIONEER_Q38_FULL_OFFLOAD_CLIENT != 1 \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_DRIVER_BUILD_TAG)
#error Current prepared-Q4 worker package requires its separate USER client
#endif
#endif
