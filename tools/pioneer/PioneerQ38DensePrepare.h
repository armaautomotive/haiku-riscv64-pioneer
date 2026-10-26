/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DENSE_PREPARE_H
#define PIONEER_Q38_DENSE_PREPARE_H
#include "PioneerQ38LearnedRms.h"
#include "PioneerGraphFma.h"
// Fixed27B dense-attention LOCAL math. Six interleaved [Q256,gate256]
// heads share one K/V head. GGUF learned norm weights already include1.
// The32 cos/sin pairs rotate ONLY dimensions0..63;64..255 pass unchanged.
// A future owner owes an immutable8192-position table uploaded at INIT,
// position/source binding and actual input production. No per-token CPU
// arithmetic, native/Work registration, NoC, KV commit or ready flag here.
namespace PioneerQ38DensePrepare {
enum {QueryHeads=6,Width=256,Rotary=64,QgWords=3072,QueryWords=1536};
static inline bool Mean256(const PioneerQ38RmsSum& sum,float& mean){
 if(!(sum.high|sum.low)){mean=0;return true;}
 int e=sum.exponent-8;
 if(e< -126 || e>127 || sum.high<0x00100000 || sum.high>=0x00200000)return false;
 uint32_t mantissa=(sum.high<<3)|(sum.low>>29),tail=sum.low&0x1fffffff;
 if(tail>0x10000000 || (tail==0x10000000 && (mantissa&1)))mantissa++;
 if(mantissa==0x01000000){mantissa>>=1;e++;}
 if(e>127)return false;
 mean=PioneerAttentionFloat((uint32_t(e+127)<<23)|(mantissa&0x7fffff));return true;
}
static inline bool Head(const float* input,const float* weight,const float* rope,float* scratch){
 PioneerQ38RmsSum sum;
 for(unsigned i=0;i<Width;i++)if(!sum.Add(input[i]*input[i]))return false;
 float mean;if(!Mean256(sum,mean))return false;
 float scale=PioneerGraphDividePositive(1,PioneerGraphSqrtPositive(mean+1e-6f));
 if(!(scale>0) || !PioneerQ38RmsDomain(scale))return false;
 for(unsigned i=0;i<Width;i++){
  scratch[i]=(input[i]*scale)*weight[i];
  if(!PioneerQ38RmsDomain(scratch[i]))return false;
 }
 for(unsigned i=0;i<Rotary/2;i++){
  float a=scratch[i],b=scratch[i+Rotary/2],c=rope[2*i],s=rope[2*i+1];
  float x=PioneerGraphFma(a,c,-(b*s)),y=PioneerGraphFma(a,s,b*c);
  if((PioneerAttentionBits(x)&0x7fffffff)>0x477fe000 || (PioneerAttentionBits(y)&0x7fffffff)>0x477fe000)return false;
  scratch[i]=x;scratch[i+Rotary/2]=y;
 }
 return true;
}
static __attribute__((noinline)) bool Run(const float* qg,const float* key,const float* value,
 const float* qWeight,const float* kWeight,const float* rope,float* query,float* gate,uint32_t* packedK,uint32_t* packedV){
 (void)&PioneerQ38LearnedRms; // only its software-binary64 SUM is reused
 using Address=__UINTPTR_TYPE__;
 const Address address[]={reinterpret_cast<Address>(qg),reinterpret_cast<Address>(key),reinterpret_cast<Address>(value),
  reinterpret_cast<Address>(qWeight),reinterpret_cast<Address>(kWeight),reinterpret_cast<Address>(rope),
  reinterpret_cast<Address>(query),reinterpret_cast<Address>(gate),reinterpret_cast<Address>(packedK),reinterpret_cast<Address>(packedV)};
 const unsigned bytes[]={12288,1024,1024,1024,1024,256,6144,6144,512,512};
 for(unsigned i=0;i<10;i++){
  if(!address[i] || (address[i]&3) || address[i]>~Address(0)-bytes[i])return false;
  for(unsigned j=0;j<i;j++)if(address[i]<address[j]+bytes[j] && address[j]<address[i]+bytes[i])return false;
 }
 // Reject raw domain failures BEFORE any output write. Arithmetic failures
 // may leave LOCAL scratch partial;false never grants a GDDR publication.
 for(unsigned i=0;i<QgWords;i++)if(!PioneerQ38RmsDomain(qg[i]))return false;
 for(unsigned i=0;i<Width;i++)if(!PioneerQ38RmsDomain(key[i]) || !PioneerQ38RmsDomain(value[i])
  || !PioneerQ38RmsDomain(qWeight[i]) || !PioneerQ38RmsDomain(kWeight[i]))return false;
 for(unsigned i=0;i<Rotary;i++){
  unsigned a=PioneerAttentionBits(rope[i])&0x7fffffff;
  if(a>0x3f800000 || (a && a<0x26800000))return false;
 }
 float scratch[Width];
 for(unsigned h=0;h<QueryHeads;h++){
  if(!Head(qg+h*512,qWeight,rope,scratch))return false;
  for(unsigned i=0;i<Width;i++){query[h*Width+i]=scratch[i];gate[h*Width+i]=qg[h*512+Width+i];}
 }
 if(!Head(key,kWeight,rope,scratch))return false;
 for(unsigned i=0;i<Width/2;i++){
  packedK[i]=PioneerAttentionToHalf(scratch[2*i])|(uint32_t(PioneerAttentionToHalf(scratch[2*i+1]))<<16);
  packedV[i]=PioneerAttentionToHalf(value[2*i])|(uint32_t(PioneerAttentionToHalf(value[2*i+1]))<<16);
 }
 return true;
}
}
#endif
