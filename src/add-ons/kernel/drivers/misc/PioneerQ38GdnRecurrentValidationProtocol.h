/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_RECURRENT_VALIDATION_PROTOCOL_H
#define PIONEER_Q38_GDN_RECURRENT_VALIDATION_PROTOCOL_H
#include <stddef.h>
#include "PioneerQ38LearnedRmsProtocol.h"
// Separate DEBUG49/namespace16. Logical token,position0/layer0 ONLY.
// No addresses,programs,CPU vectors,state/producer grants or inference ABI.
#define TT_PCI_PROBE_Q38_GDN_RECURRENT_VALIDATION_SUPPORT (B_DEVICE_OP_CODES_END+0x62cc)
#define TT_PCI_PROBE_Q38_GDN_RECURRENT_VALIDATE (B_DEVICE_OP_CODES_END+0x62cd)
enum {TT_Q38_GDN_RECURRENT_VALIDATION_HALF_WORDS=8578,
 TT_Q38_GDN_RECURRENT_VALIDATION_WORDS=823488};
struct tt_q38_gdn_recurrent_validation {
 tt_q38_embedding_produce metadata;
 uint32_t state_runs,state_pages,history_runs,history_pages;
 uint32_t projections,peer_boots,peer_stopped,peer_held,root_starts,root_holds;
 uint32_t convolution_workers,convolution_jobs,convolution_channels,convolution_held;
 uint32_t input_workers,input_installed,input_armed,input_released,input_completed,input_held;
 uint32_t layer_published,readback_workers,readback_words,saved_words,verified_words,installed_words,restored_words;
 uint32_t reserved; // explicit ABI alignment word,zero on input AND output
 uint32_t values[TT_Q38_GDN_RECURRENT_VALIDATION_WORDS]; // each half:322prepared+8192NEXT+64output
};
static_assert(sizeof(tt_q38_gdn_recurrent_validation)==3294136,"fixed pointer-free HEAP reply;not a read/address capability");
static_assert(offsetof(tt_q38_gdn_recurrent_validation,values)==184,"no implicit output padding in DEBUG ABI");
#endif
