/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_IMAGE
#error QRR2 device image never contains a CPU reference observer
#endif
#if defined(PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE)
#error SAME native62 next-layer output never includes CPU reference code
#endif
#if defined(PIONEER_Q38_NEXT_GDN_NATIVE)
#error SAME native next-layer GDN never includes CPU reference code
#endif
#if defined(PIONEER_Q38_NEXT_CONV_NATIVE)
#error SAME native next convolution never includes CPU reference code
#endif
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
#ifndef PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE_H
#define PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE_H
#ifdef PIONEER_Q38_GDN_OUTPUT_INPUT_IMAGE
#error Offline gated input image never compiles CPU references
#endif
#if defined(PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_NATIVE) || defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_GATED_NATIVE)
#error Recurrent native DEBUG never compiles CPU references
#endif
#if !defined(PIONEER_Q38_CPU_VALIDATION) || PIONEER_Q38_CPU_VALIDATION != 1 || !defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE) || PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE != 1 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) || defined(_KERNEL_MODE)
#error Independent learned RMS reference is separate DEBUG ONLY,never Work/kernel
#endif
#include "PioneerAttentionMath.h"
#include <stdint.h>
#include <string.h>
#include <vector>
// Independent original-F32 learned weight read and host double reduction.
// No device output/input,private proof,driver,FD,ready claim or fallback.
// Caller supplies an ORIGINAL-GGUF CPU embedding reference,never GPU output.
// Source must be SAME full-SHA-authenticated FILE/epoch for the entire test.
template<class Source>bool PioneerQ38LearnedRmsReference(Source& source,unsigned tensor,const uint32_t* input,uint32_t* output){
 constexpr unsigned values=5120,bytes=values*4;
 if(!input || !output || !source.Ready() || tensor>=source.Model().Tensors().size())return false;
 uintptr_t in=reinterpret_cast<uintptr_t>(input),out=reinterpret_cast<uintptr_t>(output);
 if(in>UINTPTR_MAX-bytes || out>UINTPTR_MAX-bytes || (in<out+bytes && out<in+bytes))return false;
 const auto& t=source.Model().Tensors()[tensor];
 if(t.name!="blk.0.attn_norm.weight" || t.type || t.rank!=1 || t.inputs!=values || t.outputs!=1 || t.bytes!=bytes)return false;
 std::vector<uint8_t> packed(bytes);std::vector<uint32_t> weight(values),stage(values);
 if(!source.ReadReferencePacked(tensor,0,bytes,packed.data(),bytes) || !source.Ready())return false;
 auto domain=[](uint32_t bits){uint32_t m=bits&0x7fffffffU;return !m || (m>=0x26800000U && m<=0x477fe000U);};
 double sum=0;
 for(unsigned i=0;i<values;i++){
  const auto* p=packed.data()+i*4;weight[i]=uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);
  if(!domain(input[i]) || !domain(weight[i]))return false;
  float x=PioneerAttentionFloat(input[i]),square=x*x;sum+=double(square);
 }
 float scale=1.f/sqrtf(float(sum/values)+1e-6f);
 for(unsigned i=0;i<values;i++){
  float result=(PioneerAttentionFloat(input[i])*scale)*PioneerAttentionFloat(weight[i]);stage[i]=PioneerAttentionBits(result);
  if((stage[i]&0x7fffffffU)>=0x7f800000U)return false;
 }
 if(!source.Ready())return false;
 memcpy(output,stage.data(),bytes);return true; // publish ONLY after all checks
}
#endif
