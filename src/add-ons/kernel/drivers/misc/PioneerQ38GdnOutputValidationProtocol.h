/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_OUTPUT_VALIDATION_PROTOCOL_H
#define PIONEER_Q38_GDN_OUTPUT_VALIDATION_PROTOCOL_H
#include "PioneerQ38GdnRendezvousValidationProtocol.h"
// Separate DEBUG55/namespace18. Logical token,position0,layer0 ONLY.
// Fixed output DATA,not arbitrary addresses,numeric inputs,programs,
// owner/ready/cleanup capabilities or a model/Work inference ABI.
#define TT_PCI_PROBE_Q38_GDN_OUTPUT_VALIDATION_SUPPORT (B_DEVICE_OP_CODES_END+0x62d0)
#define TT_PCI_PROBE_Q38_GDN_OUTPUT_VALIDATE (B_DEVICE_OP_CODES_END+0x62d1)
enum {TT_Q38_GDN_OUTPUT_VALIDATION_OPERANDS=829632,
 TT_Q38_GDN_OUTPUT_VALIDATION_SCALES=832704,
 TT_Q38_GDN_OUTPUT_VALIDATION_RESULT=832896,
 TT_Q38_GDN_OUTPUT_VALIDATION_WORDS=838016};
struct tt_q38_gdn_output_validation {
 tt_q38_embedding_produce metadata;
 uint32_t state_runs,state_pages,history_runs,history_pages;
 uint32_t projections,peer_boots,peer_stopped,peer_held,root_starts,root_holds;
 uint32_t convolution_workers,convolution_jobs,convolution_channels,convolution_held;
 uint32_t input_workers,input_installed,input_armed,input_released,input_completed,input_held;
 uint32_t layer_published,readback_workers,readback_words,saved_words,verified_words,installed_words,restored_words;
 uint32_t gated_published,readback_half_words,reserved;
 // Earlier root/peer counts above retain their pre-output-stage meaning.
 uint32_t output_installed,output_booted,output_root_runs,output_stopped,output_published;
 uint32_t output_pages,output_words,operand_words,scale_words,reserved_output[7];
 uint32_t values[TT_Q38_GDN_OUTPUT_VALIDATION_WORDS];
 // 96*(322prepared+8192NEXT+64raw+64gated),3072packed operands,
 // 192FP32 scales,5120FP32 ssm_out values. ALLzero on input.
};
static_assert(offsetof(tt_q38_gdn_output_validation,values)==256,"fixed64word reply header,no padding");
static_assert(sizeof(tt_q38_gdn_output_validation)==3352320,"fixed pointer-free HEAP reply");
static_assert(unsigned(TT_Q38_GDN_OUTPUT_VALIDATION_OPERANDS)==unsigned(TT_Q38_GDN_RENDEZVOUS_VALIDATION_WORDS)
 && TT_Q38_GDN_OUTPUT_VALIDATION_SCALES-TT_Q38_GDN_OUTPUT_VALIDATION_OPERANDS==3072
 && TT_Q38_GDN_OUTPUT_VALIDATION_RESULT-TT_Q38_GDN_OUTPUT_VALIDATION_SCALES==192
 && TT_Q38_GDN_OUTPUT_VALIDATION_WORDS-TT_Q38_GDN_OUTPUT_VALIDATION_RESULT==5120,"disjoint fixed numeric sections");
#endif
