/* SPDX-License-Identifier: MIT */
// Actual production native context/IO/factories;COMPILE/INSPECT ONLY.
// ALL real conventional entries renamed;separate inert controls too.
#define PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT 1
#define PIONEER_Q38_RECURRENT_BLOCK_CONTEXT_COMPILE_ONLY 1
#define PIONEER_Q38_RECURRENT_BLOCK_EMIT_CONTEXT_BODY 1
#define PIONEER_Q38_EMBEDDING_NATIVE 1
#define PIONEER_Q38_DECODER_RUNTIME 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
extern "C" {
 extern int32 pioneer_q38_recurrent_context_body_api_version;
 status_t pioneer_q38_recurrent_context_body_init_hardware();status_t pioneer_q38_recurrent_context_body_init_driver();
 void pioneer_q38_recurrent_context_body_uninit_driver();const char** pioneer_q38_recurrent_context_body_publish_devices();
 device_hooks* pioneer_q38_recurrent_context_body_find_device(const char*);
}
#define api_version pioneer_q38_recurrent_context_body_api_version
#define init_hardware pioneer_q38_recurrent_context_body_init_hardware
#define init_driver pioneer_q38_recurrent_context_body_init_driver
#define uninit_driver pioneer_q38_recurrent_context_body_uninit_driver
#define publish_devices pioneer_q38_recurrent_context_body_publish_devices
#define find_device pioneer_q38_recurrent_context_body_find_device
#include "../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp"
extern "C" status_t pioneer_q38_recurrent_context_only_init_hardware(){return B_NOT_ALLOWED;}
extern "C" status_t pioneer_q38_recurrent_context_only_init_driver(){return B_NOT_ALLOWED;}
extern "C" void pioneer_q38_recurrent_context_only_uninit_driver(){}
extern "C" const char** pioneer_q38_recurrent_context_only_publish_devices(){return NULL;}
extern "C" device_hooks* pioneer_q38_recurrent_context_only_find_device(const char*){return NULL;}
extern "C" bool pioneer_recurrent_context_prefix(PioneerStoreContext& c,unsigned token,PioneerQ38ProjectionZeroResult& zero,PioneerQ38EmbeddingResult& embedding){
 return BindQ38EmbeddingModel(c,token,0,0) && InitializeQ38EmbeddingModel(c,zero) && RunQ38EmbeddingModel(c,embedding);
}
extern "C" bool pioneer_recurrent_context_held(PioneerStoreContext& c,PioneerQ38RecurrentBlockMemoryResult& memory){
 return BindQ38RecurrentBlockNative(c) && PrepareQ38RecurrentBlockNativeHeld(c,memory) && CloseQ38RecurrentBlockNativeHeld(c,memory);
}
extern "C" bool pioneer_recurrent_context_scoped_initialized_held(PioneerProbeCookie* cookie,unsigned token,
 PioneerQ38ProjectionZeroResult& zero,PioneerQ38EmbeddingResult& embedding,PioneerQ38StateZeroResult& state,
 PioneerQ38ConvHistoryZeroResult& history,PioneerQ38RecurrentBlockMemoryResult& memory){
 PioneerQ38GdnCallScope scope(cookie);auto* c=scope.Context();
 return c && BindQ38EmbeddingModel(*c,token,0,0) && InitializeQ38EmbeddingModel(*c,zero) && RunQ38EmbeddingModel(*c,embedding)
  && BindQ38RecurrentBlockNative(*c) && InitializeQ38RecurrentBlockState(*c,state) && InitializeQ38RecurrentBlockHistory(*c,history)
  && c->q38RecurrentBlockNative->InitializationsClosed(c->banks,c->baseline.baseline.baseline.baseline.dram)
  && PrepareQ38RecurrentBlockNativeHeld(*c,memory) && CloseQ38RecurrentBlockNativeHeld(*c,memory);
} // compile/inspect only;no old init flags or fabricated ready state
extern "C" bool pioneer_recurrent_context_scoped_held(PioneerProbeCookie* cookie,unsigned token,PioneerQ38ProjectionZeroResult& zero,
 PioneerQ38EmbeddingResult& embedding,PioneerQ38RecurrentBlockMemoryResult& memory){
 PioneerQ38GdnCallScope scope(cookie);auto* c=scope.Context();
 return c && BindQ38EmbeddingModel(*c,token,0,0) && InitializeQ38EmbeddingModel(*c,zero) && RunQ38EmbeddingModel(*c,embedding)
  && BindQ38RecurrentBlockNative(*c) && PrepareQ38RecurrentBlockNativeHeld(*c,memory) && CloseQ38RecurrentBlockNativeHeld(*c,memory);
} // genuine bodies WOULD perform IO if invoked;tools only compile+inspect
extern "C" bool pioneer_recurrent_context_scoped_execution(PioneerProbeCookie* cookie,unsigned token,
 PioneerQ38ProjectionZeroResult& zero,PioneerQ38EmbeddingResult& embedding,PioneerQ38StateZeroResult& state,
 PioneerQ38ConvHistoryZeroResult& history,PioneerQ38RecurrentBlockMemoryResult& memory,PioneerQ38RecurrentBlockResult& execution){
 PioneerQ38GdnCallScope scope(cookie);auto* c=scope.Context();
 return c && BindQ38EmbeddingModel(*c,token,0,0) && InitializeQ38EmbeddingModel(*c,zero) && RunQ38EmbeddingModel(*c,embedding)
  && BindQ38RecurrentBlockNative(*c) && InitializeQ38RecurrentBlockState(*c,state) && InitializeQ38RecurrentBlockHistory(*c,history)
  && PrepareQ38RecurrentBlockNativeHeld(*c,memory) && PrepareQ38RecurrentBlockNativeExecution(*c,execution)
  && RunQ38RecurrentBlockNativeExecution(*c,execution) && CloseQ38RecurrentBlockNativeExecution(*c,execution,memory);
} // actual complete body must compile,NEVER linked/installed/invoked here
extern "C" bool pioneer_recurrent_context_scoped_dispatch_end(PioneerProbeCookie* cookie,tt_resident_session_result& result,bool& clean){
 PioneerQ38GdnCallScope scope(cookie);auto* c=scope.Context();return c && EndQ38DispatchOwnership(*c,result,clean);
} // not a complete final bank-END/VM/clock/PCI/host disposal qualification
#ifdef PIONEER_Q38_RECURRENT_BLOCK_FINAL_RELEASE
extern "C" status_t pioneer_recurrent_context_locked_end(PioneerProbeCookie* cookie,tt_resident_session_result& result){
 status_t status=AcquireQ38GdnSessionLock(cookie);if(status!=B_OK)return status;
 struct{tt_resident_session_result result;}request={};
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38RecurrentBlockFinalEndCall.inc"
 result=request.result;release_sem(sLock);return status;
} // compile/inspect actual owned END+call seal,NEVER invoked by this test
#endif
extern "C" void pioneer_recurrent_context_retain(PioneerStoreContext& c,tt_resident_session_result& result){RetainQ38DispatchOwner(c,result);}
extern "C" void pioneer_recurrent_context_destroy_clean(PioneerStoreContext* c){delete c;}
extern "C" status_t pioneer_recurrent_context_expected_refusal(){return B_NOT_ALLOWED;}
extern "C" void* pioneer_recurrent_context_expected_null(){return NULL;}
extern "C" void pioneer_recurrent_context_expected_inert(){}
