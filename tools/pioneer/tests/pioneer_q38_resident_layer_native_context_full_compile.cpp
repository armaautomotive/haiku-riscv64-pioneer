/* SPDX-License-Identifier: MIT */
// ACTUAL whole driver/context/IO/factories/templates,COMPILE ONLY.
// QRLNODRV renames ALL driver ABI entries and makes them inert. No link,
// public execution selector,install,card access or physical qualification.
#define PIONEER_Q38_RESIDENT_LAYER_CONTEXT_COMPILE_ONLY 1
#define PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT 1
#define PIONEER_Q38_RESIDENT_LAYER_NATIVE_COMPONENT 1
#define PIONEER_Q38_RESIDENT_LAYER_OWNER 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_Q38_RESIDENT_OUTPUT_OWNER 1
#define PIONEER_Q38_DECODER_RUNTIME 1
#define PIONEER_Q38_PROJECTION_RESULT_STASH 1
#define PIONEER_Q38_PROJECTION_EXECUTION_OWNER 1
#define PIONEER_Q38_LEARNED_RMS_NATIVE 1
#define PIONEER_Q38_EMBEDDING_NATIVE 1
#include "../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp"
extern "C" bool pioneer_layer_context_bind(PioneerStoreContext& c){return BindQ38ResidentLayerNativeState(c);}
extern "C" bool pioneer_layer_context_initialize(PioneerStoreContext& c,PioneerQ38StateZeroResult& a,PioneerQ38ConvHistoryZeroResult& b){
 return InitializeQ38ResidentLayerRecurrent(c,a) && InitializeQ38ResidentLayerHistory(c,b);
}
extern "C" bool pioneer_layer_context_prefix(PioneerStoreContext& c,PioneerQ38ProjectionZeroResult& z,PioneerQ38EmbeddingResult& e,
 PioneerQ38LearnedRmsResult& r,PioneerQ38ProjectionGraphResult& p){
 return InitializeQ38EmbeddingModel(c,z) && RunQ38EmbeddingModel(c,e) && RunQ38LearnedRmsModel(c,r)
  && PrepareQ38ProjectionExecution(c,p) && RunQ38ProjectionExecution(c,10,p) && RunQ38ProjectionExecution(c,11,p)
  && RunQ38ProjectionExecution(c,5,p) && RunQ38ProjectionExecution(c,6,p) && CloseQ38ProjectionExecution(c,p);
}
extern "C" bool pioneer_layer_context_execute(PioneerStoreContext& c,PioneerQ38ResidentLayerResult& r){
 return PrepareQ38ResidentLayerNative(c,r) && RunQ38ResidentLayerNative(c,r) && CloseQ38ResidentLayerNative(c,r);
}
extern "C" bool pioneer_layer_context_end(PioneerStoreContext& c,tt_resident_session_result& r,bool& clean){return EndQ38DispatchOwnership(c,r,clean);}
extern "C" bool pioneer_layer_context_validation_readback(PioneerStoreContext& c,PioneerQ38ResidentLayerResult& r,
 uint32_t* values,unsigned words,PioneerQ38ResidentLayerReadbackResult& readback){
 return CloseQ38ResidentLayerNativeForValidation(c,r,values,words,readback);
}
extern "C" void pioneer_layer_context_destroy_clean(PioneerStoreContext* c){delete c;}
extern "C" status_t pioneer_layer_context_expected_refusal(){return B_NOT_ALLOWED;}
extern "C" void* pioneer_layer_context_expected_null(){return NULL;}
extern "C" void pioneer_layer_context_expected_inert(){}
