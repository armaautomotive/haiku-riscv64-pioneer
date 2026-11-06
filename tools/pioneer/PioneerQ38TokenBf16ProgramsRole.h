/* SPDX-License-Identifier: MIT */
#include "PioneerQ38TokenBf16DataBoundary.h"
// Immutable DATA only. Rechecked even when the generated arrays are guarded.
#if !defined(PIONEER_Q38_TOKEN_BF16_PROGRAMS) || PIONEER_Q38_TOKEN_BF16_PROGRAMS != 1 \
 || defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS) || defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS) \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU) || defined(PIONEER_Q38_BF16_WORKER_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION) \
 || defined(PIONEER_Q38_TOKEN_PROGRAMS_H) || defined(PIONEER_Q38_TOKEN_PUSHED_FOOTPRINT_H) \
 || defined(PIONEER_Q38_TOKEN_PRECISE_FOOTPRINT_H)
#error Exact separate QTP4 BF16 program DATA cannot alias old programs or BUILD roles
#endif
#ifdef _KERNEL_MODE
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38Bf16FullNativeRole.h"
#elif defined(PIONEER_Q38_BF16_FULL_NATIVE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_DRIVER_BUILD_TAG)
#error BF16 native selector requires its actual kernel subject,not a host DATA fixture
#endif
