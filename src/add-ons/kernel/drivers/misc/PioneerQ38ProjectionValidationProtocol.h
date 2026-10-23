/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PROJECTION_VALIDATION_PROTOCOL_H
#define PIONEER_Q38_PROJECTION_VALIDATION_PROTOCOL_H
#include "PioneerQ38LearnedRmsProtocol.h"
// Separate tag42/namespace13 fixed first-layer QPS1 DEBUG qualification.
// Input ONLY logical token/position plus zeroed output fields. Never a
// Work/model-inference role,caller vector/address/bank/code or read grant.
#define TT_PCI_PROBE_Q38_PROJECTION_VALIDATION_SUPPORT (B_DEVICE_OP_CODES_END+0x62c6)
#define TT_PCI_PROBE_Q38_PROJECTION_VALIDATE (B_DEVICE_OP_CODES_END+0x62c7)
struct tt_q38_projection_validation {
 tt_q38_embedding_produce metadata;
 uint32_t projections,peer_boots,peer_stopped,peer_held,root_starts,root_holds;
 uint32_t readback_pages,readback_words,saved_words,verified_words,installed_words,restored_words;
 uint32_t values[18432]; // complete fixed padded73728B span,allzero on input
};
static_assert(sizeof(tt_q38_projection_validation)==73848,"fixed HEAP pointer-free DEBUG result only");
#endif
