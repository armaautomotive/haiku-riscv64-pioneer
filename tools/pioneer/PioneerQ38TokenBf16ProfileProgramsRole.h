/* SPDX-License-Identifier: MIT */
#include "PioneerQ38TokenBf16DataBoundary.h"
// Immutable DATA. A host fixture is not a native owner or execution grant.
#if !defined(PIONEER_Q38_TOKEN_BF16_PROGRAMS) || PIONEER_Q38_TOKEN_BF16_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_BF16_PROFILE_PROGRAMS) || PIONEER_Q38_TOKEN_BF16_PROFILE_PROGRAMS != 1 \
 || defined(PIONEER_Q38_TOKEN_BF16_PROGRAMS_H) || defined(PIONEER_Q38_TOKEN_PROGRAMS_H) \
 || defined(PIONEER_Q38_TOKEN_BF16_PAIR_ISSUE_PROGRAMS) || defined(PIONEER_Q38_TOKEN_BF16_PAIR_ISSUE_PROGRAMS_H) \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU) || defined(PIONEER_Q38_BF16_PROFILE_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || defined(PIONEER_Q38_BF16_WORKER_IMAGE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(__FAST_MATH__)
#error Exact BF16 timing program DATA cannot alias old arrays,BUILD,CPU or Work
#endif
#ifdef _KERNEL_MODE
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38Bf16FullNativeRole.h"
#elif defined(PIONEER_Q38_BF16_FULL_NATIVE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_DRIVER_BUILD_TAG)
#error BF16 timing native selector requires its actual kernel subject
#endif
