/* SPDX-License-Identifier: MIT */
// Private renamed native TEST/compile body only. No include guard: reject
// late changes,defined0 and physical promotion independently on every use.
#if !defined(PIONEER_Q38_RECURRENT_PREFIX_FROZEN_PLAN) || PIONEER_Q38_RECURRENT_PREFIX_FROZEN_PLAN != 1
#error Exact private frozen-prefix-data selection required
#endif
#if defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_NATIVE) || defined(PIONEER_Q38_RECURRENT_PREFIX_PLAN_MIRROR_PROTOTYPE) \
 || defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_CLIENT) || defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_REFERENCE)
#error Frozen data is not a physical driver,host mirror,client or CPU-oracle alias,including defined0
#endif
#include "PioneerQ38RecurrentPrefixNativeRole.h"
#if defined(_KERNEL_MODE) && defined(PIONEER_Q38_RECURRENT_PREFIX_FROZEN_REFUSALS_TEST)
#error Frozen refusal endpoint fixture cannot enter a kernel compile,including defined0
#endif
#if PIONEER_DRIVER_BUILD_TAG != 0x5152504e4f445256ULL
#error Frozen native data is still separately renamed QRPNODRV only
#endif
