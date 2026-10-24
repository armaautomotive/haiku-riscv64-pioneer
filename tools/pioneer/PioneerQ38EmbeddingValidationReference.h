/* SPDX-License-Identifier: MIT */
#if defined(PIONEER_Q38_NEXT_PROJECTION_NATIVE)
#error SAME native next-layer projections never include CPU reference code
#endif
#if defined(PIONEER_Q38_PROJECTION_NEXT_ROOT)
#error QPS2 device images never include independent CPU reference code
#endif
#if defined(PIONEER_Q38_NEXT_RMS_NATIVE)
#error SAME native next-layer RMS never includes independent CPU reference code
#endif
#if defined(PIONEER_Q38_FFN_RESIDUAL_NATIVE)
#error SAME native FFN execution must NEVER include independent CPU reference code
#endif
#ifdef PIONEER_Q38_RESIDUAL_RMS_NATIVE
#error Native residual RMS never compiles CPU reference observers
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE
#error Native output DEBUG never compiles CPU reference observers
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE
#error Native output execution never compiles CPU reference observers
#endif
#ifndef PIONEER_Q38_EMBEDDING_VALIDATION_REFERENCE_H
#define PIONEER_Q38_EMBEDDING_VALIDATION_REFERENCE_H
#ifdef PIONEER_Q38_GDN_OUTPUT_INPUT_IMAGE
#error Offline gated input image never compiles CPU references
#endif
#if defined(PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_NATIVE) || defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_GATED_NATIVE)
#error Recurrent native DEBUG never compiles CPU references
#endif
#if (defined(PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT)+defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_CLIENT)+defined(PIONEER_Q38_PROJECTION_VALIDATION_CLIENT)+defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT)+defined(PIONEER_Q38_GDN_INPUT_VALIDATION_CLIENT)+defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_CLIENT)+defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_CLIENT)+defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_CLIENT)) != 1 || (defined(PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT) && PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT != 1) || (defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_CLIENT) && PIONEER_Q38_LEARNED_RMS_VALIDATION_CLIENT != 1) || (defined(PIONEER_Q38_PROJECTION_VALIDATION_CLIENT) && PIONEER_Q38_PROJECTION_VALIDATION_CLIENT != 1) || (defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT) && PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT != 1) || (defined(PIONEER_Q38_GDN_INPUT_VALIDATION_CLIENT) && PIONEER_Q38_GDN_INPUT_VALIDATION_CLIENT != 1) || (defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_CLIENT) && PIONEER_Q38_GDN_RECURRENT_VALIDATION_CLIENT != 1) || (defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_CLIENT) && PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_CLIENT != 1) || (defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_CLIENT) && PIONEER_Q38_GDN_OUTPUT_VALIDATION_CLIENT != 1) || !defined(PIONEER_Q38_CPU_VALIDATION) || PIONEER_Q38_CPU_VALIDATION != 1 || defined(PIONEER_Q38_EMBEDDING_CLIENT) || defined(PIONEER_Q38_GDN_INPUT_OWNER) || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(_KERNEL_MODE)
#error Independent embedding reference is DEBUG ONLY,never Work
#endif
#include "TenstorrentQ4K.h"
#include <vector>
// Same authenticated original GGUF row,BEFORE GPU FD. No tiled/card output
// or receipt can seed/replace this independent numeric reference.
template<class Source>bool PioneerQ38EmbeddingReference(Source& source,unsigned tensor,unsigned token,uint32_t* output){
 if(!output || !source.Ready() || tensor>=source.Model().Tensors().size())return false;
 const auto& t=source.Model().Tensors()[tensor];
 if(t.name!="token_embd.weight" || t.type!=12 || t.inputs!=5120 || t.outputs!=248320 || token>=248320)return false;
 std::vector<uint8_t> packed(2880);
 if(!source.ReadReferencePacked(tensor,uint64_t(token)*2880,2880,packed.data(),2880) || !source.Ready())return false;
 float decoded[256];
 for(unsigned block=0;block<20;block++){
  if(!PioneerDecodeQ4K(packed.data()+block*144,decoded))return false;
  for(unsigned i=0;i<256;i++){uint32_t bits;memcpy(&bits,&decoded[i],4);if((bits&0x7fffffff)>=0x7f800000)return false;output[block*256+i]=bits;}
 }
 return source.Ready();
}
#endif
