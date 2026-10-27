/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_PROTOCOL_H
#define PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_PROTOCOL_H
#include <stddef.h>
#include "PioneerQ38EmbeddingProtocol.h"
// Distinct DEBUG71/namespace22. Fixed token0,position0,layer0,bank0.
// Pointer/address/threshold/code-free. ALL reply bytes MUST be zero input.
// A reply is NOT a state/token commit or permission to clean an unknown run.
#define TT_PCI_PROBE_Q38_RECURRENT_BLOCK_VALIDATION_SUPPORT (B_DEVICE_OP_CODES_END+0x6402)
#define TT_PCI_PROBE_Q38_RECURRENT_BLOCK_VALIDATE (B_DEVICE_OP_CODES_END+0x6403)
enum {TT_Q38_RECURRENT_BLOCK_VALIDATION_NAMESPACE=22,TT_Q38_RECURRENT_BLOCK_VALIDATION_WORDS=5120,
 TT_Q38_RECURRENT_BLOCK_VALIDATION_PAGES=5};
struct tt_q38_recurrent_block_validation {
 tt_q38_embedding_produce metadata;
 uint32_t zero_runs,embedding_roots,recurrent_regions,history_regions,recurrent_pages,history_pages;
 uint32_t cohort_prepared,cohort_completed,cohort_stopped,cohort_held,root_runs,root_holds,packets;
 uint32_t saved_words,verified_words,installed_words,restored_words,readback_pages,readback_words,final_rows;
 uint32_t reserved_reply[42];
 uint32_t values[TT_Q38_RECURRENT_BLOCK_VALIDATION_WORDS]; // HEAP ONLY
};
static_assert(offsetof(tt_q38_recurrent_block_validation,values)==320,"distinct80-word reply header");
static_assert(sizeof(tt_q38_recurrent_block_validation)==20800,"separate fixed recurrent diagnostic payload");
static inline bool PioneerQ38RecurrentBlockValidationRequest(const tt_q38_recurrent_block_validation& r){
 if(!PioneerQ38EmbeddingRequest(r.metadata) || r.metadata.token || r.metadata.position)return false;
 const auto* bytes=reinterpret_cast<const unsigned char*>(&r);
 for(size_t i=sizeof(r.metadata);i<sizeof(r);i++)if(bytes[i])return false;
 return true;
} // data shape only;NO source/native/launch/close authority
#endif
