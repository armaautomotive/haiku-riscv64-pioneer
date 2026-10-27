/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_LIVE_VALIDATION_PROTOCOL_H
#define PIONEER_Q38_MIXED_LIVE_VALIDATION_PROTOCOL_H
#include <stddef.h>
#include "PioneerQ38EmbeddingProtocol.h"
// Separate DEBUG70/namespace21. Fixed token0/position0/standalone layer3.
// NOT a valid model prefix/token. No addresses,code,numeric inputs,routes,
// thresholds,ready/owner/cleanup grants. ALL reply bytes MUST be zero input.
#define TT_PCI_PROBE_Q38_MIXED_LIVE_VALIDATION_SUPPORT (B_DEVICE_OP_CODES_END+0x6400)
#define TT_PCI_PROBE_Q38_MIXED_LIVE_VALIDATE (B_DEVICE_OP_CODES_END+0x6401)
enum {TT_Q38_MIXED_LIVE_VALIDATION_NAMESPACE=21,TT_Q38_MIXED_LIVE_VALIDATION_WORDS=5120,
 TT_Q38_MIXED_LIVE_VALIDATION_PAGES=5};
struct tt_q38_mixed_live_validation {
 tt_q38_embedding_produce metadata;
 uint32_t zero_runs,embedding_roots,cohort_prepared,cohort_completed,cohort_stopped,cohort_held;
 uint32_t root_runs,root_holds,packets,saved_words,verified_words,installed_words,restored_words;
 uint32_t readback_pages,readback_words,final_rows,reserved_reply[30];
 uint32_t values[TT_Q38_MIXED_LIVE_VALIDATION_WORDS]; // HEAP ONLY
};
static_assert(offsetof(tt_q38_mixed_live_validation,values)==256,"fixed64-word header");
static_assert(sizeof(tt_q38_mixed_live_validation)==20736,"separate fixed dense diagnostic payload");
static inline bool PioneerQ38MixedLiveValidationRequest(const tt_q38_mixed_live_validation& r){
 if(!PioneerQ38EmbeddingRequest(r.metadata) || r.metadata.token || r.metadata.position)return false;
 const auto* bytes=reinterpret_cast<const unsigned char*>(&r);
 for(size_t i=sizeof(r.metadata);i<sizeof(r);i++)if(bytes[i])return false;
 return true;
} // shape only,NOT source/native/execution/teardown authority
#endif
