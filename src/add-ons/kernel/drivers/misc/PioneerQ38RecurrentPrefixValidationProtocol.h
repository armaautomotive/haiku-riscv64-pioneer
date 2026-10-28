/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_PROTOCOL_H
#define PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_PROTOCOL_H
#include <stddef.h>
#include "PioneerQ38EmbeddingProtocol.h"
// RESERVED DISTINCT DEBUG73/namespace24 for actual layers0..2/token0.
// DATA contract only: no physical selector/control/module exists yet.
// No pointers,addresses,code,thresholds,producer setters or caller counts.
// All reply bytes must be zero input. Successful output is returned only
// after actual complete locked final-END;not the old pre-END layer0 reply.
#define TT_PCI_PROBE_Q38_RECURRENT_PREFIX_VALIDATION_SUPPORT (B_DEVICE_OP_CODES_END+0x6502)
#define TT_PCI_PROBE_Q38_RECURRENT_PREFIX_VALIDATE (B_DEVICE_OP_CODES_END+0x6503)
static constexpr uint64_t TT_Q38_RECURRENT_PREFIX_VALIDATION_BUILD_TAG=UINT64_C(0x2026102800010073);
enum {TT_Q38_RECURRENT_PREFIX_VALIDATION_NAMESPACE=24,TT_Q38_RECURRENT_PREFIX_VALIDATION_WORDS=5120,
 TT_Q38_RECURRENT_PREFIX_VALIDATION_PAGES=5};
struct tt_q38_recurrent_prefix_validation {
 tt_q38_embedding_produce metadata;
 uint32_t zero_runs,embedding_roots,recurrent_regions,history_regions,recurrent_pages,history_pages;
 uint32_t cohort_prepared,cohort_completed,cohort_stopped,cohort_held,root_runs,root_holds,packets;
 uint32_t saved_words,verified_words,installed_words,restored_words,readback_pages,readback_words,final_rows;
 uint32_t layers_completed,matrices_completed,root_stop_request,final_end_state,pci_restored,host_released,capabilities;
 uint32_t reserved_reply[51];
 uint32_t values[TT_Q38_RECURRENT_PREFIX_VALIDATION_WORDS]; // HEAP ONLY
};
static_assert(offsetof(tt_q38_recurrent_prefix_validation,values)==384,"distinct96-word reply header");
static_assert(sizeof(tt_q38_recurrent_prefix_validation)==20864,"no old layer0 payload alias");
static inline bool PioneerQ38RecurrentPrefixValidationRequest(const tt_q38_recurrent_prefix_validation& r){
 if(!PioneerQ38EmbeddingRequest(r.metadata) || r.metadata.token || r.metadata.position)return false;
 const auto* bytes=reinterpret_cast<const unsigned char*>(&r);
 for(size_t i=sizeof(r.metadata);i<sizeof(r);i++)if(bytes[i])return false;
 return true;
} // shape only;NEVER source/native/launch/STOP/recovery/Work authority
#endif
