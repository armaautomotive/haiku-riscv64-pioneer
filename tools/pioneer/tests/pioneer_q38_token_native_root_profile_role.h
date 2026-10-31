/* SPDX-License-Identifier: MIT */
// HOST observation for the SAME original precise-beta first-layer/full-END
// test only. Never import a profiler into the physical Haiku driver.
#include "pioneer_q38_token_native_hidden_read_role.h"
#if !defined(PIONEER_Q38_NATIVE_ROOT_PROFILE_FIXTURE) || PIONEER_Q38_NATIVE_ROOT_PROFILE_FIXTURE != 1 \
 || !defined(_KERNEL_MODE) || _KERNEL_MODE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS) || PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_POSTED_CANCEL) || PIONEER_Q38_TOKEN_POSTED_CANCEL != 1 \
 || !defined(PIONEER_DRIVER_BUILD_TAG) || PIONEER_DRIVER_BUILD_TAG != 0x5154334e4f445256ULL \
 || defined(PIONEER_Q38_TOKEN_PRECISE_BETA_VALIDATION_NATIVE) || defined(PIONEER_Q38_TOKEN_FAULT_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE) || defined(PIONEER_Q38_NATIVE_ROOT_PROFILE_BRIDGE_TEST)
#error Explicit HOST precise-beta native first-layer/full-END observation required,not a physical or bridge role
#endif
