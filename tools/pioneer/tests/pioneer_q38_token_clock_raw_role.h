/* SPDX-License-Identifier: MIT */
// No include guard:transport substitution is TEST ONLY on every include.
#if !defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1 \
 || defined(PIONEER_Q38_WORK_INFERENCE)
#error Explicit native clock transport TEST role required
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_END_FLOW_FIXTURE
#include "pioneer_q38_token_native_full_end_role.h"
#elif defined(PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE)
#error Explicit native clock transport TEST cannot alias ordinary full-flow,including defined0
#endif
