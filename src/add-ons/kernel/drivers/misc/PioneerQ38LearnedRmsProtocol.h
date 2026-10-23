/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_LEARNED_RMS_PROTOCOL_H
#define PIONEER_Q38_LEARNED_RMS_PROTOCOL_H
#include "PioneerQ38EmbeddingProtocol.h"
// Separate tag41/namespace12 DEBUG qualification. No Work or decoder grant,
// caller-supplied numerical inputs,device addresses,code,bank or ready hints.
#define TT_PCI_PROBE_Q38_RMS_VALIDATION_SUPPORT (B_DEVICE_OP_CODES_END+0x62c4)
#define TT_PCI_PROBE_Q38_RMS_VALIDATE (B_DEVICE_OP_CODES_END+0x62c5)
struct tt_q38_learned_rms_validation {
 tt_q38_embedding_produce metadata;
 uint32_t normalized[5120],residual[5120]; // actual fixed GDDR spans;allzero on input
};
static_assert(sizeof(tt_q38_learned_rms_validation)==41032,"fixed pointer-free RMS DEBUG reply only");
#endif
