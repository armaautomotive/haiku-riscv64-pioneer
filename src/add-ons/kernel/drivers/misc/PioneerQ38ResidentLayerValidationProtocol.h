/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_LAYER_VALIDATION_PROTOCOL_H
#define PIONEER_Q38_RESIDENT_LAYER_VALIDATION_PROTOCOL_H
#include <stddef.h>
#include "PioneerQ38EmbeddingProtocol.h"
// Separate physical DEBUG69/namespace20. Fixed token0/position0/layer0
// ONLY. No addresses,pointers,routes,code,numeric inputs,thresholds,owner,
// ready/current-slot flags or cleanup grants. Never an old68 payload alias.
#define TT_PCI_PROBE_Q38_RESIDENT_LAYER_VALIDATION_SUPPORT (B_DEVICE_OP_CODES_END+0x6300)
#define TT_PCI_PROBE_Q38_RESIDENT_LAYER_VALIDATE (B_DEVICE_OP_CODES_END+0x6301)
enum {TT_Q38_RESIDENT_LAYER_VALIDATION_NAMESPACE=20,
 TT_Q38_RESIDENT_LAYER_VALIDATION_FINAL=0,TT_Q38_RESIDENT_LAYER_VALIDATION_CONV=5120,
 TT_Q38_RESIDENT_LAYER_VALIDATION_HISTORY=15360,TT_Q38_RESIDENT_LAYER_VALIDATION_GATED=46080,
 TT_Q38_RESIDENT_LAYER_VALIDATION_PROJECTION=52224,TT_Q38_RESIDENT_LAYER_VALIDATION_CURRENT=57344,
 TT_Q38_RESIDENT_LAYER_VALIDATION_NEXT=843776,TT_Q38_RESIDENT_LAYER_VALIDATION_WORDS=1630208,
 TT_Q38_RESIDENT_LAYER_VALIDATION_PAGES=1592};
struct tt_q38_resident_layer_validation {
 tt_q38_embedding_produce metadata;
 uint32_t state_runs,state_pages,history_runs,history_pages;
 uint32_t projection_runs,projection_boots,projection_root_runs,projection_root_holds;
 uint32_t cohort_boots,cohort_root_runs,cohort_root_holds,cohort_stopped,cohort_held;
 uint32_t convolution_jobs,gdn_halves,output_chunks,output_rows;
 uint32_t readback_pages,readback_words,current_slot,next_slot;
 uint32_t saved_words,verified_words,installed_words,restored_words;
 uint32_t prefix_saved_words,prefix_verified_words,prefix_installed_words,prefix_restored_words;
 uint32_t ffn_matrices,ffn_producers,final_rows,reserved_reply[14];
 uint32_t values[TT_Q38_RESIDENT_LAYER_VALIDATION_WORDS]; // HEAP ONLY,ALLzero on input
};
static_assert(offsetof(tt_q38_resident_layer_validation,values)==256,"fixed64-word header,NO implicit padding");
static_assert(sizeof(tt_q38_resident_layer_validation)==6521088,"NEW fixed QLV1 diagnostic,HEAP ONLY");
static inline bool PioneerQ38ResidentLayerValidationRequest(const tt_q38_resident_layer_validation& r){
 if(!PioneerQ38EmbeddingRequest(r.metadata) || r.metadata.token || r.metadata.position)return false;
 const auto* bytes=reinterpret_cast<const unsigned char*>(&r);
 for(size_t i=sizeof(r.metadata);i<sizeof(r);i++)if(bytes[i])return false;
 return true;
} // shape ONLY,not native/source/compute/cleanup authority
#endif
