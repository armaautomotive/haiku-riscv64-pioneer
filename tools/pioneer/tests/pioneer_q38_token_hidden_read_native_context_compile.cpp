/* SPDX-License-Identifier: MIT */
// Actual private QT3 context/controller. COMPILE ONLY;no link/install/invoke.
#define PIONEER_Q38_TOKEN_HELD_HIDDEN_DIAGNOSTIC 1
#include "pioneer_q38_token_pushed_native_context_compile.cpp"
static_assert(PioneerQ38TokenHiddenReadSelection==1,"private diagnostic only");
extern "C" bool pioneer_token_context_scoped_hidden_read(PioneerProbeCookie* cookie,unsigned part,
 uint32_t* output,unsigned words,PioneerQ38TokenHiddenReadResult& result){
 PioneerQ38TokenCallScope scope(cookie);auto* c=scope.Context();
 return c && ReadQ38TokenNativeHiddenChunk(*c,part,output,words,result);
} // real FD/team/thread/global-lock scope,never a caller-ready or public opcode
