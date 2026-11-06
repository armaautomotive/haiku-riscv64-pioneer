/* SPDX-License-Identifier: MIT */
#include "PioneerQ38Bf16ProfileBuildRole.h"
#include "PioneerQ38Bf16ProfileData.h"
#if !defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE)
#error BF16 sampler must be linked from its actual persistent ROOT translation unit
#endif
namespace Q=PioneerQ38Bf16ProfileData;
#define BF_PROFILE_FN __attribute__((noinline,section(".bf16.profile")))
static volatile uint32_t* Table(){return reinterpret_cast<volatile uint32_t*>(Q::Address);}
// Same bounded READ-ONLY mcycleh/mcycle sampling as the qualified old
// phase profiler. Clock failure invalidates telemetry,never inference.
extern "C" BF_PROFILE_FN bool q38_token_phase_clock(uint32_t* words){
 for(unsigned attempt=0;attempt<4;attempt++){
  uint32_t high,low,confirm;
  asm volatile("csrrs %0,0xb80,x0\ncsrrs %1,0xb00,x0\ncsrrs %2,0xb80,x0"
   :"=r"(high),"=r"(low),"=r"(confirm)::"memory");
  if(high==confirm){words[0]=low;words[1]=high;return true;}
 }
 return false;
}
extern "C" BF_PROFILE_FN void q38_token_phase_begin(){
 uint32_t now[2]={};const bool valid=q38_token_phase_clock(now);
 Q::Initialize(Table(),valid,uint64_t(now[0])|(uint64_t(now[1])<<32));
}
extern "C" BF_PROFILE_FN void q38_token_phase_mark(unsigned phase){
 if(!Q::Running(Table()))return;
 uint32_t now[2]={};const bool valid=q38_token_phase_clock(now);
 Q::Advance(Table(),valid,uint64_t(now[0])|(uint64_t(now[1])<<32),phase);
}
extern "C" BF_PROFILE_FN unsigned q38_bf16_profile_enter(unsigned phase){
 if(!Q::Running(Table()))return Q::Phases;
 const unsigned previous=Table()[Q::Current];q38_token_phase_mark(phase);return previous;
}
extern "C" BF_PROFILE_FN void q38_token_phase_complete(unsigned committed){
 if(!Q::Running(Table()))return;
 uint32_t now[2]={};const bool valid=q38_token_phase_clock(now);
 Q::Advance(Table(),valid,uint64_t(now[0])|(uint64_t(now[1])<<32),Q::Phases,committed);
}
#undef BF_PROFILE_FN
