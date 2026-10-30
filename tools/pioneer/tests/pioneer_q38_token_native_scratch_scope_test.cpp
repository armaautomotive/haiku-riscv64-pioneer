/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38TokenPrograms.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
struct Scope {
#include "pioneer_q38_token_native_scratch_scope.inc"
};
int main(){
 namespace P=PioneerQ38TokenPrograms;
 unsigned accepted=0,late=0,refused=0;
 for(uint32_t pc=0x64000;pc<0x6c000;pc+=4){
  uint32_t word=0;const bool installed=P::ProgramWord(0,pc,word);
  assert(Scope::ConvScratchPacket(true,pc,0x80000,4)==installed);
  assert(Scope::ConvScratchPacket(true,pc,0x80000,0x1100)==installed);
  assert(Scope::ConvScratchPacket(true,pc,0x810fc,4)==installed);
  assert(!Scope::ConvScratchPacket(false,pc,0x80000,4));refused++;
  if(installed){accepted++;if(pc>=0x68000)late++;}
 }
 assert(accepted==sizeof(P::RootAux)/4 && late==(accepted>4096?accepted-4096:0));
 for(uint32_t pc:{0U,0x40000U,0x60000U,0x63ffcU,0x64001U,0x6c000U,0x6e000U,UINT32_MAX}){
  assert(!Scope::ConvScratchPacket(true,pc,0x80000,4));refused++;
 }
 for(uint32_t local:{0U,0x78000U,0x7fffcU,0x80001U,0x81100U,UINT32_MAX}){
  assert(!Scope::ConvScratchPacket(true,0x64000,local,4));refused++;
 }
 for(uint32_t bytes:{0U,1U,3U,0x1104U,16384U,UINT32_MAX}){
  assert(!Scope::ConvScratchPacket(true,0x64000,0x80000,bytes));refused++;
 }
 assert(!Scope::ConvScratchPacket(true,0x64000,0x810fc,8));refused++;
 // The actual native SubmitLayer must still enforce all other packet,
 // route,owner and response rules;this helper grants none of those.
 printf("PASS selected native TEST auxiliary-image scratch scope/ALL8192 PC words/%u installed+%u beyond old16KiB cutoff/%u inactive+outside+unaligned+overflow refusals;DATA ONLY,NOT actual native/model/physical/TPS qualification\n",accepted,late,refused);
}
