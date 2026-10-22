/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_LEARNED_RMS_H
#define PIONEER_Q38_LEARNED_RMS_H
#include "PioneerGraphNorm.h"
// Card-side fixed5120 hidden RMS. Reproduce each IEEE binary64 positive
// addition and division by5120 using ONLY paired32-bit integer arithmetic.
// No hardware double/fdiv/fsqrt,integer64 division or CPU fallback.
// Geometry/math DATA only;never a source/producer/native execution grant.
struct PioneerQ38RmsSum {
 uint32_t high=0,low=0;int exponent=0; // normalized53-bit significand,unbiased exponent
 static void RightJam(uint32_t& hi,uint32_t& lo,unsigned shift){
  if(!shift)return;
  if(shift<32){lo=(lo>>shift)|(hi<<(32-shift))|unsigned((lo<<(32-shift))!=0);hi>>=shift;}
  else if(shift==32){lo=hi|unsigned(lo!=0);hi=0;}
  else if(shift<64){lo=(hi>>(shift-32))|unsigned(lo!=0 || (hi<<(64-shift))!=0);hi=0;}
  else {lo=unsigned((hi|lo)!=0);hi=0;}
 }
 bool Add(float term){
  uint32_t bits=PioneerAttentionBits(term);
  if(bits&0x80000000U || bits>=0x7f800000 || (bits && bits<0x00800000))return false;
  if(!bits)return true;
  unsigned mantissa=(bits&0x7fffff)|0x800000;int e=int(bits>>23)-127;
  if(!(high|low)){high=mantissa>>3;low=mantissa<<29;exponent=e;return true;}
  uint32_t ah=(high<<3)|(low>>29),al=low<<3,bh=mantissa,bl=0;
  if(e>exponent){RightJam(ah,al,unsigned(e-exponent));exponent=e;}else RightJam(bh,bl,unsigned(exponent-e));
  uint32_t old=al;al+=bl;ah+=bh+unsigned(al<old);
  if(ah&0x01000000){RightJam(ah,al,1);exponent++;}
  unsigned round=al&7;low=(al>>3)|(ah<<29);high=ah>>3;
  if(round>4 || (round==4 && (low&1))){if(++low==0)high++;}
  if(high==0x00200000){low=(low>>1)|(high<<31);high>>=1;exponent++;}
  return true;
 }
 bool Mean5120(float& mean)const{
  if(!(high|low)){mean=0;return true;}
  // sum/5120 = sum/5 *2^-10. Normalize first,then exact bit long
  // division by5 and one RNE to53bits,matching CPU binary64 division.
  unsigned shift=high>=0x00140000?2:3;
  uint32_t nh=(high<<shift)|(low>>(32-shift)),nl=low<<shift,qh=0,ql=0,remainder=0;
  for(int bit=55;bit>=0;bit--){
   remainder=(remainder<<1)|((bit>=32?nh>>unsigned(bit-32):nl>>unsigned(bit))&1);
   unsigned digit=remainder>=5;remainder-=digit*5;
   qh=(qh<<1)|(ql>>31);ql=(ql<<1)|digit;
  }
  if(remainder*2>5){if(++ql==0)qh++;} // odd divisor:no halfway remainder
  int e=exponent-int(shift)-10;
  if(qh==0x00200000){ql=(ql>>1)|(qh<<31);qh>>=1;e++;}
  if(e< -126 || e>127 || qh<0x00100000 || qh>=0x00200000)return false;
  uint32_t mantissa=(qh<<3)|(ql>>29),tail=ql&0x1fffffff;
  if(tail>0x10000000 || (tail==0x10000000 && (mantissa&1)))mantissa++;
  if(mantissa==0x01000000){mantissa>>=1;e++;}
  if(e>127)return false;
  mean=PioneerAttentionFloat((uint32_t(e+127)<<23)|(mantissa&0x7fffff));return true;
 }
};
static_assert(sizeof(PioneerQ38RmsSum)==12,"software binary64 reduction,NOT hardware double");
static inline bool PioneerQ38RmsDomain(float value){
 uint32_t a=PioneerAttentionBits(value)&0x7fffffff;return a<=0x477fe000 && (!a || a>=0x26800000);
}
static __attribute__((noinline)) bool PioneerQ38LearnedRms(const float* input,const float* weight,float* output){
 if(!input || !weight || !output)return false;
 using Address=__UINTPTR_TYPE__;Address a=reinterpret_cast<Address>(input),b=reinterpret_cast<Address>(weight),c=reinterpret_cast<Address>(output);
 if((a|b|c)&3 || a>~Address(0)-20480 || b>~Address(0)-20480 || c>~Address(0)-20480
  || (a<b+20480 && b<a+20480) || (a<c+20480 && c<a+20480) || (b<c+20480 && c<b+20480))return false;
 PioneerQ38RmsSum sum;
 for(unsigned i=0;i<5120;i++){
  if(!PioneerQ38RmsDomain(input[i]) || !PioneerQ38RmsDomain(weight[i]) || !sum.Add(input[i]*input[i]))return false;
 }
 float mean;if(!sum.Mean5120(mean))return false;
 float denominator=PioneerGraphSqrtPositive(mean+1e-6f),scale=PioneerGraphDividePositive(1,denominator);
 if(!(scale>0) || !PioneerQ38RmsDomain(scale))return false;
 // Bounded domain keeps BOTH rounded multiplies normal/zero. Partial
 // local scratch on refusal is not a resident GDDR producer publication.
 for(unsigned i=0;i<5120;i++){
  float normalized=input[i]*scale,value=normalized*weight[i];
  uint32_t bits=PioneerAttentionBits(value),magnitude=bits&0x7fffffff;
  if(magnitude>=0x7f800000 || (magnitude && magnitude<0x00800000))return false;
  output[i]=value;
 }
 return true;
}
#endif
