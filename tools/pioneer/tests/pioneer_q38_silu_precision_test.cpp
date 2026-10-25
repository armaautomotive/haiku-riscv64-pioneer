/* SPDX-License-Identifier: MIT */
// Independent host libm observer of the isolated card arithmetic candidate.
// NOT a native controller/fallback or emitted/physical arithmetic proof.
#define PIONEER_Q38_SILU_PRECISION_IMAGE 1
#include "../PioneerQ38SiluPrecision.h"
#include <cassert>
#include <cfenv>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <initializer_list>
static uint32_t Reference(float value){
 double x=value,e=exp(-fabs(x));uint32_t result=PioneerAttentionBits(float(x*(x>=0?1/(1+e):e/(1+e))));
 return (result&0x7fffffff)<0x00800000?result&0x80000000:result;
}
int main(){
 assert(fegetround()==FE_TONEAREST);unsigned cases=0,oldDifferent=0,newDifferent=0,oldMaximum=0,newMaximum=0;
 auto observe=[&](float value){
  float result=PioneerAttentionFloat(0x7fc12345);assert(PioneerQ38SiluPrecision(value,result));
  uint32_t expected=Reference(value),actual=PioneerAttentionBits(result),old=PioneerAttentionBits(PioneerGraphSilu(value));
  auto distance=[](uint32_t a,uint32_t b){assert((a>>31)==(b>>31));return a>b?a-b:b-a;};
  unsigned oldError=distance(old,expected),newError=distance(actual,expected);
  if(newError>1)printf("PRECISION_FAIL input%08x expected%08x actual%08x old%08x ulp%u\n",PioneerAttentionBits(value),expected,actual,old,newError);
  assert(newError<=1);oldDifferent+=old!=expected;newDifferent+=actual!=expected;
  oldMaximum=oldError>oldMaximum?oldError:oldMaximum;newMaximum=newError>newMaximum?newError:newMaximum;cases++;
 };
 for(int i=-524288;i<=524288;i++)observe(float(i)/4096.f);
 for(uint32_t bits=0x42b40000;bits<=0x42bc0000;bits++)observe(-PioneerAttentionFloat(bits)); // EVERY binary32 input around normal-output/FTZ boundary
 uint32_t random=0x6a09e667;
 for(unsigned i=0;i<32768;i++){
  random^=random<<13;random^=random>>17;random^=random<<5;
  uint32_t magnitude=0x00800000+random%(0x7f7fffff-0x00800000+1);
  observe(PioneerAttentionFloat(magnitude|(random&0x80000000)));
 }
 observe(0.f);observe(-0.f);observe(PioneerAttentionFloat(0x26800000));observe(-PioneerAttentionFloat(0x26800000));
 for(uint32_t edge:{0x00800000U,0x00ffffffU,0x01000000U,0x32800000U,0x41a00000U,0x42a00000U,0x42b00000U,0x42b80000U,0x42ba0000U,0x42d00000U,0x477fe000U,0x7f7fffffU})for(uint32_t sign:{0U,0x80000000U})observe(PioneerAttentionFloat(edge|sign));
 for(uint32_t bad:{1U,0x007fffffU,0x7f800000U,0x7fc00000U})for(uint32_t sign:{0U,0x80000000U}){
  float output=PioneerAttentionFloat(0x13579bdf);assert(!PioneerQ38SiluPrecision(PioneerAttentionFloat(bad|sign),output) && PioneerAttentionBits(output)==0x13579bdf);
 }
 printf("PASS isolated SiLU CPU-arithmetic cases%u OLD different%u/maxULP%u NEW different%u/maxULP%u/8atomic domain refusals/finite normal-or-zero inputs/final FTZ only;NOT emitted/native/physical proof or speed result\n",cases,oldDifferent,oldMaximum,newDifferent,newMaximum);
}
