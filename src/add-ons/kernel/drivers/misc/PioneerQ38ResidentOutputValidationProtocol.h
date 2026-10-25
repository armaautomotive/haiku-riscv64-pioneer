/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_PROTOCOL_H
#define PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_PROTOCOL_H
#include <stddef.h>
#include "PioneerQ38EmbeddingProtocol.h"
// Separate combined-output DEBUG68/namespace19. The actual private native
// lifetime and original-GGUF qualification are REQUIRED before Work;
// compile-only67 is NOT this physical role or a loadable driver.
// Logical token/position0/layer0 ONLY. No pointers,addresses,routes,code,
// numeric inputs,thresholds,owner/readiness/current-slot or cleanup grants.
#define TT_PCI_PROBE_Q38_RESIDENT_OUTPUT_VALIDATION_SUPPORT (B_DEVICE_OP_CODES_END+0x62f0)
#define TT_PCI_PROBE_Q38_RESIDENT_OUTPUT_VALIDATE (B_DEVICE_OP_CODES_END+0x62f1)
enum {TT_Q38_RESIDENT_OUTPUT_VALIDATION_NAMESPACE=19,
 TT_Q38_RESIDENT_OUTPUT_VALIDATION_CONV=0,TT_Q38_RESIDENT_OUTPUT_VALIDATION_HISTORY=10240,
 TT_Q38_RESIDENT_OUTPUT_VALIDATION_GATED=40960,TT_Q38_RESIDENT_OUTPUT_VALIDATION_OPERANDS=47104,
 TT_Q38_RESIDENT_OUTPUT_VALIDATION_LINES=50176,TT_Q38_RESIDENT_OUTPUT_VALIDATION_SCALES=50944,
 TT_Q38_RESIDENT_OUTPUT_VALIDATION_RESULT=51136,TT_Q38_RESIDENT_OUTPUT_VALIDATION_CURRENT=56256,
 TT_Q38_RESIDENT_OUTPUT_VALIDATION_NEXT=842688,TT_Q38_RESIDENT_OUTPUT_VALIDATION_WORDS=1629120,
 TT_Q38_RESIDENT_OUTPUT_VALIDATION_PAGES=1592};
struct tt_q38_resident_output_validation {
 tt_q38_embedding_produce metadata;
 uint32_t state_runs,state_pages,history_runs,history_pages;
 uint32_t projection_runs,projection_boots,projection_root_runs,projection_root_holds;
 uint32_t cohort_boots,cohort_root_runs,cohort_root_holds,cohort_stopped,cohort_held;
 uint32_t convolution_jobs,gdn_halves,output_chunks,output_rows;
 uint32_t readback_pages,readback_words,current_slot,next_slot;
 uint32_t saved_words,verified_words,installed_words,restored_words;
 uint32_t prefix_saved_words,prefix_verified_words,prefix_installed_words,prefix_restored_words;
 uint32_t reserved_reply[17];
 uint32_t values[TT_Q38_RESIDENT_OUTPUT_VALIDATION_WORDS]; // HEAP ONLY;ALLzero on input
};
static_assert(offsetof(tt_q38_resident_output_validation,values)==256,"fixed64-word header,no implicit padding");
static_assert(sizeof(tt_q38_resident_output_validation)==6516736,"fixed pointer-free combined-output DEBUG reply,HEAP ONLY");
static inline bool PioneerQ38ResidentOutputValidationRequest(const tt_q38_resident_output_validation& r){
 if(!PioneerQ38EmbeddingRequest(r.metadata) || r.metadata.position)return false;
 const auto* bytes=reinterpret_cast<const unsigned char*>(&r);
 for(size_t i=sizeof(r.metadata);i<sizeof(r);i++)if(bytes[i])return false;
 return true;
} // Request SHAPE ONLY;never a source/IO/producer/native execution capability
#endif
