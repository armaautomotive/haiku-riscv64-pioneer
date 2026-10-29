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
extern "C" bool pioneer_token_context_scoped_dispatch_end(PioneerProbeCookie* cookie,
 tt_resident_session_result& result,bool& clean){
 PioneerQ38TokenCallScope scope(cookie);auto* c=scope.Context();
 return c && EndQ38DispatchOwnership(*c,result,clean);
} // unfinished full-token END must retain, never take legacy cleanup
extern "C" void pioneer_token_context_retain(PioneerStoreContext& c,tt_resident_session_result& result){
 RetainQ38DispatchOwner(c,result);
}
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
