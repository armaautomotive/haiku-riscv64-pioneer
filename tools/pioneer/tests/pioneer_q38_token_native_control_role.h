/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_token_native_backend_init_fault_role.h"
#if !defined(PIONEER_Q38_TOKEN_NATIVE_CONTROL_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_CONTROL_FIXTURE != 1
#error Explicit private user-copy/early-init failure TEST required
#endif
#if defined(PIONEER_Q38_TOKEN_NATIVE_CONTROL_BEGIN_FIXTURE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTROL_FINISH_FIXTURE)
#error Early-init control TEST cannot alias positive Start/Finish fixtures, including defined0
#endif
