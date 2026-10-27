/* SPDX-License-Identifier: MIT */
// Independent host libm observer; no original/native/device/Work authority.
#define PIONEER_Q38_SIGMOID_PRECISION_IMAGE 1
#include "../PioneerQ38SigmoidPrecision.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cfenv>
int main(){
 assert(std::fegetround()==FE_TONEAREST);unsigned cases=0,differences=0,maxUlp=0,refused=0;
 auto check=[&](float input){
  unsigned magnitude=PioneerAttentionBits(input)&0x7fffffff;assert(!magnitude || magnitude>=0x00800000);
  double e=std::exp(-std::fabs(double(input)));float expected=float(input>=0?1/(1+e):e/(1+e));
  unsigned wanted=PioneerAttentionBits(expected);if(!(wanted&0x7f800000))wanted=0;
  float output=PioneerAttentionFloat(0x7fc00001);assert(PioneerQ38SigmoidPrecision(input,output));unsigned actual=PioneerAttentionBits(output);
  assert(actual<=0x3f800000 && (!actual || actual>=0x00800000));
  unsigned distance=actual>wanted?actual-wanted:wanted-actual;
  if(distance>1)fprintf(stderr,"SIGMOID_PRECISION_FAILURE input%08x actual%08x expected%08x ulp%u\n",PioneerAttentionBits(input),actual,wanted,distance);
  assert(distance<=1);cases++;differences+=distance!=0;maxUlp=std::max(maxUlp,distance);
 };
 for(unsigned bits:{0U,0x80000000U,0x00800000U,0x80800000U,0x32800000U,0xb2800000U,0x7f7fffffU,0xff7fffffU})check(PioneerAttentionFloat(bits));
 for(int i=-106496;i<=106496;i++)check(float(i)*0x1p-10f);
 uint32_t random=0x51535031;
 for(unsigned i=0;i<100000;i++){
  random=random*1664525+1013904223;check(float(int(random%40000001)-20000000)*.000001f);
 }
 for(float boundary:{-104.f,-88.f,-87.3365478515625f,-80.f,-20.f,-16.63553237915039f,0x1p-26f,16.63553237915039f,20.f}){
  unsigned bits=PioneerAttentionBits(boundary);for(int d=-4;d<=4;d++)check(PioneerAttentionFloat(bits+unsigned(d)));
 }
 for(uint32_t bad:{1U,0x007fffffU,0x80000001U,0x807fffffU,0x7f800000U,0xff800000U,0x7fc00000U}){
  float output=PioneerAttentionFloat(0xdeadbeef);assert(!PioneerQ38SigmoidPrecision(PioneerAttentionFloat(bad),output)
   && PioneerAttentionBits(output)==0xdeadbeef);refused++;
 }
 printf("PASS isolated sigmoid compensated candidate HOST cases%u/differences%u/maxUlp%u/refusals%u/finalSigmoidFTZ/atomic output;NOT universal correct-rounding proof,emitted instructions,original-model,native,Work or TPS\n",cases,differences,maxUlp,refused);
}
