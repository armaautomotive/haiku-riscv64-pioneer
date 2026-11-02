/* SPDX-License-Identifier: MIT */
// Separate PRIVATE full timing ROOT + unchanged returning wide GDN.
#define PIONEER_Q38_TOKEN_PHASE_PROFILE_PROGRAMS 1
#define PIONEER_Q38_TOKEN_PHASE_PROFILE_NATIVE_CONTEXT 1
#include "pioneer_q38_wide_gdn_native_context_compile.cpp"
static_assert(PioneerQ38TokenPhaseProfileDataSelection==2 && PioneerQ38TokenPrograms::TokenPhaseProfile==1
 && PioneerQ38TokenPrograms::PhaseProfileAddress==0x75710 && PioneerQ38TokenPrograms::PhaseProfileWords==60,
 "actual complete timing ROOT in distinct private native subject");
extern "C" bool pioneer_token_phase_context_read(PioneerProbeCookie* cookie,uint32_t* output,PioneerQ38TokenPhaseProfileResult& result){
 PioneerQ38TokenCallScope scope(cookie);auto* c=scope.Context();
 return c && ReadQ38TokenNativePhaseProfile(*c,output,result);
} // PRIVATE compile witness only,not a conventional driver/Work ABI.
extern "C" status_t pioneer_token_phase_context_control(PioneerProbeCookie* cookie,void* data,size_t bytes){
 return Q38TokenPhaseProfileControl(cookie,data,bytes);
}
