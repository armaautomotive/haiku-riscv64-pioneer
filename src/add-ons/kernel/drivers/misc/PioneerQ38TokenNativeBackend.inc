/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
#include "PioneerQ38TokenNativeRole.h"
// Trusted kernel orchestration ONLY. No public/user-pointer ABI,normal
// driver entry,Work route or permission derived from a returned result.
struct PioneerQ38TokenBackendResult {
 uint32_t version=1,started=0,stage=0,ready=0,unsafe=0,complete=0,reserved[2]={},detail[8]={};
};
static_assert(sizeof(PioneerQ38TokenBackendResult)==64,"private observation,not an ownership capability");
template<class Result> static void Q38TokenBackendDetail(PioneerQ38TokenBackendResult& out,const Result& result){
 static_assert(sizeof(Result)>=32,"complete diagnostic prefix");
 const auto* raw=reinterpret_cast<const uint8_t*>(&result);
 for(unsigned i=0;i<8;i++)out.detail[i]=uint32_t(raw[i*4])|uint32_t(raw[i*4+1])<<8
  |uint32_t(raw[i*4+2])<<16|uint32_t(raw[i*4+3])<<24;
} // immutable diagnostics only;never accepted as a phase/ready/cleanup input
static PioneerQ38TokenBackendResult Q38TokenBackendFailure(PioneerQ38TokenBackendResult out,PioneerStoreContext& c){
 out.ready=out.complete=0;out.unsafe=c.Unsafe(false,false,true);return out;
} // MEMORY ONLY;no automatic cancellation,rollback,END,retry or owner deletion

[[maybe_unused]] static __attribute__((noinline)) PioneerQ38TokenBackendResult
BeginQ38TokenNativeBackendScoped(PioneerQ38TokenCallScope& scope,unsigned bank,const uint32_t* prompt,unsigned prompts,
 unsigned limit,bool stopEos,unsigned eos0,unsigned eos1){
 PioneerQ38TokenBackendResult out;
 if(bank>=7 || !prompt || !prompts || prompts>limit || !limit || limit>8192 || eos0>=248320 || eos1>=248320)return out;
 const uintptr_t input=reinterpret_cast<uintptr_t>(prompt);
 if((input&3) || input>UINTPTR_MAX-size_t(prompts)*4)return out;
 // Caller supplies a valid trusted kernel buffer,NOT an unchecked user
 // pointer. The held factory independently copies and rejects owner aliases.
 for(unsigned i=0;i<prompts;i++)if(prompt[i]>=248320)return out;
 auto* c=scope.Context();
 if(!c || c->q38TokenNative)return out;
 out.started=1;out.stage=1;
 if(!BindQ38TokenNative(*c,bank))return Q38TokenBackendFailure(out,*c);
 {PioneerQ38StateZeroResult result;out.stage=2;
  const bool okay=InitializeQ38TokenRecurrent(*c,result);Q38TokenBackendDetail(out,result);
  if(!okay)return Q38TokenBackendFailure(out,*c);}
 {PioneerQ38ConvHistoryZeroResult result;out.stage=3;
  const bool okay=InitializeQ38TokenHistory(*c,result);Q38TokenBackendDetail(out,result);
  if(!okay)return Q38TokenBackendFailure(out,*c);}
 {PioneerQ38ProjectionZeroResult result;out.stage=4;
  const bool okay=InitializeQ38TokenProjection(*c,result);Q38TokenBackendDetail(out,result);
  if(!okay)return Q38TokenBackendFailure(out,*c);}
 {PioneerQ38TokenMemoryResult result;out.stage=5;
  const bool okay=PrepareQ38TokenNativeHeld(*c,prompt,prompts,limit,stopEos,eos0,eos1,result);Q38TokenBackendDetail(out,result);
  if(!okay)return Q38TokenBackendFailure(out,*c);}
 {PioneerQ38TokenRuntimeResult result;out.stage=6;
  const bool okay=PrepareQ38TokenNativeRuntime(*c,result);Q38TokenBackendDetail(out,result);
  if(!okay)return Q38TokenBackendFailure(out,*c);}
 {PioneerQ38TokenBootstrapResult result;out.stage=7;
  const bool okay=BootstrapQ38TokenNativeWorkers(*c,result);Q38TokenBackendDetail(out,result);
  if(!okay)return Q38TokenBackendFailure(out,*c);}
 {PioneerQ38TokenExecutionResult result;out.stage=8;
  const bool okay=StartQ38TokenNativeRoot(*c,result);Q38TokenBackendDetail(out,result);
  if(!okay || !result.acknowledged)return Q38TokenBackendFailure(out,*c);}
 out.ready=1;return out;
} // READY means actual ROOT startup acknowledged,NOT a model output or END.

[[maybe_unused]] static __attribute__((noinline)) PioneerQ38TokenBackendResult
BeginQ38TokenNativeBackend(PioneerProbeCookie* cookie,unsigned bank,const uint32_t* prompt,unsigned prompts,
 unsigned limit,bool stopEos,unsigned eos0,unsigned eos1){
 PioneerQ38TokenBackendResult out;
 if(bank>=7 || !prompt || !prompts || prompts>limit || !limit || limit>8192 || eos0>=248320 || eos1>=248320)return out;
 const uintptr_t input=reinterpret_cast<uintptr_t>(prompt);
 if((input&3) || input>UINTPTR_MAX-size_t(prompts)*4)return out;
 for(unsigned i=0;i<prompts;i++)if(prompt[i]>=248320)return out;
 PioneerQ38TokenCallScope scope(cookie);
 return BeginQ38TokenNativeBackendScoped(scope,bank,prompt,prompts,limit,stopEos,eos0,eos1);
} // preserve early argument refusal before acquiring the actual FD call scope

[[maybe_unused]] static __attribute__((noinline)) PioneerQ38TokenBackendResult
FinishQ38TokenNativeBackendScoped(PioneerQ38TokenCallScope& scope){
 PioneerQ38TokenBackendResult out;auto* c=scope.Context();
 if(!c || !c->q38TokenNative || !c->q38TokenNative->TerminalHeld())return out;
 // The caller must observe genuine terminal,hold ALL participants and
 // drain every committed record first. Restore refuses a missing drain.
 out.started=1;
#define PIONEER_Q38_BACKEND_CLOSE_STAGE(number,Type,operation) \
 {Type result;out.stage=number;const bool okay=operation(*c,result);Q38TokenBackendDetail(out,result); \
  if(!okay)return Q38TokenBackendFailure(out,*c);}
 PIONEER_Q38_BACKEND_CLOSE_STAGE(1,PioneerQ38TokenRestoreResult,RestoreQ38TokenNativeL1)
 PIONEER_Q38_BACKEND_CLOSE_STAGE(2,PioneerQ38TokenDispatchRestoreResult,RestoreQ38TokenNativeDispatchImages)
 PIONEER_Q38_BACKEND_CLOSE_STAGE(3,PioneerQ38TokenPeerRestoreResult,RestoreQ38TokenNativePeerStores)
 PIONEER_Q38_BACKEND_CLOSE_STAGE(4,PioneerQ38TokenRootStoreRestoreResult,RestoreQ38TokenNativeRootStore)
 PIONEER_Q38_BACKEND_CLOSE_STAGE(5,PioneerQ38TokenSharedTlbRestoreResult,RestoreQ38TokenNativeSharedMappings)
 PIONEER_Q38_BACKEND_CLOSE_STAGE(6,PioneerQ38TokenClockResult,EndQ38TokenNativeClock)
 PIONEER_Q38_BACKEND_CLOSE_STAGE(7,PioneerQ38TokenHardwareSealResult,SealQ38TokenNativeHardware)
 PIONEER_Q38_BACKEND_CLOSE_STAGE(8,PioneerQ38TokenViewsResult,CloseQ38TokenNativeViews)
 PIONEER_Q38_BACKEND_CLOSE_STAGE(9,PioneerQ38TokenPciResult,CloseQ38TokenNativePci)
 PIONEER_Q38_BACKEND_CLOSE_STAGE(10,PioneerQ38TokenHostsResult,CloseQ38TokenNativeHosts)
#undef PIONEER_Q38_BACKEND_CLOSE_STAGE
 PioneerQ38TokenEndResult result;out.stage=11;
 const bool okay=scope.Finish(result);Q38TokenBackendDetail(out,result);
 // Finish may have destroyed the whole context. NEVER inspect c afterward.
 out.unsafe=result.unsafe;out.complete=okay && result.ready && result.context && result.reservation && result.hosts==3;
 out.ready=out.complete;return out;
} // exact explicit cleanup only;NOT a prelaunch abort or resident-model reuse path

[[maybe_unused]] static __attribute__((noinline)) PioneerQ38TokenBackendResult
FinishQ38TokenNativeBackend(PioneerProbeCookie* cookie){
 PioneerQ38TokenCallScope scope(cookie);return FinishQ38TokenNativeBackendScoped(scope);
}
#endif
