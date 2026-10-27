/* SPDX-License-Identifier: MIT */
// Instantiate the ACTUAL production context/IO/factories,never a mock IO.
// All conventional driver entries are renamed AND independently inert.
#define PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT 1
#define PIONEER_Q38_MIXED_LIVE_CONTEXT_COMPILE_ONLY 1
#define PIONEER_Q38_MIXED_LIVE_EMIT_CONTEXT_BODY 1
#define PIONEER_Q38_EMBEDDING_NATIVE 1
#define PIONEER_Q38_DECODER_RUNTIME 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
extern "C" {
 extern int32 pioneer_q38_live_context_body_api_version;
 status_t pioneer_q38_live_context_body_init_hardware();status_t pioneer_q38_live_context_body_init_driver();
 void pioneer_q38_live_context_body_uninit_driver();const char** pioneer_q38_live_context_body_publish_devices();
 device_hooks* pioneer_q38_live_context_body_find_device(const char*);
}
#define api_version pioneer_q38_live_context_body_api_version
#define init_hardware pioneer_q38_live_context_body_init_hardware
#define init_driver pioneer_q38_live_context_body_init_driver
#define uninit_driver pioneer_q38_live_context_body_uninit_driver
#define publish_devices pioneer_q38_live_context_body_publish_devices
#define find_device pioneer_q38_live_context_body_find_device
#include "../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp"
// Do not erase genuine private globals/factories by making the ONLY
// referencing setup functions constant refusals: that folds every scope
// to false and is not a meaningful native integration compile witness.
// The REAL bodies above are private renamed symbols,not driver ABI. The
// independent inert controls below ALSO have no conventional ABI names.
extern "C" status_t pioneer_q38_live_context_only_init_hardware(){return B_NOT_ALLOWED;}
extern "C" status_t pioneer_q38_live_context_only_init_driver(){return B_NOT_ALLOWED;}
extern "C" void pioneer_q38_live_context_only_uninit_driver(){}
extern "C" const char** pioneer_q38_live_context_only_publish_devices(){return NULL;}
extern "C" device_hooks* pioneer_q38_live_context_only_find_device(const char*){return NULL;}
extern "C" bool pioneer_live_context_prefix(PioneerStoreContext& c,unsigned token,PioneerQ38ProjectionZeroResult& z,PioneerQ38EmbeddingResult& e){
 return BindQ38EmbeddingModel(c,token,0,0) && InitializeQ38EmbeddingModel(c,z) && RunQ38EmbeddingModel(c,e);
}
extern "C" bool pioneer_live_context_prepare_held(PioneerStoreContext& c,PioneerQ38MixedLiveMemoryResult& r){
 return BindQ38MixedLiveNative(c) && PrepareQ38MixedLiveNativeHeld(c,r);
}
extern "C" bool pioneer_live_context_close_held(PioneerStoreContext& c,PioneerQ38MixedLiveMemoryResult& r){return CloseQ38MixedLiveNativeHeld(c,r);}
// Genuine call-scope construction: no caller assignment to the private
// context's call thread/global FD, no synthesized completed input state.
// This would execute the actual prefix/journal if invoked. Compile ONLY.
extern "C" bool pioneer_live_context_scoped_held(PioneerProbeCookie* cookie,unsigned token,
 PioneerQ38ProjectionZeroResult& z,PioneerQ38EmbeddingResult& e,PioneerQ38MixedLiveMemoryResult& r){
 PioneerQ38GdnCallScope scope(cookie);auto* c=scope.Context();
 return c && BindQ38EmbeddingModel(*c,token,0,0) && InitializeQ38EmbeddingModel(*c,z) && RunQ38EmbeddingModel(*c,e)
  && BindQ38MixedLiveNative(*c) && PrepareQ38MixedLiveNativeHeld(*c,r) && CloseQ38MixedLiveNativeHeld(*c,r);
}
extern "C" bool pioneer_live_context_scoped_execute(PioneerProbeCookie* cookie,unsigned token,
 PioneerQ38ProjectionZeroResult& z,PioneerQ38EmbeddingResult& e,PioneerQ38MixedLiveMemoryResult& memory,PioneerQ38MixedLiveResult& execution){
 PioneerQ38GdnCallScope scope(cookie);auto* c=scope.Context();
 return c && BindQ38EmbeddingModel(*c,token,0,0) && InitializeQ38EmbeddingModel(*c,z) && RunQ38EmbeddingModel(*c,e)
  && BindQ38MixedLiveNative(*c) && PrepareQ38MixedLiveNativeHeld(*c,memory) && PrepareQ38MixedLiveNativeExecution(*c,execution)
  && RunQ38MixedLiveNativeExecution(*c,execution) && CloseQ38MixedLiveNativeExecution(*c,execution,memory);
} // compile ONLY. Genuine native execution/containment/ORIGINAL-weight qualification required before card use.
#ifdef PIONEER_Q38_MIXED_LIVE_VALIDATION_READBACK
extern "C" bool pioneer_live_context_scoped_validation(PioneerProbeCookie* cookie,unsigned token,
 PioneerQ38ProjectionZeroResult& z,PioneerQ38EmbeddingResult& e,PioneerQ38MixedLiveMemoryResult& memory,PioneerQ38MixedLiveResult& execution,
 uint32_t* values,unsigned words,PioneerQ38MixedLiveReadbackResult& readback){
 PioneerQ38GdnCallScope scope(cookie);auto* c=scope.Context();
 return c && BindQ38EmbeddingModel(*c,token,0,0) && InitializeQ38EmbeddingModel(*c,z) && RunQ38EmbeddingModel(*c,e)
  && BindQ38MixedLiveNative(*c) && PrepareQ38MixedLiveNativeHeld(*c,memory) && PrepareQ38MixedLiveNativeExecution(*c,execution)
  && RunQ38MixedLiveNativeExecution(*c,execution) && CloseQ38MixedLiveNativeValidation(*c,execution,memory,values,words,readback);
} // COMPILE ONLY;trusted kernel storage,not a direct user buffer or loadable validation entry
#endif
extern "C" bool pioneer_live_context_end(PioneerStoreContext& c,tt_resident_session_result& r,bool& clean){return EndQ38DispatchOwnership(c,r,clean);}
#ifdef PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE
extern "C" status_t pioneer_live_context_final_end(PioneerProbeCookie* cookie,tt_resident_session_result& r){
 status_t status=AcquireQ38GdnSessionLock(cookie);if(status!=B_OK)return status;
 struct{tt_resident_session_result result;}request={};
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38MixedLiveFinalEndCall.inc"
 r=request.result;release_sem(sLock);return status;
} // Compile ONLY;ACTUAL lock+caller seal+END,true recheck,retained-or-destroyed postcondition.
#endif
extern "C" void pioneer_live_context_retain(PioneerStoreContext& c,tt_resident_session_result& r){RetainQ38DispatchOwner(c,r);}
extern "C" void pioneer_live_context_destroy_clean(PioneerStoreContext* c){delete c;}
extern "C" status_t pioneer_live_context_expected_refusal(){return B_NOT_ALLOWED;}
extern "C" void* pioneer_live_context_expected_null(){return NULL;}
extern "C" void pioneer_live_context_expected_inert(){}
