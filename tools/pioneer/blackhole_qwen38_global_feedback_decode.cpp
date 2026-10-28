/* SPDX-License-Identifier: MIT */
#include "PioneerQ38GlobalFeedbackComponentRole.h"
// Reuse the exact original Q4_K decoder. Linker drops the old descriptor/
// packet/main functions; this capsule performs NO NoC or publication itself.
#define scalar_main q38_feedback_unbound_embedding_main
#include "blackhole_qwen38_embedding.cpp"
#undef scalar_main
extern "C" bool q38_global_chain_feedback_decode(unsigned block,unsigned lane){
 return block<20 && lane<16 && Embedding::Decode(block,lane);
}
