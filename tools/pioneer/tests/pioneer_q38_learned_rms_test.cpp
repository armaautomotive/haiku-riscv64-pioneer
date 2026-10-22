/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38LearnedRms.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
static uint32_t randomState=0x89ab7431;
static uint32_t Next(){randomState^=randomState<<13;randomState^=randomState>>17;randomState^=randomState<<5;return randomState;}
static uint64_t Bits(double d){uint64_t b;memcpy(&b,&d,8);return b;}
int main(){
 unsigned adds=0,means=0,rows=0,refusals=0;
 for(unsigned trial=0;trial<2000;trial++){
  PioneerQ38RmsSum sum;double reference=0;
  for(unsigned i=0;i<5120;i++){
   uint32_t bits=i%31?((Next()%254+1)<<23)|(Next()&0x7fffff):0;
   float term=PioneerAttentionFloat(bits);assert(sum.Add(term));reference+=double(term);
   uint64_t actual=(sum.high|sum.low)?(uint64_t(sum.exponent+1023)<<52)|(uint64_t(sum.high&0xfffff)<<32)|sum.low:0;
   assert(actual==Bits(reference));adds++;
  }
 }
 std::vector<float> input(5120),weight(5120),output(5120,-37),before;
 for(unsigned trial=0;trial<2000;trial++){
  double reference=0;PioneerQ38RmsSum sum;
  for(unsigned i=0;i<5120;i++){
   unsigned kind=trial%8;
   input[i]=kind==0?float(int(Next()%200001)-100000)*.00013f:kind==1?PioneerAttentionFloat((Next()&0x807fffff)|((Next()%65+77)<<23))
    :kind==2?1:kind==3?-0.0f:kind==4?65504.0f:kind==5?PioneerAttentionFloat(0x26800000):float(int(Next()%200001)-100000)*.13f;
   weight[i]=trial%5?float(int(Next()%2001)-1000)*.0017f:1;
   float square=input[i]*input[i];assert(sum.Add(square));reference+=double(square);
  }
  float mean=0;assert(sum.Mean5120(mean) && PioneerAttentionBits(mean)==PioneerAttentionBits(float(reference/5120)));means++;
  float scale=1.0f/sqrtf(float(reference/5120)+1e-6f);
  assert(PioneerQ38LearnedRms(input.data(),weight.data(),output.data()));
  for(unsigned i=0;i<5120;i++){float expected=(input[i]*scale)*weight[i];assert(PioneerAttentionBits(output[i])==PioneerAttentionBits(expected));}rows++;
 }
 for(unsigned which:{0U,5119U})for(uint32_t bits:{1U,0x267fffffU,0x47800000U,0x7f800000U,0x7fc00000U})for(bool learned:{false,true}){
  std::fill(input.begin(),input.end(),1);std::fill(weight.begin(),weight.end(),1);std::fill(output.begin(),output.end(),-37);before=output;
  (learned?weight:input)[which]=PioneerAttentionFloat(bits);
  assert(!PioneerQ38LearnedRms(input.data(),weight.data(),output.data()) && output==before);refusals++;
 }
 assert(!PioneerQ38LearnedRms(input.data(),weight.data(),input.data()) && !PioneerQ38LearnedRms(input.data(),input.data(),output.data()));refusals+=2;
 printf("PASS fixed5120 learned RMS:software binary64 additions%u/means%u/rows%u/refusals%u BITEXACT independent double/sqrtf/scaled-weight reference;NO native IO/fallback\n",adds,means,rows,refusals);
}
