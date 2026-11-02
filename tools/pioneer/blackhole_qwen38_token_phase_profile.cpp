/* SPDX-License-Identifier: MIT */
#include "PioneerQ38TokenPhaseProfileImageRole.h"
#ifndef PIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE
#error This sampler is only the explicit full-token ROOT phase timing BUILD
#endif
#include "PioneerQ38TokenPhaseProfileData.h"
namespace Q=PioneerQ38TokenPhaseProfileData;
// Official Blackhole BabyRISCV CSRs.md: read-only CSRRS mcycleh/mcycle/x0.
// Four bounded attempts, no reset/inhibit/control writes. Uncertain clocks
// invalidate telemetry ONLY; no inference predicate or cleanup is bypassed.
extern "C" __attribute__((noinline)) bool q38_token_phase_clock(uint32_t* words){
 for(unsigned attempt=0;attempt<4;attempt++){
  uint32_t high,low,confirm;
  asm volatile("csrrs %0,0xb80,x0\ncsrrs %1,0xb00,x0\ncsrrs %2,0xb80,x0"
   :"=r"(high),"=r"(low),"=r"(confirm)::"memory");
  if(high==confirm){words[0]=low;words[1]=high;return true;}
 }
 return false;
}
static volatile uint32_t* Table(){return reinterpret_cast<volatile uint32_t*>(Q::Address);}
extern "C" __attribute__((noinline)) void q38_token_phase_begin(){
 uint32_t now[2]={};bool valid=q38_token_phase_clock(now);
 Q::Initialize(Table(),valid,uint64_t(now[0])|(uint64_t(now[1])<<32));
}
extern "C" __attribute__((noinline)) void q38_token_phase_mark(unsigned phase){
 if(!Q::Running(Table()))return;
 uint32_t now[2]={};bool valid=q38_token_phase_clock(now);
 Q::Advance(Table(),valid,uint64_t(now[0])|(uint64_t(now[1])<<32),phase);
}
extern "C" __attribute__((noinline)) void q38_token_phase_complete(unsigned committed){
 if(!Q::Running(Table()))return;
 uint32_t now[2]={};bool valid=q38_token_phase_clock(now);
 Q::Advance(Table(),valid,uint64_t(now[0])|(uint64_t(now[1])<<32),Q::Phases,committed);
}
