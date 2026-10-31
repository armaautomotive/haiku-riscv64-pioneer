/* SPDX-License-Identifier: MIT */
// Recheck every include; a public DATA header cannot select this driver.
#if !defined(PIONEER_Q38_TOKEN_DECODE_BENCHMARK_NATIVE) || PIONEER_Q38_TOKEN_DECODE_BENCHMARK_NATIVE != 1 \
 || !defined(_KERNEL_MODE) || _KERNEL_MODE != 1 \
 || !defined(PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_VALIDATION_NATIVE) || PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_VALIDATION_NATIVE != 1 \
 || defined(PIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT)
#error Exact separate physical decode benchmark revision required
#endif
#include "PioneerQ38TokenJobBootFoldValidationRole.h"
