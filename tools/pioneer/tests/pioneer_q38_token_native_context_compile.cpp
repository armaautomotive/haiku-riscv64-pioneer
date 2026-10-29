/* SPDX-License-Identifier: MIT */
// Actual full-model context and IO. Compile/inspect ONLY: no conventional
// driver ABI, public operation selector, link, install, or device invocation.
#define PIONEER_Q38_TOKEN_NATIVE_CONTEXT 1
#define PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY 1
#define PIONEER_Q38_TOKEN_EMIT_CONTEXT_BODY 1
#define PIONEER_Q38_DECODER_RUNTIME 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
extern "C" {
 extern int32 pioneer_q38_token_context_body_api_version;
 status_t pioneer_q38_token_context_body_init_hardware();
 status_t pioneer_q38_token_context_body_init_driver();
 void pioneer_q38_token_context_body_uninit_driver();
 const char** pioneer_q38_token_context_body_publish_devices();
 device_hooks* pioneer_q38_token_context_body_find_device(const char*);
}
#define api_version pioneer_q38_token_context_body_api_version
#define init_hardware pioneer_q38_token_context_body_init_hardware
#define init_driver pioneer_q38_token_context_body_init_driver
#define uninit_driver pioneer_q38_token_context_body_uninit_driver
#define publish_devices pioneer_q38_token_context_body_publish_devices
#define find_device pioneer_q38_token_context_body_find_device
#include "../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp"

extern "C" bool pioneer_token_context_scoped_bind(PioneerProbeCookie* cookie,unsigned bank){
 PioneerQ38TokenCallScope scope(cookie);auto* c=scope.Context();
 return c && BindQ38TokenNative(*c,bank);
} // genuine private FD/team/thread/global-lock scope, not a caller-ready seal
extern "C" bool pioneer_token_context_scoped_binding(PioneerProbeCookie* cookie){
 PioneerQ38TokenCallScope scope(cookie);auto* c=scope.Context();
 return c && c->io.TokenModelSourceScope(false);
}
extern "C" bool pioneer_token_context_scoped_initialize(PioneerProbeCookie* cookie,
 PioneerQ38StateZeroResult& recurrent,PioneerQ38ConvHistoryZeroResult& history,PioneerQ38ProjectionZeroResult& projection){
 PioneerQ38TokenCallScope scope(cookie);auto* c=scope.Context();
 return c && InitializeQ38TokenRecurrent(*c,recurrent) && InitializeQ38TokenHistory(*c,history)
  && InitializeQ38TokenProjection(*c,projection) && c->q38TokenNative->InitializationsClosed(c->banks,c->baseline.baseline.baseline.baseline.dram);
} // full actual controller bodies,never linked,installed or invoked by this compile test
extern "C" bool pioneer_token_context_scoped_dispatch_end(PioneerProbeCookie* cookie,
 tt_resident_session_result& result,bool& clean){
 PioneerQ38TokenCallScope scope(cookie);auto* c=scope.Context();
 return c && EndQ38DispatchOwnership(*c,result,clean);
} // unfinished full-token END must retain, never take legacy cleanup
extern "C" void pioneer_token_context_retain(PioneerStoreContext& c,tt_resident_session_result& result){
 RetainQ38DispatchOwner(c,result);
}
extern "C" bool pioneer_token_context_scoped_held(PioneerProbeCookie* cookie,const uint32_t* prompt,unsigned prompts,
 unsigned limit,bool stopEos,unsigned eos0,unsigned eos1,PioneerQ38TokenMemoryResult& result){
 PioneerQ38TokenCallScope scope(cookie);auto* c=scope.Context();
 return c && PrepareQ38TokenNativeHeld(*c,prompt,prompts,limit,stopEos,eos0,eos1,result) && CloseQ38TokenNativeHeld(*c,result);
} // actual whole held-only factory/controllers;no public route/link/install/invocation
extern "C" bool pioneer_token_context_scoped_runtime(PioneerProbeCookie* cookie,PioneerQ38TokenRuntimeResult& result){
 PioneerQ38TokenCallScope scope(cookie);auto* c=scope.Context();
 return c && PrepareQ38TokenNativeRuntime(*c,result);
} // startup metadata/empty-prefix DATA ONLY;NO native bootstrap/RUN/STOP/END grant
extern "C" bool pioneer_token_context_scoped_bootstrap(PioneerProbeCookie* cookie,PioneerQ38TokenBootstrapResult& result){
 PioneerQ38TokenCallScope scope(cookie);auto* c=scope.Context();
 return c && BootstrapQ38TokenNativeWorkers(*c,result);
} // private actual fresh-worker controller,not a public driver or ROOT/model RUN route
extern "C" bool pioneer_token_context_scoped_root_start(PioneerProbeCookie* cookie,PioneerQ38TokenExecutionResult& result){
 PioneerQ38TokenCallScope scope(cookie);auto* c=scope.Context();return c && StartQ38TokenNativeRoot(*c,result);
}
extern "C" bool pioneer_token_context_scoped_root_cancel(PioneerProbeCookie* cookie,PioneerQ38TokenExecutionResult& result){
 PioneerQ38TokenCallScope scope(cookie);auto* c=scope.Context();return c && CancelQ38TokenNativeRoot(*c,result);
} // private actual native ROOT lifecycle candidates,NO public ABI/Work/clean END
extern "C" bool pioneer_token_context_scoped_root_observe(PioneerProbeCookie* cookie,PioneerQ38TokenExecutionResult& result){
 PioneerQ38TokenCallScope scope(cookie);auto* c=scope.Context();return c && ObserveQ38TokenNativeRoot(*c,result);
} // private observation candidate,not a model event or completion capability
extern "C" bool pioneer_token_context_scoped_terminal_observe(PioneerProbeCookie* cookie,PioneerQ38TokenTerminalResult& result){
 PioneerQ38TokenCallScope scope(cookie);auto* c=scope.Context();return c && ObserveQ38TokenNativeTerminal(*c,result);
} // private read-only STOP observation;no public ABI/HOLD/restore/END/Work route
extern "C" void pioneer_token_context_destroy_clean(PioneerStoreContext* c){delete c;}

// Independent inert controls, separate from the renamed actual driver bodies.
extern "C" status_t pioneer_q38_token_context_only_init_hardware(){return B_NOT_ALLOWED;}
extern "C" status_t pioneer_q38_token_context_only_init_driver(){return B_NOT_ALLOWED;}
extern "C" void pioneer_q38_token_context_only_uninit_driver(){}
extern "C" const char** pioneer_q38_token_context_only_publish_devices(){return NULL;}
extern "C" device_hooks* pioneer_q38_token_context_only_find_device(const char*){return NULL;}
extern "C" status_t pioneer_token_context_expected_refusal(){return B_NOT_ALLOWED;}
extern "C" void* pioneer_token_context_expected_null(){return NULL;}
extern "C" void pioneer_token_context_expected_inert(){}
