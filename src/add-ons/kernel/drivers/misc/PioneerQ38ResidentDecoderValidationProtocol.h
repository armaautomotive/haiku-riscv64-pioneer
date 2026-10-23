/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_DECODER_VALIDATION_PROTOCOL_H
#define PIONEER_Q38_RESIDENT_DECODER_VALIDATION_PROTOCOL_H
#include "PioneerQ38LearnedRmsProtocol.h"
// Separate tag44/namespace14 DEBUG qualification of first-token/layer0
// retained QPS1 -> QCB1. Logical token only,position MUST be zero. No
// vector,address,bank,program,read capability or full-inference promotion.
#define TT_PCI_PROBE_Q38_DECODER_VALIDATION_SUPPORT (B_DEVICE_OP_CODES_END+0x62c8)
#define TT_PCI_PROBE_Q38_DECODER_VALIDATE (B_DEVICE_OP_CODES_END+0x62c9)
struct tt_q38_resident_decoder_validation {
 tt_q38_embedding_produce metadata;
 uint32_t history_runs,history_pages;
 uint32_t projections,peer_boots,peer_stopped,peer_held,root_starts,root_holds;
 uint32_t convolution_workers,convolution_jobs,convolution_channels,convolution_held;
 uint32_t readback_pages,readback_words,saved_words,verified_words,installed_words,restored_words;
 uint32_t values[40960]; // 10240 outputs then30720 PRIVATE next-history words
};
static_assert(sizeof(tt_q38_resident_decoder_validation)==163984,"fixed HEAP pointer-free DEBUG reply,ALL outputs zero on input");
#endif
