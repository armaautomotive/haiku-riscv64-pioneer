/* SPDX-License-Identifier: MIT */
// No guard: physical and renamed NODRV selections are independent and
// mutually exclusive;neither a body include nor compilation qualifies silicon.
#ifdef PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_NATIVE
#include "PioneerQ38RecurrentPrefixValidationRole.h"
#else
#if !defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_CONTROL_COMPILE_ONLY) || PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_CONTROL_COMPILE_ONLY != 1 \
 || !defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_READBACK) || PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_READBACK != 1 \
 || !defined(PIONEER_Q38_RECURRENT_PREFIX_FINAL_RELEASE) || PIONEER_Q38_RECURRENT_PREFIX_FINAL_RELEASE != 1
#error Exact distinct prefix control COMPILE-TEST/readback/final-release prerequisites required
#endif
#if defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_NATIVE) || defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_REFERENCE) || defined(PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_REFERENCE)
#error Control compile component cannot alias physical,client or CPU-oracle roles,including defined0
#endif
#endif
#include "PioneerQ38RecurrentPrefixNativeRole.h"
#include "PioneerQ38RecurrentPrefixReadbackRole.h"
#include "PioneerQ38RecurrentPrefixFinalReleaseRole.h"
