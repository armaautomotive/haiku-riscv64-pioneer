/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_REGISTER_AFFINE_REPLAY_MODEL_H
#define PIONEER_Q38_REGISTER_AFFINE_REPLAY_MODEL_H
#include <array>
#include <cstdint>
// Independent selected-contract functional frontend,NOT a timing model.
// Blackhole REPLAY shared specification:32 per-thread words,record+execute
// or replay. Reject noncontractual bits,load-only,undefined/nested/memory
// programs. Actual recorded words execute through the caller's ISA model.
struct ExactRegisterAffineReplay {
 std::array<uint32_t,32> words{};std::array<bool,32> defined{};
 unsigned remaining=0,next=0,records=0,replays=0,expanded=0;
 void Poison(){for(unsigned i=0;i<32;i++){words[i]=0xdeadc000+i;defined[i]=true;}}
 static bool RegisterOnly(unsigned op){const unsigned c=op>>24;return c==0x90 || c==0x85 || c==0x86 || c==0x8f;}
 template<class Emit> bool Push(unsigned op,uint32_t unknown,Emit emit){
  if(remaining){
   if(!RegisterOnly(op) || next>=25)throw unknown;
   words[next]=op;defined[next++]=true;remaining--;expanded++;emit(op);return true;
  }
  if(op>>24!=4)return false;
  const unsigned first=(op>>14)&31,count=(op>>4)&63,mode=op&3;
  if(op!=(0x04000000|(first<<14)|(count<<4)|mode) || first!=12 || count!=13 || (mode!=0 && mode!=3))throw unknown;
  if(mode==3){remaining=count;next=first;records++;return true;}
  for(unsigned i=0;i<count;i++)if(!defined[first+i] || !RegisterOnly(words[first+i]))throw unknown;
  replays++;for(unsigned i=0;i<count;i++){expanded++;emit(words[first+i]);}return true;
 }
};
#endif
