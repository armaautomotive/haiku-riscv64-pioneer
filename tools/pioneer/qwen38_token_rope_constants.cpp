/* SPDX-License-Identifier: MIT */
// OFFLINE fixed model-configuration DATA,not hidden/KV/model products or an
// inference/validation backend. Kernel startup uses the frozen bits,NO libm.
#include <cassert>
#include <cfenv>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
int main(){
 static_assert(sizeof(float)==4,"IEEE binary32 raw positional constants");
 assert(std::fesetround(FE_TONEAREST)==0);
 for(unsigned position=0;position<8192;position++)for(unsigned i=0;i<32;i++){
  const float frequency=powf(10000000.f,-float(i)/32.f),angle=float(position)*frequency;
  const float values[]={cosf(angle),sinf(angle)};
  for(float value:values){
   assert(std::isfinite(value) && fabsf(value)<=1.f);uint32_t word;std::memcpy(&word,&value,4);
   unsigned char bytes[4];for(unsigned b=0;b<4;b++)bytes[b]=static_cast<unsigned char>(word>>(8*b));
   assert(std::fwrite(bytes,1,4,stdout)==4);
  }
 }
 assert(std::fflush(stdout)==0);
}
