/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_EMBEDDING_PROTOCOL_H
#define PIONEER_Q38_EMBEDDING_PROTOCOL_H
#include <stdint.h>
// Exact tag39/namespace10 qualification,NOT an Assistant/model decoder.
// Logical token+context position ONLY;no input vector/addresses/code/nonce/
// source receipt/ready flags or arbitrary mutable-memory capability.
#define TT_PCI_PROBE_Q38_EMBEDDING_SUPPORT (B_DEVICE_OP_CODES_END+0x62c0)
#define TT_PCI_PROBE_Q38_EMBEDDING_PRODUCE (B_DEVICE_OP_CODES_END+0x62c1)
// Separate tag40/namespace11 DEBUG endpoints;never accepted by tag39 Work.
#define TT_PCI_PROBE_Q38_EMBEDDING_VALIDATION_SUPPORT (B_DEVICE_OP_CODES_END+0x62c2)
#define TT_PCI_PROBE_Q38_EMBEDDING_VALIDATE (B_DEVICE_OP_CODES_END+0x62c3)
struct tt_q38_embedding_support {
 uint32_t version,driver,model_namespace,inputs,vocabulary,runtime_pages,no_cpu_reference,model_inference;
};
struct tt_q38_embedding_produce {
 uint32_t version,token,position,reserved;
 uint32_t state,closed,input_ready,recovery_required;
 uint64_t session,driver_generation,elapsed_us;
 uint32_t source_pages,workspace_pages,saved_words,restored_words;
};
static_assert(sizeof(tt_q38_embedding_produce)==72,"fixed pointer-free embedding metadata only");
struct tt_q38_embedding_validation {
 tt_q38_embedding_produce metadata;
 uint32_t values[5120]; // actual GDDR input readback,DEBUG ONLY,allzero on input
};
static_assert(sizeof(tt_q38_embedding_validation)==20552,"fixed numeric debug reply;NO addresses/pointers");
static inline bool PioneerQ38EmbeddingRequest(const tt_q38_embedding_produce& r){
 return r.version==1 && r.token<248320 && r.position<8192 && !r.reserved && !r.state && !r.closed && !r.input_ready
  && !r.recovery_required && !r.session && !r.driver_generation && !r.elapsed_us && !r.source_pages && !r.workspace_pages
  && !r.saved_words && !r.restored_words;
}
#endif
