/* SPDX-License-Identifier: MIT */
// USER-side identity selection only. No accelerator capability is granted
// until the existing session checks the actual driver's tag and generation.
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38Bf16FullProfileBinding.h"
#ifdef PIONEER_Q38_BF16_FULL_PROFILE
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38Bf16FullProfileRole.h"
#endif
#if !defined(PIONEER_Q38_BF16_FULL_CLIENT) || PIONEER_Q38_BF16_FULL_CLIENT != 1 \
 || !defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || PIONEER_Q38_FULL_OFFLOAD_CLIENT != 1 \
 || defined(_KERNEL_MODE) || defined(__FAST_MATH__) || defined(PIONEER_Q38_BF16_FULL_NATIVE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_TOKEN_BF16_PROGRAMS) || defined(PIONEER_Q38_TOKEN_BF16_PROFILE_PROGRAMS) \
 || defined(PIONEER_Q38_BF16_PROFILE_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_PROTOCOL_H) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_BATCHED) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4) || defined(PIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_MACRO_Q4) || defined(PIONEER_Q38_FULL_OFFLOAD_RESIDENT) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_COUNTER_UNROLL) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_TWO_PRODUCT_REPLAY) || defined(PIONEER_Q38_FULL_OFFLOAD_INSTRUCTION_PREFETCH) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_QUEUED_Q4) || defined(PIONEER_Q38_FULL_OFFLOAD_FIXED_CONFIG_Q4)
#error BF16 full-card client requires its separate USER tag105 identity,not old/native/reference roles
#endif
#if !defined(PIONEER_Q38_BF16_FULL_PROFILE) && (defined(PIONEER_Q38_FULL_OFFLOAD_METRICS) || defined(PIONEER_Q38_FULL_OFFLOAD_PHASE_PROFILE))
#error BF16 full-card client requires its separate timing identity for observers
#endif
