/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_PROTOCOL_H
#define PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_PROTOCOL_H
#include <stddef.h>
#include "PioneerQ38LearnedRmsProtocol.h"
// Separate DEBUG52/namespace17. Token/position0/layer0 ONLY;no addresses,
// programs,numeric inputs,state/producer/cleanup grants or inference ABI.
#define TT_PCI_PROBE_Q38_GDN_RENDEZVOUS_VALIDATION_SUPPORT (B_DEVICE_OP_CODES_END+0x62ce)
#define TT_PCI_PROBE_Q38_GDN_RENDEZVOUS_VALIDATE (B_DEVICE_OP_CODES_END+0x62cf)
enum {TT_Q38_GDN_RENDEZVOUS_VALIDATION_HALF_WORDS=8642,
 TT_Q38_GDN_RENDEZVOUS_VALIDATION_WORDS=829632};
struct tt_q38_gdn_rendezvous_validation {
 tt_q38_embedding_produce metadata;
 uint32_t state_runs,state_pages,history_runs,history_pages;
 uint32_t projections,peer_boots,peer_stopped,peer_held,root_starts,root_holds;
 uint32_t convolution_workers,convolution_jobs,convolution_channels,convolution_held;
 uint32_t input_workers,input_installed,input_armed,input_released,input_completed,input_held;
 uint32_t layer_published,readback_workers,readback_words,saved_words,verified_words,installed_words,restored_words;
 uint32_t gated_published,readback_half_words,reserved;
 uint32_t values[TT_Q38_GDN_RENDEZVOUS_VALIDATION_WORDS]; // each half:322prepared+8192NEXT+64raw+64gated
};
static_assert(sizeof(tt_q38_gdn_rendezvous_validation)==3318720,"fixed pointer-free HEAP reply;no read/address capability");
static_assert(offsetof(tt_q38_gdn_rendezvous_validation,values)==192,"no implicit output padding in DEBUG52 ABI");
#endif
