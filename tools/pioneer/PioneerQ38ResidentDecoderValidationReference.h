/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_DECODER_VALIDATION_REFERENCE_H
#define PIONEER_Q38_RESIDENT_DECODER_VALIDATION_REFERENCE_H
#if defined(PIONEER_Q38_GDN_RENDEZVOUS_NATIVE) || defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_GATED_NATIVE)
#error Recurrent native DEBUG never compiles CPU references
#endif
#if (defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT)+defined(PIONEER_Q38_GDN_INPUT_VALIDATION_CLIENT)+defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_CLIENT)) != 1 || (defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT) && PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT != 1) || (defined(PIONEER_Q38_GDN_INPUT_VALIDATION_CLIENT) && PIONEER_Q38_GDN_INPUT_VALIDATION_CLIENT != 1) || (defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_CLIENT) && PIONEER_Q38_GDN_RECURRENT_VALIDATION_CLIENT != 1) || !defined(PIONEER_Q38_CPU_VALIDATION) || PIONEER_Q38_CPU_VALIDATION != 1 || !defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE) || PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE != 1 || defined(PIONEER_Q38_PROJECTION_VALIDATION_CLIENT) || defined(PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT) || defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_CLIENT) || defined(PIONEER_Q38_EMBEDDING_CLIENT) || defined(PIONEER_Q38_RESIDENT_DECODER_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE) || defined(PIONEER_Q38_EMBEDDING_NATIVE) || defined(PIONEER_Q38_GDN_INPUT_OWNER) || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) || defined(_KERNEL_MODE)
#error Independent original Conv4 reference is separate DEBUG ONLY,never Work/native/kernel
#endif
#include "PioneerQ38ProjectionValidationReference.h"
// BEFORE accelerator FD,using ORIGINAL F32 weights and independently
// computed ORIGINAL-GGUF QKV. Position0 ONLY: canonical previous samples
// must all be +0. No caller/card history or result may seed the oracle.
// Reply layout is10240 SiLU outputs +30720 private next-history words.
template<class Source>bool PioneerQ38ResidentDecoderReference(Source& source,const uint32_t* qkv,unsigned inputs,uint32_t* output,unsigned capacity){
 constexpr unsigned channels=10240,words=40960;
 if(!qkv || !output || inputs!=channels || capacity!=words || !source.Ready())return false;
 uintptr_t in=reinterpret_cast<uintptr_t>(qkv),out=reinterpret_cast<uintptr_t>(output);
 if((in&3) || (out&3) || in>UINTPTR_MAX-channels*4 || out>UINTPTR_MAX-words*4 || (in<out+words*4 && out<in+channels*4))return false;
 const auto* t=source.Model().Find("blk.0.ssm_conv1d.weight");
 if(!t || t->type || t->rank!=2 || t->inputs!=4 || t->outputs!=channels || t->bytes!=channels*16)return false;
 unsigned index=unsigned(t-source.Model().Tensors().data());
 for(unsigned c=0;c<channels;c++)if((qkv[c]&0x7fffffff)>=0x7f800000)return false;
 std::vector<uint32_t> staged(words,0);uint8_t packed[256];
 for(unsigned first=0;first<channels;first+=16){
  if(!source.ReadReferencePacked(index,uint64_t(first)*16,256,packed,256) || !source.Ready())return false;
  for(unsigned c=0;c<16;c++){
   float sum=0;
   for(unsigned tap=0;tap<4;tap++){
    const auto* b=packed+(c*4+tap)*4;uint32_t weight=uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);
    if((weight&0x7fffffff)>=0x7f800000)return false;
    sum+=(tap==3?PioneerAttentionFloat(qkv[first+c]):0.f)*PioneerAttentionFloat(weight);
   }
   // Independent double SiLU oracle;card approximation compared with the
   // previously qualified3e-6*(1+abs(reference)) bound,NOT bit identity.
   double expected=double(sum)/(1+exp(-double(sum)));float value=float(expected);
   if(!isfinite(expected) || !isfinite(value))return false;
   staged[first+c]=PioneerAttentionBits(value);staged[channels+(first+c)*3+2]=qkv[first+c];
  }
 }
 if(!source.Ready())return false;
 memcpy(output,staged.data(),words*4);return true; // atomic publish after ALL original coefficients/input/source checks
}
#endif
