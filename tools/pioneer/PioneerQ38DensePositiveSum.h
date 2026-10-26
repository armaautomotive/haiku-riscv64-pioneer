/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DENSE_POSITIVE_SUM_H
#define PIONEER_Q38_DENSE_POSITIVE_SUM_H
#include "PioneerQ38LearnedRms.h"
// Root-side reduction of actual53-bit LOCAL records. Paired32-bit only;
// no hardware double/division,CPU reference or caller readiness authority.
struct PioneerQ38DensePositiveSum {
 uint32_t high=0,low=0;int exponent=0;
 static bool Record(uint32_t hi,uint32_t lo,int e){
  return !(hi|lo)?e==0:hi>=0x00100000 && hi<0x00200000 && e>=-126 && e<=13;
 }
 bool Add(uint32_t hi,uint32_t lo,int e){
  if(!Record(hi,lo,e))return false;
  if(!(hi|lo))return true;
  if(!(high|low)){high=hi;low=lo;exponent=e;return true;}
  uint32_t ah=(high<<3)|(low>>29),al=low<<3,bh=(hi<<3)|(lo>>29),bl=lo<<3;
  if(e>exponent){PioneerQ38RmsSum::RightJam(ah,al,unsigned(e-exponent));exponent=e;}
  else PioneerQ38RmsSum::RightJam(bh,bl,unsigned(exponent-e));
  uint32_t old=al;al+=bl;ah+=bh+unsigned(al<old);
  if(ah&0x01000000){PioneerQ38RmsSum::RightJam(ah,al,1);exponent++;}
  unsigned round=al&7;low=(al>>3)|(ah<<29);high=ah>>3;
  if(round>4 || (round==4 && (low&1))){if(++low==0)high++;}
  if(high==0x00200000){low=(low>>1)|(high<<31);high>>=1;exponent++;}
  return Record(high,low,exponent);
 }
 bool Inverse(float& value)const{
  // Global softmax denominator in[1,8192]. Normalize the numerator so
  // quotient is a24-bit F32 significand,then exact restoring division.
  if(!Record(high,low,exponent) || exponent<0 || exponent>13
   || (exponent==13 && (high!=0x00100000 || low)))return false;
  bool power=high==0x00100000 && !low;int e=-exponent-int(!power);
  unsigned top=power?75:76,q=0;uint32_t rh=0,rl=0;
  for(int bit=int(top);bit>=0;bit--){
   rh=(rh<<1)|(rl>>31);rl=(rl<<1)|unsigned(unsigned(bit)==top);
   bool digit=rh>high || (rh==high && rl>=low);
   if(digit){uint32_t old=rl;rl-=low;rh-=high+unsigned(old<low);}
   q=(q<<1)|unsigned(digit);
  }
  uint32_t th=(rh<<1)|(rl>>31),tl=rl<<1;
  if(th>high || (th==high && (tl>low || (tl==low && (q&1)))))q++;
  if(q==0x01000000){q>>=1;e++;}
  if(q<0x00800000 || q>=0x01000000 || e< -13 || e>0)return false;
  value=PioneerAttentionFloat((uint32_t(e+127)<<23)|(q&0x7fffff));return true;
 }
};
#endif
