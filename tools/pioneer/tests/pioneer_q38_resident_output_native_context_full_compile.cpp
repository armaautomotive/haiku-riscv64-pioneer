/* SPDX-License-Identifier: MIT */
// ACTUAL whole driver/context/IO/factories/templates,COMPILE ONLY.
// Role67 renames ALL driver ABI entries and makes them inert. No link,
// public execution selector,install,card access or physical qualification.
#define PIONEER_Q38_RESIDENT_OUTPUT_CONTEXT_COMPILE_ONLY 1
#define PIONEER_Q38_RESIDENT_OUTPUT_OWNER 1
#define PIONEER_Q38_DECODER_RUNTIME 1
#define PIONEER_Q38_PROJECTION_RESULT_STASH 1
#define PIONEER_Q38_PROJECTION_EXECUTION_OWNER 1
#define PIONEER_Q38_LEARNED_RMS_NATIVE 1
#define PIONEER_Q38_EMBEDDING_NATIVE 1
#include "../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp"
extern "C" bool pioneer_output_context_bind(PioneerStoreContext& c){return BindQ38ResidentOutputNativeState(c);}
extern "C" bool pioneer_output_context_initialize(PioneerStoreContext& c,PioneerQ38StateZeroResult& a,PioneerQ38ConvHistoryZeroResult& b){
 return InitializeQ38ResidentOutputRecurrent(c,a) && InitializeQ38ResidentOutputHistory(c,b);
}
extern "C" bool pioneer_output_context_prefix(PioneerStoreContext& c,PioneerQ38ProjectionZeroResult& z,PioneerQ38EmbeddingResult& e,
 PioneerQ38LearnedRmsResult& r,PioneerQ38ProjectionGraphResult& p){
 return InitializeQ38EmbeddingModel(c,z) && RunQ38EmbeddingModel(c,e) && RunQ38LearnedRmsModel(c,r)
  && PrepareQ38ProjectionExecution(c,p) && RunQ38ProjectionExecution(c,10,p) && RunQ38ProjectionExecution(c,11,p)
  && RunQ38ProjectionExecution(c,5,p) && RunQ38ProjectionExecution(c,6,p) && CloseQ38ProjectionExecution(c,p);
}
extern "C" bool pioneer_output_context_execute(PioneerStoreContext& c,PioneerQ38ResidentOutputResult& r){
 return PrepareQ38ResidentOutputNative(c,r) && RunQ38ResidentOutputNative(c,r) && CloseQ38ResidentOutputNative(c,r);
}
extern "C" bool pioneer_output_context_end(PioneerStoreContext& c,tt_resident_session_result& r,bool& clean){return EndQ38DispatchOwnership(c,r,clean);}
extern "C" void pioneer_output_context_destroy_clean(PioneerStoreContext* c){delete c;}
extern "C" status_t pioneer_output_context_expected_refusal(){return B_NOT_ALLOWED;}
extern "C" void* pioneer_output_context_expected_null(){return NULL;}
extern "C" void pioneer_output_context_expected_inert(){}
