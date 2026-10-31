/* SPDX-License-Identifier: MIT */
// New private observer only; no public ioctl,conventional ABI,link or install.
#define PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS 1
#define PIONEER_Q38_TOKEN_FRONTIER_NATIVE_CONTEXT 1
#include "pioneer_q38_token_job_boot_fold_native_context_compile.cpp"
static_assert(PioneerQ38TokenFrontierProgramSelection==2
 && PioneerQ38TokenPrograms::TokenFrontier==1,"actual new publisher/private reader binding");
extern "C" bool pioneer_token_frontier_context_scoped_observe(PioneerProbeCookie* cookie,PioneerQ38TokenFrontierResult& result){
 PioneerQ38TokenCallScope scope(cookie);auto* c=scope.Context();
 return c && ObserveQ38TokenNativeFrontier(*c,result);
} // actual same-owner call scope; no caller generation,ready or cleanup receipt
extern "C" status_t pioneer_token_frontier_context_control(PioneerProbeCookie* cookie,void* data,size_t bytes){
 return Q38TokenFrontierControl(cookie,data,bytes);
} // private copy/lock adapter witness ONLY;not routed by ProbeControl
