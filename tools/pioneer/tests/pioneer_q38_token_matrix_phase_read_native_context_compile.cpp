/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_TOKEN_MATRIX_PHASE_READ_NATIVE 1
#include "pioneer_q38_token_matrix_phase_native_context_compile.cpp"
static_assert(PioneerQ38TokenMatrixPhaseReadSelection==1,"distinct PRIVATE held sampler reader");
extern "C" bool pioneer_token_context_scoped_matrix_phase(PioneerProbeCookie* cookie,unsigned worker,
 uint32_t* output,PioneerQ38TokenMatrixPhaseResult* result){
 if(!result)return false;
 PioneerQ38TokenCallScope scope(cookie);auto* c=scope.Context();
 return c && ReadQ38TokenNativeMatrixPhase(*c,worker,output,*result);
} // Inert NODRV context only;no exported driver/public packet/USER or Work selector.
