/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_INPUT_VALIDATION_PROTOCOL_H
#define PIONEER_Q38_GDN_INPUT_VALIDATION_PROTOCOL_H
#include "PioneerQ38LearnedRmsProtocol.h"
// Exact DEBUG46/namespace15 first-token/layer0 QCB1->QGI1 qualifier.
// Logical token only,position0;ALL output fields zero on input. No vector,
// address,program,bank,producer/state-init or full-inference capability.
#define TT_PCI_PROBE_Q38_GDN_INPUT_VALIDATION_SUPPORT (B_DEVICE_OP_CODES_END+0x62ca)
#define TT_PCI_PROBE_Q38_GDN_INPUT_VALIDATE (B_DEVICE_OP_CODES_END+0x62cb)
struct tt_q38_gdn_input_validation {
 tt_q38_embedding_produce metadata;
 uint32_t history_runs,history_pages;
 uint32_t projections,peer_boots,peer_stopped,peer_held,root_starts,root_holds;
 uint32_t convolution_workers,convolution_jobs,convolution_channels,convolution_held;
 uint32_t input_workers,input_installed,input_armed,input_released,input_completed,input_held;
 uint32_t readback_workers,readback_words,saved_words,verified_words,installed_words,restored_words;
 uint32_t values[30912]; // half0..95,each128Q+128K+64V+decay+beta
};
static_assert(sizeof(tt_q38_gdn_input_validation)==123816,"fixed pointer-free HEAP DEBUG reply,not an address/read capability");
#endif
