/* SPDX-License-Identifier: MIT */
// No guard: cancelled-first-layer cleanup TEST, NOT full model-generation END.
#include "pioneer_q38_token_native_backend_finish_role.h"
#if !defined(PIONEER_Q38_TOKEN_NATIVE_CONTROL_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_CONTROL_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_CONTROL_FINISH_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_CONTROL_FINISH_FIXTURE != 1
#error Explicit private control Finish/actual cancelled-first-layer predecessor TEST required
#endif
#if defined(PIONEER_Q38_TOKEN_NATIVE_CONTROL_BEGIN_FIXTURE)
#error Control Finish-only TEST cannot alias a Start fixture, including defined0
#endif
