/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_RECURRENT_VALIDATION_REFERENCE_H
#define PIONEER_Q38_GDN_RECURRENT_VALIDATION_REFERENCE_H
#if defined(PIONEER_Q38_GDN_GATED_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_NATIVE)
#error Native fused QGH1 never compiles CPU references
#endif
#if !defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_CLIENT) || PIONEER_Q38_GDN_RECURRENT_VALIDATION_CLIENT != 1 || !defined(PIONEER_Q38_CPU_VALIDATION) || PIONEER_Q38_CPU_VALIDATION != 1 || !defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE) || PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE != 1 || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_CLIENT) || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT) || defined(PIONEER_Q38_PROJECTION_VALIDATION_CLIENT) || defined(PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT) || defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_CLIENT) || defined(PIONEER_Q38_EMBEDDING_CLIENT) || defined(PIONEER_Q38_GDN_INPUT_OWNER) || defined(PIONEER_Q38_GDN_STATE_INIT_NATIVE) || defined(PIONEER_Q38_GDN_RECURRENT_NATIVE) || defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_NATIVE) || defined(PIONEER_Q38_EMBEDDING_NATIVE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) || defined(_KERNEL_MODE)
#error Independent original recurrent reference requires separate exact CPU DEBUG client,never native/Work/kernel
#endif
#include "PioneerQ38ResidentDecoderValidationReference.h"
// Independent libm/FP32 oracle,BEFORE FD. ORIGINAL CPU Conv/alpha/beta
// references only. Canonical position0 has an allzero CURRENT state;no card
// outputs/current history/numerical uploads can seed this reference.
// Q is UNSCALED;apply1/sqrt(128) ONCE after output reduction,not twice.
template<class Source>bool PioneerQ38GdnRecurrentReference(Source& source,const uint32_t* conv,unsigned convWords,
 const uint32_t* alpha,const uint32_t* beta,unsigned heads,uint32_t* output,unsigned words){
 constexpr unsigned halfWords=8578,total=823488;
 if(!source.Ready() || !conv || !alpha || !beta || !output || convWords!=40960 || heads!=48 || words!=total)return false;
 uintptr_t out=reinterpret_cast<uintptr_t>(output);if((out&3) || out>UINTPTR_MAX-total*4)return false;
 const uint32_t* inputs[]={conv,alpha,beta};const unsigned sizes[]={163840,192,192};
 for(unsigned i=0;i<3;i++){
  uintptr_t in=reinterpret_cast<uintptr_t>(inputs[i]);
  if((in&3) || in>UINTPTR_MAX-sizes[i] || (in<out+total*4 && out<in+sizes[i]))return false;
 }
 uint32_t learned[2][48];uint8_t bytes[192];
 for(unsigned role=0;role<2;role++){
  const auto* t=source.Model().Find(role?"blk.0.ssm_dt.bias":"blk.0.ssm_a");
  if(!t || t->type || t->rank!=1 || t->inputs!=48 || t->outputs!=1 || t->bytes!=192)return false;
  unsigned index=unsigned(t-source.Model().Tensors().data());
  if(!source.ReadReferencePacked(index,0,192,bytes,192) || !source.Ready())return false;
  for(unsigned h=0;h<48;h++){
   const auto* b=bytes+h*4;learned[role][h]=uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);
   uint32_t magnitude=learned[role][h]&0x7fffffff;
   if((magnitude && magnitude<0x00800000) || magnitude>0x477fe000 || (!role && !(PioneerAttentionFloat(learned[role][h])<0)))return false;
  }
 }
 for(unsigned i=0;i<40960;i++)if((conv[i]&0x7fffffff)>=0x7f800000)return false;
 std::vector<uint32_t> staged(total);std::vector<float> q(2048),k(2048);
 for(unsigned head=0;head<16;head++){
  float qs=0,ks=0;
  for(unsigned i=0;i<128;i++){
   float a=PioneerAttentionFloat(conv[head*128+i]),b=PioneerAttentionFloat(conv[2048+head*128+i]);qs+=a*a;ks+=b*b;
  }
  float qi=1.f/sqrtf(qs+1e-6f),ki=1.f/sqrtf(ks+1e-6f);
  if(!isfinite(qi) || !isfinite(ki) || !(qi>0) || !(ki>0))return false;
  for(unsigned i=0;i<128;i++){q[head*128+i]=PioneerAttentionFloat(conv[head*128+i])*qi;k[head*128+i]=PioneerAttentionFloat(conv[2048+head*128+i])*ki;}
 }
 for(unsigned half=0;half<96;half++){
  unsigned head=half>>1,key=head%16;auto* expected=staged.data()+half*halfWords;
  if((alpha[head]&0x7fffffff)>0x477fe000 || (beta[head]&0x7fffffff)>0x477fe000)return false;
  for(unsigned i=0;i<128;i++){expected[i]=PioneerAttentionBits(q[key*128+i]);expected[128+i]=PioneerAttentionBits(k[key*128+i]);}
  for(unsigned i=0;i<64;i++)expected[256+i]=conv[4096+head*128+(half&1)*64+i];
  float shifted=PioneerAttentionFloat(alpha[head])+PioneerAttentionFloat(learned[1][head]);double x=shifted;
  float softplus=float(x>0?x+log1p(exp(-x)):log1p(exp(x))),exponent=PioneerAttentionFloat(learned[0][head])*softplus;
  double logit=PioneerAttentionFloat(beta[head]),e=exp(-fabs(logit));
  expected[320]=PioneerAttentionBits(float(exp(double(exponent))));expected[321]=PioneerAttentionBits(float(logit>=0?1/(1+e):e/(1+e)));
  for(unsigned i=320;i<322;i++)if((expected[i]&0x7fffffff)<0x00800000)expected[i]=0; // true-subnormal FTZ only
  float gate=PioneerAttentionFloat(expected[321]);
  for(unsigned value=0;value<64;value++){
   float delta=PioneerAttentionFloat(expected[256+value])*gate,sum=0;
   for(unsigned i=0;i<128;i++){
    float next=PioneerAttentionFloat(expected[128+i])*delta;expected[322+i*64+value]=PioneerAttentionBits(next);
    sum+=next*PioneerAttentionFloat(expected[i]);
   }
   expected[8514+value]=PioneerAttentionBits(sum*0.0883883461356163f);
  }
  for(unsigned i=0;i<halfWords;i++)if((expected[i]&0x7fffffff)>=0x7f800000)return false;
 }
 if(!source.Ready())return false;
 memcpy(output,staged.data(),total*4);return true; // atomic independent result,NEVER uploaded
}
#endif
