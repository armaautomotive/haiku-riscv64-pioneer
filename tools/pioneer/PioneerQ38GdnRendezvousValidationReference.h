/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_RESIDENT_OPERATOR_IMAGE
#error Resident operator IMAGE never contains CPU reference observers
#endif
#ifdef PIONEER_Q38_RESIDENT_SILU_PRECISION_NATIVE
#error Native66 never contains CPU reference observers
#endif
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
#error Native layer1 FFN never contains CPU reference observers
#endif
#ifdef PIONEER_Q38_CONV_SILU_PRECISION_NATIVE
#error Native precision diagnostic never contains CPU reference observers
#endif
#if defined(PIONEER_Q38_SILU_PRECISION_IMAGE) || defined(PIONEER_Q38_SILU_LEGACY_CONTROL_IMAGE)
#error Isolated SiLU diagnostic images never include CPU reference observers
#endif
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE
#error SAME native63 residual RMS never includes CPU reference code
#endif
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
#ifndef PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_REFERENCE_H
#define PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_REFERENCE_H
#ifdef PIONEER_Q38_GDN_OUTPUT_INPUT_IMAGE
#error Offline gated input image never compiles CPU references
#endif
#if (defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_CLIENT)+defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_CLIENT)) != 1 || (defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_CLIENT) && PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_CLIENT != 1) || (defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_CLIENT) && PIONEER_Q38_GDN_OUTPUT_VALIDATION_CLIENT != 1) || defined(PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_NATIVE) || defined(PIONEER_Q38_GDN_GATED_NATIVE) || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE)
#error Exact separate CPU DEBUG52 observer required,never kernel/native/Work
#endif
#include "PioneerQ38GdnRecurrentValidationReference.h"
// Independent original FP32/libm reference,computed BEFORE FD. No card
// arithmetic helper,card products,ready flag or numeric uploads may enter.
template<class Source>bool PioneerQ38GdnRendezvousReference(Source& source,const uint32_t* conv,unsigned convWords,
 const uint32_t* alpha,const uint32_t* beta,unsigned heads,const uint32_t* z,unsigned zWords,uint32_t* output,unsigned words){
 constexpr unsigned halfWords=8642,total=829632;
 if(!source.Ready() || !conv || !alpha || !beta || !z || !output || convWords!=40960 || heads!=48 || zWords!=6144 || words!=total)return false;
 uintptr_t out=reinterpret_cast<uintptr_t>(output);
 if((out&3) || out>UINTPTR_MAX-total*4)return false;
 const uint32_t* inputs[]={conv,alpha,beta,z};const unsigned sizes[]={163840,192,192,24576};
 for(unsigned i=0;i<4;i++){
  uintptr_t in=reinterpret_cast<uintptr_t>(inputs[i]);
  if((in&3) || in>UINTPTR_MAX-sizes[i] || (in<out+total*4 && out<in+sizes[i]))return false;
 }
 for(unsigned i=0;i<6144;i++)if((z[i]&0x7fffffff)>0x477fe000)return false;
 const auto* tensor=source.Model().Find("blk.0.ssm_norm.weight");
 if(!tensor || tensor->type || tensor->rank!=1 || tensor->inputs!=128 || tensor->outputs!=1 || tensor->bytes!=512)return false;
 unsigned index=unsigned(tensor-source.Model().Tensors().data());uint8_t bytes[512];uint32_t norm[128];
 if(!source.ReadReferencePacked(index,0,512,bytes,512) || !source.Ready())return false;
 for(unsigned i=0;i<128;i++){
  const auto* b=bytes+i*4;norm[i]=uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);
  if((norm[i]&0x7fffffff)>0x477fe000)return false;
 }
 std::vector<uint32_t> recurrent(823488),staged(total);
 if(!PioneerQ38GdnRecurrentReference(source,conv,convWords,alpha,beta,heads,recurrent.data(),823488))return false;
 for(unsigned half=0;half<96;half++)memcpy(staged.data()+half*halfWords,recurrent.data()+half*8578,8578*4);
 for(unsigned head=0;head<48;head++){
  float raw[128],sum=0;
  for(unsigned i=0;i<128;i++){raw[i]=PioneerAttentionFloat(recurrent[(head*2+i/64)*8578+8514+i%64]);sum+=raw[i]*raw[i];}
  float inverse=1.f/sqrtf(sum*.0078125f+1e-6f);if(!isfinite(inverse) || !(inverse>0))return false;
  for(unsigned i=0;i<128;i++){
   float weighted=(raw[i]*inverse)*PioneerAttentionFloat(norm[i]);double gate=PioneerAttentionFloat(z[head*128+i]);
   double e=exp(-fabs(gate)),silu=gate*(gate>=0?1/(1+e):e/(1+e));
   float value=float(double(weighted)*silu);if(!isfinite(value))return false;
   uint32_t bits=PioneerAttentionBits(value);if((bits&0x7fffffff)<0x00800000)bits&=0x80000000;
   staged[(head*2+i/64)*halfWords+8578+i%64]=bits;
  }
 }
 if(!source.Ready())return false;
 memcpy(output,staged.data(),total*4);return true; // atomic;observer only,NEVER uploaded
}
#endif
