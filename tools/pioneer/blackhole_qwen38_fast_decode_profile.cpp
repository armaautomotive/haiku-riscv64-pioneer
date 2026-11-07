/* SPDX-License-Identifier: MIT */
#include "PioneerQ38FastDecodeProfileBuildRole.h"
#include "PioneerQ38FastDecodeProfileData.h"
#ifndef PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE
#error Fast decode clock helper belongs only to its actual persistent ROOT TU
#endif
namespace Q=PioneerQ38FastDecodeProfileData;
#define FAST_PROFILE_FN __attribute__((noinline,section(".fast.decode.profile")))
static volatile uint32_t* Table(){return reinterpret_cast<volatile uint32_t*>(Q::Address);}
// Read-only CSRRS; no clock writes/reset/inhibit. Bad observations invalidate
// DATA only. No inference/NoC/ownership/terminal decision reads this result.
extern "C" FAST_PROFILE_FN bool q38_token_phase_clock(uint32_t* words){
 for(unsigned attempt=0;attempt<4;attempt++){
  uint32_t high,low,confirm;
  asm volatile("csrrs %0,0xb80,x0\ncsrrs %1,0xb00,x0\ncsrrs %2,0xb80,x0"
   :"=r"(high),"=r"(low),"=r"(confirm)::"memory");
  if(high==confirm){words[0]=low;words[1]=high;return true;}
 }
 return false;
}
extern "C" FAST_PROFILE_FN void q38_token_phase_begin(){Q::Initialize(Table());}
extern "C" FAST_PROFILE_FN void q38_fast_decode_profile_position(unsigned position,unsigned prompts){
 uint32_t now[2]={};bool valid=false;
 if(Table()[Q::State]==Q::Waiting && position==prompts)valid=q38_token_phase_clock(now);
 Q::Position(Table(),position,prompts,valid,uint64_t(now[0])|(uint64_t(now[1])<<32));
}
extern "C" FAST_PROFILE_FN void q38_token_phase_mark(unsigned phase){
 if(!Q::Running(Table()))return;
 uint32_t now[2]={};const bool valid=q38_token_phase_clock(now);
 Q::Advance(Table(),valid,uint64_t(now[0])|(uint64_t(now[1])<<32),phase);
}
extern "C" FAST_PROFILE_FN unsigned q38_fast_decode_profile_enter(unsigned phase){
 if(!Q::Running(Table()))return Q::Phases;
 const unsigned previous=Table()[Q::Current];q38_token_phase_mark(phase);return previous;
}
extern "C" FAST_PROFILE_FN void q38_token_phase_complete(unsigned committed){
 if(Table()[Q::State]==Q::Waiting){Q::Invalidate(Table(),Q::NoWindow);return;}
 if(!Q::Running(Table()))return;
 uint32_t now[2]={};const bool valid=q38_token_phase_clock(now);
 Q::Advance(Table(),valid,uint64_t(now[0])|(uint64_t(now[1])<<32),Q::Phases,committed);
}
#undef FAST_PROFILE_FN
