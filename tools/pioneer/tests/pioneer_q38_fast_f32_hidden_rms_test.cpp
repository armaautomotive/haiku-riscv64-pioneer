/* SPDX-License-Identifier: MIT */
// Host-only synthetic accuracy and safety screening; no model or card.
#define PIONEER_Q38_FAST_RMS_DIAGNOSTIC 1
#include "../PioneerQ38FastF32HiddenRms.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cfenv>
#include <cmath>
#include <cstdio>

int main(){
 (void)&PioneerQ38LearnedRms;(void)&PioneerQ38F32HiddenResidualRms;
 assert(std::fegetround()==FE_TONEAREST);
 std::array<float,5120> input={},weight={},precise={},fast={};
 double worstRelative=0,worstAbsolute=0;unsigned changedWords=0,rows=0;
 uint32_t state=0x1264b381;
 for(unsigned trial=0;trial<256;trial++){
  for(unsigned i=0;i<5120;i++){
   state=state*1664525U+1013904223U;
   const int x=int((state>>8)%20001)-10000;
   const float factor=trial%4==0?1.0f/1024:trial%4==1?1.0f/16384:trial%4==2?32.0f:0.0001220703125f;
   input[i]=float(x)*factor;
   state=state*1664525U+1013904223U;
   weight[i]=float(int((state>>8)%2001)-1000)*(1.0f/1024);
  }
  if(trial==0)input.fill(0);
  if(trial==1)input.fill(67108864.f);
  if(trial==2){input.fill(0);input[5119]=67108864.f;}
  assert(PioneerQ38F32HiddenRms(input.data(),weight.data(),precise.data()));
  assert(PioneerQ38FastF32HiddenRms(input.data(),weight.data(),fast.data()));
  double error=0,norm=0;
  for(unsigned i=0;i<5120;i++){
   const double a=precise[i],b=fast[i],delta=b-a;
   assert(std::isfinite(a) && std::isfinite(b));
   error+=delta*delta;norm+=a*a;worstAbsolute=std::max(worstAbsolute,std::abs(delta));
   changedWords+=PioneerAttentionBits(precise[i])!=PioneerAttentionBits(fast[i]);
  }
  const double relative=std::sqrt(error/(norm+1e-300));worstRelative=std::max(worstRelative,relative);
  assert(relative<0.00002);rows++;
 }
 input.fill(1);weight.fill(1);fast.fill(42);const auto untouched=fast;
 for(unsigned index:{0U,5119U})for(uint32_t bad:{1U,0x267fffffU,0x4c800001U,0x7f800000U,0x7fc00000U}){
  input[index]=PioneerAttentionFloat(bad);
  assert(!PioneerQ38FastF32HiddenRms(input.data(),weight.data(),fast.data()) && fast==untouched);
  input[index]=1;
 }
 for(uint32_t bad:{0x47800000U,0x7f800000U,0x7fc00000U}){
  weight[5119]=PioneerAttentionFloat(bad);
  assert(!PioneerQ38FastF32HiddenRms(input.data(),weight.data(),fast.data()) && fast==untouched);
  weight[5119]=1;
 }
 assert(!PioneerQ38FastF32HiddenRms(input.data(),weight.data(),input.data()));
 printf("PASS approximate FP32 RMS synthetic rows%u changed_words%u worst_relative_l2%.9g worst_absolute%.9g; "
  "NOT real-model quality, emitted device code, hardware or tokens/sec\n",rows,changedWords,worstRelative,worstAbsolute);
}
