/* SPDX-License-Identifier: MIT */
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
#ifndef PIONEER_Q38_GDN_INPUT_VALIDATION_REFERENCE_H
#define PIONEER_Q38_GDN_INPUT_VALIDATION_REFERENCE_H
#ifdef PIONEER_Q38_GDN_OUTPUT_INPUT_IMAGE
#error Offline gated input image never compiles CPU references
#endif
#if defined(PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_NATIVE) || defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_GATED_NATIVE)
#error Recurrent native DEBUG never compiles CPU references
#endif
#if !defined(PIONEER_Q38_GDN_INPUT_VALIDATION_CLIENT) || PIONEER_Q38_GDN_INPUT_VALIDATION_CLIENT != 1 || !defined(PIONEER_Q38_CPU_VALIDATION) || PIONEER_Q38_CPU_VALIDATION != 1 || !defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE) || PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE != 1 || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT) || defined(PIONEER_Q38_PROJECTION_VALIDATION_CLIENT) || defined(PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT) || defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_CLIENT) || defined(PIONEER_Q38_EMBEDDING_CLIENT) || defined(PIONEER_Q38_GDN_INPUT_OWNER) || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE) || defined(PIONEER_Q38_EMBEDDING_NATIVE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) || defined(_KERNEL_MODE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_CLIENT)
#error Independent original GDN-input reference is exact separate DEBUG ONLY,never native/Work/kernel
#endif
#include "PioneerQ38ResidentDecoderValidationReference.h"
// Independent libm oracle BEFORE GPU FD. Original CPU conv/alpha/beta
// products only,NEVER card outputs or the card's arithmetic primitives.
// ssm_a is ALREADY negative-exp(A_log),not an A_log to exponentiate again.
template<class Source>bool PioneerQ38GdnInputReference(Source& source,const uint32_t* conv,unsigned convWords,
 const uint32_t* alpha,const uint32_t* beta,unsigned heads,uint32_t* output,unsigned words){
 if(!conv || !alpha || !beta || !output || convWords!=40960 || heads!=48 || words!=30912 || !source.Ready())return false;
 uintptr_t out=reinterpret_cast<uintptr_t>(output);if((out&3) || out>UINTPTR_MAX-123648)return false;
 const uint32_t* inputs[]={conv,alpha,beta};const unsigned bytes[]={163840,192,192};
 for(unsigned i=0;i<3;i++){
  uintptr_t in=reinterpret_cast<uintptr_t>(inputs[i]);
  if((in&3) || in>UINTPTR_MAX-bytes[i] || (in<out+123648 && out<in+bytes[i]))return false;
 }
 uint32_t learned[2][48];uint8_t packed[192];
 for(unsigned role=0;role<2;role++){
  const auto* t=source.Model().Find(role?"blk.0.ssm_dt.bias":"blk.0.ssm_a");
  if(!t || t->type || t->rank!=1 || t->inputs!=48 || t->outputs!=1 || t->bytes!=192)return false;
  unsigned index=unsigned(t-source.Model().Tensors().data());
  if(!source.ReadReferencePacked(index,0,192,packed,192) || !source.Ready())return false;
  for(unsigned h=0;h<48;h++){
   const auto* b=packed+h*4;learned[role][h]=uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);
   uint32_t m=learned[role][h]&0x7fffffff;
   if((m && m<0x00800000) || m>0x477fe000 || (!role && !(PioneerAttentionFloat(learned[role][h])<0)))return false;
  }
 }
 std::vector<uint32_t> staged(30912);std::vector<float> q(2048),k(2048);
 for(unsigned h=0;h<16;h++){
  float qs=0,ks=0;
  for(unsigned i=0;i<128;i++){
   float a=PioneerAttentionFloat(conv[h*128+i]),b=PioneerAttentionFloat(conv[2048+h*128+i]);
   if(!isfinite(a) || !isfinite(b))return false;
   qs+=a*a;ks+=b*b;
  }
  float qi=(1.f/sqrtf(qs+1e-6f))*0.0883883461356163f,ki=1.f/sqrtf(ks+1e-6f);
  if(!isfinite(qi) || !isfinite(ki) || !(qi>0) || !(ki>0))return false;
  for(unsigned i=0;i<128;i++){q[h*128+i]=PioneerAttentionFloat(conv[h*128+i])*qi;k[h*128+i]=PioneerAttentionFloat(conv[2048+h*128+i])*ki;}
 }
 for(unsigned half=0;half<96;half++){
  unsigned h=half>>1,key=h%16;auto* values=staged.data()+half*322;
  if((alpha[h]&0x7fffffff)>0x477fe000 || (beta[h]&0x7fffffff)>0x477fe000)return false;
  for(unsigned i=0;i<128;i++){values[i]=PioneerAttentionBits(q[key*128+i]);values[128+i]=PioneerAttentionBits(k[key*128+i]);}
  for(unsigned i=0;i<64;i++)values[256+i]=conv[4096+h*128+(half&1)*64+i];
  float shifted=PioneerAttentionFloat(alpha[h])+PioneerAttentionFloat(learned[1][h]);double x=shifted;
  float softplus=float(x>0?x+log1p(exp(-x)):log1p(exp(x)));
  float exponent=PioneerAttentionFloat(learned[0][h])*softplus;
  double logit=PioneerAttentionFloat(beta[h]),e=exp(-fabs(logit));
  values[320]=PioneerAttentionBits(float(exp(double(exponent))));values[321]=PioneerAttentionBits(float(logit>=0?1/(1+e):e/(1+e)));
  for(unsigned i=0;i<322;i++)if((values[i]&0x7fffffff)>=0x7f800000)return false;
  for(unsigned i=320;i<322;i++)if((values[i]&0x7fffffff)<0x00800000)values[i]=0; // explicit actual-subnormal gate FTZ ONLY
 }
 if(!source.Ready())return false;
 memcpy(output,staged.data(),123648);return true; // atomic independent reference,never uploaded
}
#endif
